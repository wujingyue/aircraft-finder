#include <unistd.h>

#include <iostream>
#include <sstream>
#include <tuple>

#include "aircraft_finder.h"

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
