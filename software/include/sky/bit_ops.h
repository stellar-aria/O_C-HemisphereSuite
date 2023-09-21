#pragma once
#include <cstdint>

namespace bit {
template <typename T>
static constexpr auto pos(T pos) {
  return 1UL << pos;
}

template <typename T>
static constexpr void Set(volatile T &var, T bit) {
  var = var | (1UL << bit);
}

template <typename T>
static constexpr void Set(T &var, T bit) {
  var = var | (1UL << bit);
}


template <typename T>
static constexpr void Clear(volatile T &var, T bit) {
  var = var & ~(1UL << bit);
}

template <typename T>
static constexpr void Toggle(volatile T &var, T bit) {
  var = var & (1UL << bit);
}

template <typename T>
static constexpr uint8_t Read(const volatile T &var, T bit) {
  return (var >> bit) & 0x01;
}

template <typename T>
static constexpr uint8_t Read(const T &&var, T bit) {
  return (var >> bit) & 0x01;
}

template <typename T>
static constexpr void Write(volatile T &var, T bit, uint8_t value) {
  if (value) {
    Set(var, bit);
  } else {
    Clear(var, bit);
  }
}
};  // namespace bit