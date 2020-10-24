#include "aircraft_finder.h"
#include "aircraft_generator.h"
#include "benchmark/benchmark.h"
#include "color.h"

using namespace std;

void BM_Finder(benchmark::State& state) {
  const int rows = state.range(0);
  const int cols = state.range(1);
  const int num_aircrafts = state.range(2);

  srand(1229);
  AircraftGenerator generator(rows, cols, num_aircrafts);
  vector<vector<Color>> board = generator.Generate();

  for (auto _ : state) {
    AircraftFinder finder(rows, cols, num_aircrafts);
    int num_remaining_aircrafts = num_aircrafts;
    while (num_remaining_aircrafts > 0) {
      int x;
      int y;
      tie(x, y) = finder.GetCellToBomb(false);
      finder.SetColor(x, y, board[x][y]);
      if (board[x][y] == kRed) {
        num_remaining_aircrafts--;
      }
    }
  }
}

BENCHMARK(BM_Finder)
    ->Args({10, 10, 2})
    ->Args({15, 12, 3})
    ->Args({18, 15, 3})
    ->Unit(benchmark::kMillisecond);