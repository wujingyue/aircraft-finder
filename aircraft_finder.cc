#include "aircraft_finder.h"

#include <unistd.h>

#include <algorithm>
#include <cmath>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

#include "aircraft_placer.h"
#include "color.h"

using namespace std;

class Heatmap : public vector<vector<Frequency>> {
 public:
  Heatmap(const int r, const int c)
      : vector<vector<Frequency>>(r, vector<Frequency>(c)), r_(r), c_(c) {}

  Heatmap& operator+=(const Heatmap& other) {
    for (int x = 0; x < r_; x++) {
      for (int y = 0; y < c_; y++) {
        (*this)[x][y].red += other[x][y].red;
        (*this)[x][y].blue += other[x][y].blue;
        (*this)[x][y].white += other[x][y].white;
      }
    }
    return *this;
  }

 private:
  const int r_;
  const int c_;
};

Probability::Probability(const Frequency& freq) {
  red_ = freq.red;
  blue_ = freq.blue;
  white_ = freq.white;
  Normalize();
}

double Probability::Entropy() const {
  double entropy = 0.0;
  if (red_ > 0.0) {
    entropy -= log(red_) * red_;
  }
  if (blue_ > 0.0) {
    entropy -= log(blue_) * blue_;
  }
  if (white_ > 0.0) {
    entropy -= log(white_) * white_;
  }
  return entropy;
}

void Probability::Normalize() {
  double sum = red_ + blue_ + white_;
  red_ /= sum;
  blue_ /= sum;
  white_ /= sum;
}

struct CellProbability {
  int x;
  int y;
  Probability prob;
};

struct AircraftPosition {
  int x;
  int y;
  int dir;
};

class Workqueue {
 public:
  void Add(const AircraftPosition& pos) {
    lock_guard<mutex> guard(mutex_);
    workqueue_.push(pos);
  }

  bool Pop(AircraftPosition* pos) {
    lock_guard<mutex> guard(mutex_);
    if (workqueue_.empty()) {
      return false;
    }
    *pos = workqueue_.front();
    workqueue_.pop();
    return true;
  }

 private:
  queue<AircraftPosition> workqueue_;
  mutex mutex_;
};

class DFSHelper {
 public:
  DFSHelper(const vector<vector<Color>>& board, const int num_aircrafts,
            Workqueue& workqueue)
      : board_(board),
        r_(board.size()),
        c_(board[0].size()),
        num_aircrafts_(num_aircrafts),
        workqueue_(workqueue),
        placer_(board) {}

  Heatmap ComputeHeatmap() const {
    int known_bodies = 0;
    for (int x = 0; x < r_; x++) {
      for (int y = 0; y < c_; y++) {
        known_bodies += (board_[x][y] == kBlue || board_[x][y] == kRed);
      }
    }

    vector<AircraftPosition> aircraft_positions;
    aircraft_positions.reserve(num_aircrafts_);

    vector<vector<bool>> occupied(r_, vector<bool>(c_));

    Heatmap heatmap(r_, c_);
    int num_combinations =
        DFS(known_bodies, aircraft_positions, occupied, heatmap);
    for (int x = 0; x < r_; x++) {
      for (int y = 0; y < c_; y++) {
        heatmap[x][y].white =
            num_combinations - heatmap[x][y].red - heatmap[x][y].blue;
      }
    }
    return heatmap;
  }

 private:
  int DFS(const int num_remaining_known_bodies,
          vector<AircraftPosition>& aircraft_positions,
          vector<vector<bool>>& occupied, Heatmap& heatmap) const {
    if ((num_aircrafts_ - (int)aircraft_positions.size()) *
            placer_.AircraftSize() <
        num_remaining_known_bodies) {
      return 0;
    }

    if (num_aircrafts_ == (int)aircraft_positions.size()) {
      UpdateHeatmap(aircraft_positions, heatmap);
      return 1;
    }

    vector<pair<int, int>> placed;
    placed.reserve(placer_.AircraftSize());

    auto Process = [this, &aircraft_positions, &occupied, &heatmap, &placed](
                       int x, int y, int dir,
                       int num_remaining_known_bodies) -> int {
      int num_combinations = 0;

      if (placer_.TryLand(x, y, dir, &occupied, &placed)) {
        for (const pair<int, int>& p : placed) {
          if (board_[p.first][p.second] != kGray) {
            num_remaining_known_bodies--;
          }
        }
        aircraft_positions.push_back(AircraftPosition{x, y, dir});
        num_combinations = DFS(num_remaining_known_bodies, aircraft_positions,
                               occupied, heatmap);
        aircraft_positions.pop_back();
      }
      placer_.Lift(&occupied, &placed);

      return num_combinations;
    };

    int num_combinations = 0;
    if (aircraft_positions.empty()) {
      AircraftPosition pos;
      while (workqueue_.Pop(&pos)) {
        num_combinations +=
            Process(pos.x, pos.y, pos.dir, num_remaining_known_bodies);
      }
    } else {
      for (int x = 0; x < r_; x++) {
        for (int y = 0; y < c_; y++) {
          const AircraftPosition& prev = aircraft_positions.back();
          if (make_pair(x, y) <= make_pair(prev.x, prev.y)) {
            continue;
          }
          for (int dir = 0; dir < 4; dir++) {
            num_combinations += Process(x, y, dir, num_remaining_known_bodies);
          }
        }
      }
    }
    return num_combinations;
  }

  void UpdateHeatmap(const vector<AircraftPosition>& aircraft_positions,
                     Heatmap& heatmap) const {
    for (const auto& pos : aircraft_positions) {
      for (const pair<int, int>& body : placer_.GetAircraftBody(pos.dir)) {
        const int dx = body.first;
        const int dy = body.second;
        const int x2 = pos.x + dx;
        const int y2 = pos.y + dy;
        if (dx == 0 && dy == 0) {
          heatmap[x2][y2].red++;
        } else {
          heatmap[x2][y2].blue++;
        }
      }
    }
  }

  const vector<vector<Color>>& board_;
  const int r_;
  const int c_;
  const int num_aircrafts_;

  Workqueue& workqueue_;

  AircraftPlacer placer_;
};

AircraftFinder::AircraftFinder(int r, int c, int num_aircrafts)
    : r_(r),
      c_(c),
      num_aircrafts_(num_aircrafts),
      board_(r, vector<Color>(c, kGray)) {}

pair<int, int> AircraftFinder::GetCellToBomb(
    const bool print_entropy_matrix) const {
  Workqueue workqueue;
  for (int x = 0; x < r_; x++) {
    for (int y = 0; y < c_; y++) {
      for (int dir = 0; dir < 4; dir++) {
        workqueue.Add(AircraftPosition{x, y, dir});
      }
    }
  }

  const int num_threads = thread::hardware_concurrency();
  vector<unique_ptr<DFSHelper>> workers;
  workers.reserve(num_threads);
  vector<future<Heatmap>> heatmap_per_worker;
  heatmap_per_worker.reserve(num_threads);
  for (int i = 0; i < num_threads; i++) {
    auto helper = make_unique<DFSHelper>(board_, num_aircrafts_, workqueue);
    heatmap_per_worker.push_back(
        async(launch::async, &DFSHelper::ComputeHeatmap, helper.get()));
    workers.push_back(move(helper));
  }

  Heatmap heatmap(r_, c_);
  for (int i = 0; i < num_threads; i++) {
    heatmap += heatmap_per_worker[i].get();
  }

  vector<vector<Probability>> normalized_heatmap;
  normalized_heatmap.resize(r_);
  vector<CellProbability> cell_probabilities;
  for (int x = 0; x < r_; x++) {
    normalized_heatmap.reserve(c_);
    for (int y = 0; y < c_; y++) {
      Probability prob(heatmap[x][y]);
      normalized_heatmap[x].push_back(prob);
      cell_probabilities.push_back(CellProbability{x, y, prob});
    }
  }

  double max_red = 0.0;
  pair<int, int> top_cell(-1, -1);
  for (const CellProbability& p : cell_probabilities) {
    if (board_[p.x][p.y] == kGray && p.prob.Red() > max_red) {
      max_red = p.prob.Red();
      top_cell = make_pair(p.x, p.y);
    }
  }
  constexpr double kThresholdMustBomb = 0.5;
  if (max_red < kThresholdMustBomb) {
    sort(cell_probabilities.begin(), cell_probabilities.end(),
         [this](const CellProbability& p1, const CellProbability& p2) {
           // Pick the cell with a larger entropy.
           const double e1 = p1.prob.Entropy();
           const double e2 = p2.prob.Entropy();
           if (e1 != e2) {
             return e1 > e2;
           }

           // Pick the cell whose color is unknown.
           const bool known1 = board_[p1.x][p1.y] != kGray;
           const bool known2 = board_[p2.x][p2.y] != kGray;
           if (known1 != known2) {
             return known1 < known2;
           }

           // Pick the cell that is more likely to be red.
           return p1.prob.Red() > p2.prob.Red();
         });
    top_cell = make_pair(cell_probabilities[0].x, cell_probabilities[0].y);
  }

  if (print_entropy_matrix) {
    printf("  ");
    for (int y = 0; y < c_; y++) {
      printf("%6c", 'A' + y);
    }
    printf("\n");
    for (int x = 0; x < r_; x++) {
      printf("%2d: ", x + 1);
      for (int y = 0; y < c_; y++) {
        PrintCell(normalized_heatmap[x][y], make_pair(x, y) == top_cell,
                  board_[x][y] != kGray);
      }
      printf("\n");
    }
  }

  return top_cell;
}

void AircraftFinder::PrintCell(const Probability& p, const bool is_top,
                               const bool is_known) const {
  double max_probability = max(p.Red(), max(p.Blue(), p.White()));
  int color_code;
  if (p.Red() == max_probability) {
    color_code = 31;
  } else if (p.Blue() == max_probability) {
    color_code = 34;
  } else {
    color_code = 30;
  }

  int style_code = 0;
  if (is_top) {
    style_code = 1;
  } else if (is_known) {
    style_code = 9;
  }
  printf("\033[%d;%dm", style_code, color_code);
  printf("%5.1f ", p.Entropy() * 100);
  printf("\33[0m");
}
