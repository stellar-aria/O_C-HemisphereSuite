#ifndef UTIL_DEBUGPINS_H_
#define UTIL_DEBUGPINS_H_
#include "per/gpio.h"

namespace util {

struct scoped_debug_pin {
  scoped_debug_pin(daisy::GPIO &debug_pin) : gpio_(debug_pin) {
    gpio_.Write(true);
  }

  ~scoped_debug_pin() {
    gpio_.Write(false);
  }
private:
  daisy::GPIO &gpio_;
};

}; // namespace util

//#define ENABLE_DEBUG_PINS

#ifdef ENABLE_DEBUG_PINS
#define DEBUG_PIN_SCOPE(pin) \
  util::scoped_debug_pin<pin> debug_pin
#else
#define DEBUG_PIN_SCOPE(pin) \
  do {} while (0)
#endif

#endif // UTIL_DEBUGPINS_H_
