#ifndef __AIRCRAFT_FINDER_H
#define __AIRCRAFT_FINDER_H

#include <vector>

#include "color.h"

struct Frequency {
  int red = 0;
  int blue = 0;
  int white = 0;
};

class Probability {
 public:
  explicit Probability(const Frequency& freq);

  double Entropy() const;
  double Red() const { return red_; }
  double Blue() const { return blue_; }
  double White() const { return white_; }

 private:
  void Normalize();

  double red_;
  double blue_;
  double white_;
};

class AircraftFinder {
 public:
  AircraftFinder(int r, int c, int num_aircrafts);

  void SetColor(int x, int y, Color color) { board_[x][y] = color; }

  std::pair<int, int> GetCellToBomb(const bool print_entropy_matrix) const;

 private:
  void PrintCell(const Probability& p, const bool is_top,
                 const bool is_known) const;

  const int r_;
  const int c_;
  const int num_aircrafts_;
  std::vector<std::vector<Color>> board_;
};

#endif