#include "aircraft_generator.h"

#include <cstdlib>
#include <ctime>

#include "aircraft_placer.h"
#include "color.h"

using namespace std;

AircraftGenerator::AircraftGenerator(int rows, int cols, int num_aircrafts)
    : r_(rows), c_(cols), num_aircrafts_(num_aircrafts) {}

vector<vector<Color>> AircraftGenerator::Generate() const {
  vector<vector<Color>> board(r_, vector<Color>(c_, kGray));
  vector<vector<bool>> occupied(r_, vector<bool>(c_, false));

  AircraftPlacer placer(board);
  srand(time(nullptr));
  for (int i = 0; i < num_aircrafts_; i++) {
    while (true) {
      int x = rand() % r_;
      int y = rand() % c_;
      int dir = rand() % 4;
      vector<pair<int, int>> placed;
      if (placer.TryLand(x, y, dir, &occupied, &placed)) {
        for (const pair<int, int>& body : placed) {
          board[body.first][body.second] =
              (body.first == x && body.second == y ? kRed : kBlue);
        }
        break;
      }
      placer.Lift(&occupied, &placed);
    }
  }

  for (int x = 0; x < r_; x++) {
    for (int y = 0; y < c_; y++) {
      if (board[x][y] == kGray) {
        board[x][y] = kWhite;
      }
    }
  }

  return board;
}