#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>

using namespace std;

enum Color : char {
  kWhite = 'w',
  kGray = 'g',
  kBlue = 'b',
  kRed = 'r',
};

struct CellEntropy {
  int x;
  int y;
  double entropy;
};

struct Frequency {
  int red = 0;
  int blue = 0;
  int white = 0;
};

class Probability {
 public:
  explicit Probability(Frequency freq) {
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

struct AircraftPosition {
  int x;
  int y;
  int dir;
};

class Solution {
 public:
  Solution(int r, int c, int num_aircrafts)
      : r_(r),
        c_(c),
        num_aircrafts_(num_aircrafts),
        board_(r, vector<Color>(c, kGray)),
        known_bodies_(0) {
    aircraft_bodies_.resize(4);
    aircraft_bodies_[0] = {{0, 0}, {1, -2}, {1, -1}, {1, 0}, {1, 1},
                           {1, 2}, {2, 0},  {3, -1}, {3, 0}, {3, 1}};
    for (int k = 1; k < 4; k++) {
      for (const pair<int, int>& body : aircraft_bodies_[k - 1]) {
        aircraft_bodies_[k].push_back({-body.second, body.first});
      }
    }
  }

  void SetColor(int x, int y, Color color) {
    if (board_[x][y] != kGray) {
      cerr << "board_[" << x << ", " << y << "] is already set to "
           << board_[x][y] << endl;
      return;
    }
    board_[x][y] = color;
    if (color == kBlue || color == kRed) {
      known_bodies_++;
    }
  }

  void PrintProbabilityMatrix() const {
    vector<vector<Frequency>> heatmap(r_, vector<Frequency>(c_));
    vector<vector<bool>> occupied(r_, vector<bool>(c_));
    vector<AircraftPosition> aircraft_positions;
    aircraft_positions.reserve(num_aircrafts_);
    int num_combinations =
        DFS(known_bodies_, aircraft_positions, occupied, heatmap);

    for (int x = 0; x < r_; x++) {
      for (int y = 0; y < c_; y++) {
        heatmap[x][y].white =
            num_combinations - heatmap[x][y].red - heatmap[x][y].blue;
      }
    }

    vector<vector<Probability>> normalized_heatmap;
    normalized_heatmap.resize(r_);
    vector<CellEntropy> cell_entropies;
    for (int x = 0; x < r_; x++) {
      normalized_heatmap.reserve(c_);
      for (int y = 0; y < c_; y++) {
        normalized_heatmap[x].push_back(Probability(heatmap[x][y]));
        cell_entropies.push_back(
            CellEntropy{x, y, normalized_heatmap[x][y].Entropy()});
      }
    }
    sort(cell_entropies.begin(), cell_entropies.end(),
         [](const CellEntropy& e1, const CellEntropy& e2) {
           return e1.entropy > e2.entropy;
         });

    vector<pair<int, int>> top_cells;
    constexpr int kPrintLimit = 1;
    for (int i = 0; i < kPrintLimit; i++) {
      top_cells.push_back({cell_entropies[i].x, cell_entropies[i].y});
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

 private:
  int DFS(const int num_remaining_known_bodies,
          vector<AircraftPosition>& aircraft_positions,
          vector<vector<bool>>& occupied,
          vector<vector<Frequency>>& heatmap) const {
    if ((num_aircrafts_ - (int)aircraft_positions.size()) *
            (int)aircraft_bodies_[0].size() <
        num_remaining_known_bodies) {
      return 0;
    }

    if (num_aircrafts_ == (int)aircraft_positions.size()) {
      UpdateHeatmap(aircraft_positions, heatmap);
      return 1;
    }

    int num_combinations = 0;
    vector<pair<int, int>> placed;
    placed.reserve(aircraft_bodies_[0].size());
    for (int x = 0; x < r_; x++) {
      for (int y = 0; y < c_; y++) {
        if (!aircraft_positions.empty()) {
          const AircraftPosition& prev = aircraft_positions.back();
          if (make_pair(x, y) <= make_pair(prev.x, prev.y)) {
            continue;
          }
        }
        for (int dir = 0; dir < 4; dir++) {
          int num_known_bodies_covered = 0;
          if (TryLand(occupied, x, y, dir, &placed,
                      &num_known_bodies_covered)) {
            aircraft_positions.push_back(AircraftPosition{x, y, dir});
            num_combinations +=
                DFS(num_remaining_known_bodies - num_known_bodies_covered,
                    aircraft_positions, occupied, heatmap);
            aircraft_positions.pop_back();
          }
          Lift(occupied, &placed);
        }
      }
    }
    return num_combinations;
  }

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

  void UpdateHeatmap(const vector<AircraftPosition>& aircraft_positions,
                     vector<vector<Frequency>>& heatmap) const {
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

  const int r_;
  const int c_;
  const int num_aircrafts_;
  vector<vector<Color>> board_;
  int known_bodies_;
  vector<vector<pair<int, int>>> aircraft_bodies_;
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
  char char_y;
  char color;
  s.PrintProbabilityMatrix();
  while (cin >> x >> char_y >> color) {
    x--;
    int y = char_y - (isupper(char_y) ? 'A' : 'a');
    s.SetColor(x, y, static_cast<Color>(color));
    s.PrintProbabilityMatrix();
  }

  return 0;
}
