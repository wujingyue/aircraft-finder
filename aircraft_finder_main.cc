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

  int num_remaining_aircrafts = num_aircrafts;
  while (true) {
    int x;
    int y;
    tie(x, y) = finder.GetCellToBomb(true);
    if (num_remaining_aircrafts <= 0) {
      break;
    }

    printf("(%d, %c) > ", x + 1, 'A' + y);

    string line;
    if (!getline(cin, line)) {
      break;
    }

    char char_c;
    istringstream iss(line);
    iss >> char_c;
    // If the line contains a color only, reuse the cell to bomb.
    if (isdigit(char_c)) {
      iss.str(line);

      char char_y;
      iss >> x >> char_y >> char_c;
      x--;
      y = char_y - (isupper(char_y) ? 'A' : 'a');
    }

    Color c = static_cast<Color>(char_c);
    finder.SetColor(x, y, c);
    if (c == kRed) {
      num_remaining_aircrafts--;
    }
  }

  return 0;
}
