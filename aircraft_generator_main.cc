#include <unistd.h>

#include <iostream>

#include "aircraft_generator.h"

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

  srand(time(nullptr));
  AircraftGenerator generator(rows, cols, num_aircrafts);
  vector<vector<Color>> board = generator.Generate();

  printf("    ");
  for (int y = 0; y < cols; y++) {
    printf("%2c", 'A' + y);
  }
  printf("\n");
  for (int x = 0; x < rows; x++) {
    printf("%2d: ", x + 1);
    for (int y = 0; y < cols; y++) {
      int color_code = 30;
      int style_code = 0;
      if (board[x][y] == kRed) {
        color_code = 31;
        style_code = 1;
      } else if (board[x][y] == kBlue) {
        color_code = 34;
      }
      printf("\033[%d;%dm", style_code, color_code);
      printf(" A");
      printf("\33[0m");
    }
    printf("\n");
  }

  return 0;
}
