#ifndef __AIRCRAFT_PLACER_H
#define __AIRCRAFT_PLACER_H

#include <vector>

#include "color.h"

class AircraftPlacer {
 public:
  AircraftPlacer(const std::vector<std::vector<Color>>& board);

  bool TryLand(int x, int y, int dir, std::vector<std::vector<bool>>* occupied,
               std::vector<std::pair<int, int>>* placed) const;

  void Lift(std::vector<std::vector<bool>>* occupied,
            std::vector<std::pair<int, int>>* placed) const;

  int AircraftSize() const { return aircraft_bodies_[0].size(); }

  const std::vector<std::pair<int, int>>& GetAircraftBody(int dir) const {
    return aircraft_bodies_[dir];
  }

 private:
  const std::vector<std::vector<Color>>& board_;

  std::vector<std::vector<std::pair<int, int>>> aircraft_bodies_;
};

#endif