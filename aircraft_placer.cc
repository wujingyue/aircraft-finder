#include "aircraft_placer.h"

using namespace std;

AircraftPlacer::AircraftPlacer(const vector<vector<Color>>& board)
    : board_(board) {
  aircraft_bodies_.resize(4);
  aircraft_bodies_[0] = {{0, 0}, {1, -2}, {1, -1}, {1, 0}, {1, 1},
                         {1, 2}, {2, 0},  {3, -1}, {3, 0}, {3, 1}};
  for (int k = 1; k < 4; k++) {
    for (const pair<int, int>& body : aircraft_bodies_[k - 1]) {
      aircraft_bodies_[k].push_back({-body.second, body.first});
    }
  }
}

bool AircraftPlacer::TryLand(int x, int y, int dir,
                             vector<vector<bool>>* occupied,
                             vector<pair<int, int>>* placed) const {
  const int r = board_.size();
  const int c = board_[0].size();
  for (const pair<int, int>& body : aircraft_bodies_[dir]) {
    int dx = body.first;
    int dy = body.second;
    int x2 = x + dx;
    int y2 = y + dy;
    if (x2 < 0 || x2 >= r || y2 < 0 || y2 >= c) {
      return false;
    }
    if ((*occupied)[x2][y2]) {
      return false;
    }
    Color new_color = (dx == 0 && dy == 0 ? kRed : kBlue);
    if (board_[x2][y2] != kGray) {
      if (board_[x2][y2] != new_color) {
        return false;
      }
    }
    (*occupied)[x2][y2] = true;
    placed->push_back({x2, y2});
  };
  return true;
}

void AircraftPlacer::Lift(vector<vector<bool>>* occupied,
                          vector<pair<int, int>>* placed) const {
  while (!placed->empty()) {
    int x = placed->back().first;
    int y = placed->back().second;
    placed->pop_back();
    (*occupied)[x][y] = false;
  }
}
