#pragma once
#include <cstdint>
#include <limits>
#include "per/rng.h"

inline uint32_t random(uint32_t min, uint32_t max) {
  const uint32_t raw = daisy::Random::GetValue();
  return min + (raw * (max - min)) / std::numeric_limits<uint32_t>::max();
}

inline uint32_t random(uint32_t max) { return random(0, max); }

#define randomSeed(x) ;