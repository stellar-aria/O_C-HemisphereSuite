#ifndef OC_CORE_H_
#define OC_CORE_H_

#include <stdint.h>

// TODO: Eliminate the need for these
using byte = uint8_t;
constexpr auto HIGH = 0x01;
constexpr auto LOW = 0x00;
#define constrain(amt,low,high) ((amt)<(low)?(low):((amt)>(high)?(high):(amt)))

#include "drivers/display.h"
#include "oc/config.h"
#include "util/debugpins.h"


namespace oc {
namespace core {  
extern volatile uint32_t ticks;
extern volatile bool app_isr_enabled;

};  // namespace core

struct TickCount {
  TickCount() {}
  void Init() { last_ticks = 0; }

  uint32_t Update() {
    uint32_t now = oc::core::ticks;
    uint32_t ticks = now - last_ticks;
    last_ticks = now;
    return ticks;
  }

  uint32_t last_ticks;
};
};  // namespace oc

#endif  // OC_CORE_H_
