#include <unistd.h>

#include <iostream>

#include "aircraft_placer.h"
#include "color.h"

using namespace std;

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

  vector<vector<Color>> board(rows, vector<Color>(cols, kGray));
  vector<vector<bool>> occupied(rows, vector<bool>(cols, false));

  AircraftPlacer placer(board);
  srand(time(nullptr));
  for (int i = 0; i < num_aircrafts; i++) {
    while (true) {
      int x = rand() % rows;
      int y = rand() % cols;
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

  printf("    ");
  for (int y = 0; y < cols; y++) {
    printf("%2c", 'A' + y);
  }
  printf("\n");
  for (int x = 0; x < rows; x++) {
    printf("%2d: ", x + 1);
    for (int y = 0; y < cols; y++) {
      if (board[x][y] == kGray) {
        printf("  ");
      } else {
        printf("\033[0;%dm", (board[x][y] == kRed ? 31 : 34));
        printf("AA");
        printf("\33[0m");
      }
    }
    printf("\n");
  }

  return 0;
}