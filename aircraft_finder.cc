#include <unistd.h>

#include <algorithm>
#include <cmath>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <sstream>
#include <thread>
#include <vector>

#include "aircraft_placer.h"
#include "color.h"

using namespace std;

struct Frequency {
  int red = 0;
  int blue = 0;
  int white = 0;
};

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

class Probability {
 public:
  explicit Probability(const Frequency& freq) {
    red_ = freq.red;
    blue_ = freq.blue;
    white_ = freq.white;
    Normalize();
  }

  double Entropy() const {
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

  double Red() const { return red_; }
  double Blue() const { return blue_; }
  double White() const { return white_; }

 private:
  void Normalize() {
    double sum = red_ + blue_ + white_;
    red_ /= sum;
    blue_ /= sum;
    white_ /= sum;
  }

  double red_;
  double blue_;
  double white_;
};

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
      for (const pair<int, int>& body :
           placer_.GetAircraftBody(pos.dir)) {
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

class AircraftFinder {
 public:
  AircraftFinder(int r, int c, int num_aircrafts)
      : r_(r),
        c_(c),
        num_aircrafts_(num_aircrafts),
        board_(r, vector<Color>(c, kGray)) {}

  void SetColor(int x, int y, Color color) { board_[x][y] = color; }

  pair<int, int> GetCellToBomb(const bool print_entropy_matrix) const {
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

    pair<int, int> top_cell(cell_probabilities[0].x, cell_probabilities[0].y);

    if (print_entropy_matrix) {
      vector<pair<int, int>> top_cells;
      constexpr int kHighlightLimit = 1;
      for (int i = 0; i < kHighlightLimit; i++) {
        const int x = cell_probabilities[i].x;
        const int y = cell_probabilities[i].y;
        top_cells.push_back({x, y});
      }

      printf("  ");
      for (int y = 0; y < c_; y++) {
        printf("%6c", 'A' + y);
      }
      printf("\n");
      for (int x = 0; x < r_; x++) {
        printf("%2d: ", x + 1);
        for (int y = 0; y < c_; y++) {
          bool is_top = find(top_cells.begin(), top_cells.end(),
                             make_pair(x, y)) != top_cells.end();
          PrintCell(normalized_heatmap[x][y], is_top, board_[x][y] != kGray);
        }
        printf("\n");
      }
    }

    return top_cell;
  }

 private:
  void PrintCell(const Probability& p, const bool is_top,
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

  const int r_;
  const int c_;
  const int num_aircrafts_;
  vector<vector<Color>> board_;
};

void PrintUsage(const char* exec_name) {
  cerr << "Usage: " << exec_name << " -r rows -c cols -n aircrafts" << endl;
}

int main(int argc, char* argv[]) {
  int rows = 0;
  int cols = 0;
  int num_aircrafts = 0;

  int opt;
  while ((opt = getopt(argc, argv, "r:c:n:")) != -1) {
    switch (opt) {
      case 'r':
        rows = atoi(optarg);
        break;
      case 'c':
        cols = atoi(optarg);
        break;
      case 'n':
        num_aircrafts = atoi(optarg);
        break;
      default:
        PrintUsage(argv[0]);
        return 1;
    }
  }

  if (rows <= 0 || cols <= 0 || num_aircrafts <= 0) {
    PrintUsage(argv[0]);
    return 1;
  }

  AircraftFinder finder(rows, cols, num_aircrafts);

  while (true) {
    int x;
    int y;
    tie(x, y) = finder.GetCellToBomb(true);
    printf("(%d, %c) > ", x + 1, 'A' + y);

    string line;
    if (!getline(cin, line)) {
      break;
    }

    char color;
    istringstream iss(line);
    iss >> color;
    // If the line contains a color only, reuse the cell to bomb.
    if (isdigit(color)) {
      iss.str(line);

      char char_y;
      iss >> x >> char_y >> color;
      x--;
      y = char_y - (isupper(char_y) ? 'A' : 'a');
    }

    finder.SetColor(x, y, static_cast<Color>(color));
  }

  return 0;
}
