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

struct Probability {
  double red = 0.0;
  double blue = 0.0;
  double white = 0.0;

  double Entropy() const {
    double entropy = 0.0;
    if (red > 0.0) {
      entropy -= log(red) * red;
    }
    if (blue > 0.0) {
      entropy -= log(blue) * blue;
    }
    if (white > 0.0) {
      entropy -= log(white) * white;
    }
    return entropy;
  }

  void Normalize() {
    double sum = red + blue + white;
    red /= sum;
    blue /= sum;
    white /= sum;
  }
};

class Solution {
 public:
  Solution(int r, int c, int num_aircrafts)
      : r_(r),
        c_(c),
        num_aircrafts_(num_aircrafts),
        board_(r, vector<Color>(c, kGray)),
        aircraft_locations_({{0, 0},
                             {1, -2},
                             {1, -1},
                             {1, 0},
                             {1, 1},
                             {1, 2},
                             {2, 0},
                             {3, -1},
                             {3, 0},
                             {3, 1}}) {}

  void SetColor(int x, int y, Color color) {
    if (board_[x][y] != kGray) {
      cerr << "board_[" << x << ", " << y << "] is already set to "
           << board_[x][y] << endl;
      return;
    }
    board_[x][y] = color;
  }

  void PrintProbabilityMatrix() const {
    vector<vector<Probability>> heatmap(r_, vector<Probability>(c_));
    vector<vector<Color>> scratch_board(r_, vector<Color>(c_, kWhite));
    DFS(num_aircrafts_, -1, -1, scratch_board, heatmap);

    vector<CellEntropy> cell_entropies;
    for (int x = 0; x < r_; x++) {
      for (int y = 0; y < c_; y++) {
        heatmap[x][y].Normalize();
        cell_entropies.push_back(CellEntropy{x, y, heatmap[x][y].Entropy()});
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
        PrintCell(heatmap[x][y], is_top);
      }
      printf("\n");
    }
  }

 private:
  void DFS(const int num_remaining, const int prev_x, const int prev_y,
           vector<vector<Color>>& scratch_board,
           vector<vector<Probability>>& heatmap) const {
    if (num_remaining == 0) {
      if (Matches(scratch_board)) {
        UpdateHeatmap(scratch_board, heatmap);
      }
      return;
    }
    vector<pair<int, int>> placed;
    placed.reserve(10);
    for (int x = 0; x < r_; x++) {
      for (int y = 0; y < c_; y++) {
        if (make_pair(x, y) <= make_pair(prev_x, prev_y)) {
          continue;
        }
        for (int dir = 0; dir < 4; dir++) {
          if (TryLand(scratch_board, x, y, dir, &placed)) {
            DFS(num_remaining - 1, x, y, scratch_board, heatmap);
          }
          Lift(scratch_board, &placed);
        }
      }
    }
  }

  void PrintCell(const Probability& p, const bool is_top) const {
    double max_probability = max(p.red, max(p.blue, p.white));
    int color_code;
    if (p.red == max_probability) {
      color_code = 31;
    } else if (p.blue == max_probability) {
      color_code = 34;
    } else {
      color_code = 30;
    }

    printf("\033[%d;%dm", is_top, color_code);
    printf("%5.1f ", p.Entropy() * 100);
    printf("\33[0m");
  }

  void UpdateHeatmap(const vector<vector<Color>>& b,
                     vector<vector<Probability>>& heatmap) const {
    for (int x = 0; x < r_; x++) {
      for (int y = 0; y < c_; y++) {
        switch (b[x][y]) {
          case kRed:
            heatmap[x][y].red++;
            break;
          case kBlue:
            heatmap[x][y].blue++;
            break;
          case kWhite:
            heatmap[x][y].white++;
            break;
          default:
            break;
        }
      }
    }
  }

  bool TryLand(vector<vector<Color>>& b, int x, int y, int dir,
               vector<pair<int, int>>* placed) const {
    for (const pair<int, int>& location : aircraft_locations_) {
      int dx = location.first;
      int dy = location.second;
      for (int k = 0; k < dir; k++) {
        // Rotate (dx, dy) by 90 degrees.
        int orig_dx = dx;
        dx = -dy;
        dy = orig_dx;
      }
      int x2 = x + dx;
      int y2 = y + dy;
      if (x2 < 0 || x2 >= r_ || y2 < 0 || y2 >= c_) {
        return false;
      }
      if (b[x2][y2] != kWhite) {
        return false;
      }
      b[x2][y2] = (dx == 0 && dy == 0 ? kRed : kBlue);
      placed->push_back({x2, y2});
    };
    return true;
  }

  void Lift(vector<vector<Color>>& b, vector<pair<int, int>>* placed) const {
    while (!placed->empty()) {
      int x = placed->back().first;
      int y = placed->back().second;
      placed->pop_back();
      b[x][y] = kWhite;
    }
  }

  bool Matches(const vector<vector<Color>>& b) const {
    for (int x = 0; x < r_; x++) {
      for (int y = 0; y < c_; y++) {
        if (board_[x][y] == kGray) {
          continue;
        }
        if (board_[x][y] != b[x][y]) {
          return false;
        }
      }
    }
    return true;
  }

  const int r_;
  const int c_;
  const int num_aircrafts_;
  vector<vector<Color>> board_;
  const vector<pair<int, int>> aircraft_locations_;
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
