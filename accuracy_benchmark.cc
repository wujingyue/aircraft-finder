#include <unistd.h>

#include <iostream>
#include <numeric>
#include <tuple>

#include "aircraft_finder.h"
#include "aircraft_generator.h"
#include "color.h"

using namespace std;

void PrintUsage(const char* exec_name) {
  cerr << "Usage: " << exec_name << " -r rows -c cols -n aircrafts" << endl;
}

class Histogram {
 public:
  void AddNumGuesses(const int num_guesses) {
    if (num_guesses >= static_cast<int>(num_games_.size())) {
      num_games_.resize(num_guesses + 1);
    }
    num_games_[num_guesses]++;
  }

  void Print() const {
    cout << "Average = " << Average() << endl;
    cout << "Median = " << Median() << endl;
    cout << "Worst = " << Worst() << endl;
    for (int num_guesses = 0, size = num_games_.size(); num_guesses < size;
         num_guesses++) {
      printf("%3d: ", num_guesses);
      cout << string(num_games_[num_guesses], '#') << endl;
    }
  }

 private:
  float Average() const {
    int count = accumulate(num_games_.begin(), num_games_.end(), 0);
    int sum = 0;
    for (int num_guesses = 0, size = num_games_.size(); num_guesses < size;
         num_guesses++) {
      sum += num_guesses * num_games_[num_guesses];
    }
    return static_cast<float>(sum) / count;
  }

  int Median() const {
    int count = accumulate(num_games_.begin(), num_games_.end(), 0);
    int sum = 0;
    for (int num_guesses = 0, size = num_games_.size(); num_guesses < size;
         num_guesses++) {
      sum += num_games_[num_guesses];
      if (sum >= count / 2) {
        return num_guesses;
      }
    }
    return -1;
  }

  int Worst() const { return num_games_.size() - 1; }

  vector<int> num_games_;
};

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
  srand(1229);

  Histogram histogram;
  for (int i = 0; i < 100; i++) {
    vector<vector<Color>> board = generator.Generate();

    AircraftFinder finder(rows, cols, num_aircrafts);
    int num_remaining_aircrafts = num_aircrafts;
    int num_guesses = 0;
    while (num_remaining_aircrafts > 0) {
      num_guesses++;

      int x;
      int y;
      tie(x, y) = finder.GetCellToBomb(false);
      finder.SetColor(x, y, board[x][y]);
      if (board[x][y] == kRed) {
        num_remaining_aircrafts--;
      }
    }

    histogram.AddNumGuesses(num_guesses);
  }

  histogram.Print();

  return 0;
}