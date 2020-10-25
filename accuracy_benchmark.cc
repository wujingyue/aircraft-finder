#include <unistd.h>

#include <iostream>
#include <tuple>

#include "aircraft_finder.h"
#include "aircraft_generator.h"
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

  AircraftGenerator generator(rows, cols, num_aircrafts);
  vector<vector<Color>> board = generator.Generate();

  AircraftFinder finder(rows, cols, num_aircrafts);
  int num_remaining_aircrafts = num_aircrafts;
  int num_guesses = 0;
  while (num_remaining_aircrafts > 0) {
    num_guesses++;

    int x;
    int y;
    tie(x, y) = finder.GetCellToBomb(false);
    cerr << x << ' ' << y << ' ' << board[x][y] << endl;
    finder.SetColor(x, y, board[x][y]);
    if (board[x][y] == kRed) {
      num_remaining_aircrafts--;
    }
  }

  cout << "Number of guesses = " << num_guesses << endl;

  return 0;
}