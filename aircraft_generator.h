#ifndef __AIRCRAFT_GENERATOR_H
#define __AIRCRAFT_GENERATOR_H

#include <vector>

#include "color.h"

class AircraftGenerator {
 public:
  AircraftGenerator(int rows, int cols, int num_aircrafts);

  std::vector<std::vector<Color>> Generate() const;

 private:
  const int r_;
  const int c_;
  const int num_aircrafts_;
};

#endif