#include <algorithm>
#include <iostream>
#include <tuple>
#include <vector>

using namespace std;

enum Color : char {
  kWhite = 'w',
  kGray = 'g',
  kBlue = 'b',
  kRed = 'r',
};

class Solution {
 public:
  Solution(int r, int c) : r_(r), c_(c), board_(r, vector<Color>(c, kGray)) {}

  void SetColor(int x, int y, Color color) {
    if (board_[x][y] != kGray) {
      cerr << "board_[" << x << ", " << y << "] is already set to "
           << board_[x][y] << endl;
      return;
    }
    board_[x][y] = color;
  }

  void PrintProbabilityMatrix() const {
    vector<vector<int>> probability(r_, vector<int>(c_));

    vector<vector<Color>> scratch_board(r_, vector<Color>(c_, kWhite));
    int sum_probabilities = 0;
    for (int x1 = 0; x1 < r_; x1++) {
      for (int y1 = 0; y1 < c_; y1++) {
        for (int dir1 = 0; dir1 < 4; dir1++) {
          if (!TryLand(scratch_board, x1, y1, dir1)) {
            continue;
          }
          for (int x2 = 0; x2 < r_; x2++) {
            for (int y2 = 0; y2 < c_; y2++) {
              for (int dir2 = 0; dir2 < 4; dir2++) {
                if (!TryLand(scratch_board, x2, y2, dir2)) {
                  continue;
                }
                if (Matches(scratch_board)) {
                  probability[x1][y1]++;
                  probability[x2][y2]++;
                  sum_probabilities += 2;
                }
                Lift(scratch_board, x2, y2, dir2);
              }
            }
          }
          Lift(scratch_board, x1, y1, dir1);
        }
      }
    }

    vector<tuple<float, int, int>> probabilities;
    for (int y = 0; y < c_; y++) {
      printf("%6d", y);
    }
    printf("\n");
    for (int x = 0; x < r_; x++) {
      printf("%d: ", x);
      for (int y = 0; y < c_; y++) {
        float normalized_probability =
            (float)probability[x][y] / sum_probabilities;
        probabilities.push_back(make_tuple(normalized_probability, x, y));
        printf("%.3f ", normalized_probability);
      }
      printf("\n");
    }
    sort(probabilities.begin(), probabilities.end(),
         [](const tuple<double, int, int>& p1,
            const tuple<double, int, int>& p2) {
           return get<0>(p1) > get<0>(p2);
         });
    printf("Top 5:\n");
    for (int i = 0; i < 5; i++) {
      printf("%.3f (%d, %d)\n", get<0>(probabilities[i]),
             get<1>(probabilities[i]), get<2>(probabilities[i]));
    }
  }

 private:
  bool TryLand(vector<vector<Color>>& b, int x, int y, int dir) const {
    vector<pair<int, int>> locations = GetLocations(x, y, dir);
    for (int i = 0, e = locations.size(); i < e; i++) {
      const auto& p = locations[i];
      if (p.first < 0 || p.first >= r_ || p.second < 0 || p.second >= c_) {
        Reset(b, locations, i);
        return false;
      }
      if (b[p.first][p.second] != kWhite) {
        Reset(b, locations, i);
        return false;
      }
      b[p.first][p.second] = (i == 0 ? kRed : kBlue);
    }
    return true;
  }

  void Lift(vector<vector<Color>>& b, int x, int y, int dir) const {
    vector<pair<int, int>> locations = GetLocations(x, y, dir);
    for (const auto& p : locations) {
      b[p.first][p.second] = kWhite;
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

  static void Reset(vector<vector<Color>>& b,
                    const vector<pair<int, int>>& locations, const int e) {
    for (int i = 0; i < e; i++) {
      const auto& p = locations[i];
      b[p.first][p.second] = kWhite;
    }
  }

  static vector<pair<int, int>> GetLocations(const int x, const int y,
                                             const int dir) {
    vector<pair<int, int>> locations;
    auto AddLocation = [&locations, x, y, dir](int dx, int dy) {
      for (int k = 0; k < dir; k++) {
        // Rotate (dx, dy) by 90 degrees.
        int orig_dx = dx;
        dx = -dy;
        dy = orig_dx;
      }
      locations.push_back({x + dx, y + dy});
    };
    AddLocation(0, 0);
    AddLocation(1, -2);
    AddLocation(1, -1);
    AddLocation(1, 0);
    AddLocation(1, 1);
    AddLocation(1, 2);
    AddLocation(2, 0);
    AddLocation(3, -1);
    AddLocation(3, 0);
    AddLocation(3, 1);
    return locations;
  }

  const int r_;
  const int c_;
  vector<vector<Color>> board_;
};

int main() {
  Solution s(10, 10);
  char op;
  int x;
  int y;
  char color;
  s.PrintProbabilityMatrix();
  while (cin >> x >> y >> color) {
    s.SetColor(x, y, static_cast<Color>(color));
    s.PrintProbabilityMatrix();
  }

  return 0;
}