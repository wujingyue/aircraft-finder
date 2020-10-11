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

using namespace std;

enum Color : char {
  kWhite = 'w',
  kGray = 'g',
  kBlue = 'b',
  kRed = 'r',
};

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
        workqueue_(workqueue) {
    aircraft_bodies_.resize(4);
    aircraft_bodies_[0] = {{0, 0}, {1, -2}, {1, -1}, {1, 0}, {1, 1},
                           {1, 2}, {2, 0},  {3, -1}, {3, 0}, {3, 1}};
    for (int k = 1; k < 4; k++) {
      for (const pair<int, int>& body : aircraft_bodies_[k - 1]) {
        aircraft_bodies_[k].push_back({-body.second, body.first});
      }
    }
  }

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
            (int)aircraft_bodies_[0].size() <
        num_remaining_known_bodies) {
      return 0;
    }

    if (num_aircrafts_ == (int)aircraft_positions.size()) {
      UpdateHeatmap(aircraft_positions, heatmap);
      return 1;
    }

    vector<pair<int, int>> placed;
    placed.reserve(aircraft_bodies_[0].size());

    auto Process = [this, num_remaining_known_bodies, &aircraft_positions,
                    &occupied, &heatmap,
                    &placed](int x, int y, int dir) -> int {
      int num_combinations = 0;

      int num_known_bodies_covered = 0;
      if (TryLand(occupied, x, y, dir, &placed, &num_known_bodies_covered)) {
        aircraft_positions.push_back(AircraftPosition{x, y, dir});
        num_combinations =
            DFS(num_remaining_known_bodies - num_known_bodies_covered,
                aircraft_positions, occupied, heatmap);
        aircraft_positions.pop_back();
      }
      Lift(occupied, &placed);

      return num_combinations;
    };

    int num_combinations = 0;
    if (aircraft_positions.empty()) {
      AircraftPosition pos;
      while (workqueue_.Pop(&pos)) {
        num_combinations += Process(pos.x, pos.y, pos.dir);
      }
    } else {
      for (int x = 0; x < r_; x++) {
        for (int y = 0; y < c_; y++) {
          const AircraftPosition& prev = aircraft_positions.back();
          if (make_pair(x, y) <= make_pair(prev.x, prev.y)) {
            continue;
          }
          for (int dir = 0; dir < 4; dir++) {
            num_combinations += Process(x, y, dir);
          }
        }
      }
    }
    return num_combinations;
  }

  bool TryLand(vector<vector<bool>>& occupied, int x, int y, int dir,
               vector<pair<int, int>>* placed,
               int* num_known_bodies_covered) const {
    for (const pair<int, int>& body : aircraft_bodies_[dir]) {
      int dx = body.first;
      int dy = body.second;
      int x2 = x + dx;
      int y2 = y + dy;
      if (x2 < 0 || x2 >= r_ || y2 < 0 || y2 >= c_) {
        return false;
      }
      if (occupied[x2][y2]) {
        return false;
      }
      Color new_color = (dx == 0 && dy == 0 ? kRed : kBlue);
      if (board_[x2][y2] != kGray) {
        if (board_[x2][y2] != new_color) {
          return false;
        }
        (*num_known_bodies_covered)++;
      }
      occupied[x2][y2] = true;
      placed->push_back({x2, y2});
    };
    return true;
  }

  void Lift(vector<vector<bool>>& occupied,
            vector<pair<int, int>>* placed) const {
    while (!placed->empty()) {
      int x = placed->back().first;
      int y = placed->back().second;
      placed->pop_back();
      occupied[x][y] = false;
    }
  }

  void UpdateHeatmap(const vector<AircraftPosition>& aircraft_positions,
                     Heatmap& heatmap) const {
    for (const auto& pos : aircraft_positions) {
      for (const pair<int, int>& body : aircraft_bodies_[pos.dir]) {
        int dx = body.first;
        int dy = body.second;
        int x2 = pos.x + dx;
        int y2 = pos.y + dy;
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

  vector<vector<pair<int, int>>> aircraft_bodies_;
};

class Solution {
 public:
  Solution(int r, int c, int num_aircrafts)
      : r_(r),
        c_(c),
        num_aircrafts_(num_aircrafts),
        board_(r, vector<Color>(c, kGray)) {}

  void SetColor(int x, int y, Color color) { board_[x][y] = color; }

  pair<int, int> PrintProbabilityMatrix() const {
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
         [](const CellProbability& p1, const CellProbability& p2) {
           const double e1 = p1.prob.Entropy();
           const double e2 = p2.prob.Entropy();
           return e1 > e2 || (e1 == e2 && p1.prob.Red() > p2.prob.Red());
         });

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

    const pair<int, int>& top_cell = top_cells[0];
    printf("(%d, %c) > ", top_cell.first + 1, 'A' + top_cell.second);
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

int main(int argc, char* argv[]) {
  if (argc < 4) {
    cerr << "Usage error: ./main <num rows> <num colums> <num aircrafts>"
         << endl;
    return 1;
  }

  int r = atoi(argv[1]);
  int c = atoi(argv[2]);
  int num_aircrafts = atoi(argv[3]);
  Solution s(r, c, num_aircrafts);

  int x;
  int y;
  tie(x, y) = s.PrintProbabilityMatrix();

  string line;
  while (getline(cin, line)) {
    char color;

    istringstream iss(line);
    iss >> color;
    if (isdigit(color)) {
      iss.str(line);

      char char_y;
      iss >> x >> char_y >> color;
      x--;
      y = char_y - (isupper(char_y) ? 'A' : 'a');
    }

    s.SetColor(x, y, static_cast<Color>(color));
    tie(x, y) = s.PrintProbabilityMatrix();
  }

  return 0;
}
