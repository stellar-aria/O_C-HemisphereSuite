#ifndef OC_DEBUG_H_
#define OC_DEBUG_H_

#include "oc/gpio.h"
#include "util/math.h"
#include "util/macros.h"
#include "util/profiling.h"
#include "per/gpio.h"

namespace oc {

namespace debug {

  void Init();

  extern debug::AveragedCycles ISR_cycles;
  extern debug::AveragedCycles UI_cycles;
  extern debug::AveragedCycles MENU_draw_cycles;

  extern uint32_t UI_event_count;
  extern uint32_t UI_max_queue_depth;
  extern uint32_t UI_queue_overflow;
};

class DebugPins {
  daisy::GPIO pin1;
  daisy::GPIO pin2;

  void Init() {
    using namespace daisy;
    pin1.Init(OC_GPIO_DEBUG_PIN1, GPIO::Mode::OUTPUT, GPIO::Pull::NOPULL, GPIO::Speed::VERY_HIGH);
    pin2.Init(OC_GPIO_DEBUG_PIN2, GPIO::Mode::OUTPUT, GPIO::Pull::NOPULL, GPIO::Speed::VERY_HIGH);
  }
};

}; // namespace oc


#define OC_DEBUG_PROFILE_SCOPE(var) \
  debug::ScopedCycleMeasurement cycles(var)

#define OC_DEBUG_RESET_CYCLES(counter, count, var) \
  do { \
    if (!((counter) & (count - 1))) \
      var.Reset(); \
  } while (0)

#endif // OC_DEBUG_H