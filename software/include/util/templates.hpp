#pragma once

template <typename T>
constexpr T clamp(T x, T lower, T upper) {
  if (x < lower) {
    return lower;
  } else if (x > upper) {
    return upper;
  } else {
    return x;
  }
}