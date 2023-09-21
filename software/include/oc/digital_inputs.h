#ifndef OC_DIGITAL_INPUTS_H_
#define OC_DIGITAL_INPUTS_H_

#include <stdint.h>
#include "oc/config.h"
#include "oc/core.h"
#include "oc/gpio.h"
#include "oc/hw.h"

#include "sky/bit_ops.h"

namespace oc {

static constexpr uint32_t DIGITAL_INPUT_MASK(size_t x) { return 0x1 << x; }

constexpr size_t kNumDigitalInputs = 4;

class DigitalInputs {
public:
  // @return mask of all pins clocked since last call, reset state
  static inline uint32_t clocked() {
    uint32_t out = 0;
    for (uint32_t i = 0; i < hw.GATE_IN_LAST; i++) {
      if (hw.gate_input[i].Trig()) {
        bit::Set(out, i);
      }
    }
    return out;
  }

  // @return mask if pin clocked since last call, reset state
  static inline uint32_t clocked(size_t input) {
    if (input < hw.GATE_IN_LAST) {
      return hw.gate_input[input].Trig();
    } else {
      return false;
    }    
  }

  static inline bool read_immediate(size_t input) {
    if (input < hw.GATE_IN_LAST) {
      return hw.gate_input[input].State();
    } else {
      return false;
    }
  }
};

// Helper class for visualizing digital inputs with decay
// Uses 4 bits for decay
class DigitalInputDisplay {
public:
  static constexpr uint32_t kDisplayTime = OC_CORE_ISR_FREQ / 8;
  static constexpr uint32_t kPhaseInc = (0xf << 28) / kDisplayTime;

  void Init() {
    phase_ = 0;
  }

  void Update(uint32_t ticks, bool clocked) {
    uint32_t phase_inc = ticks * kPhaseInc;
    if (clocked) {
      phase_ = 0xffffffff;
    } else {
      uint32_t phase = phase_;
      if (phase) {
        if (phase < phase_inc)
          phase_ = 0;
        else
          phase_ = phase - phase_inc;
      }
    }
  }

  uint8_t getState() const {
    return phase_ >> 28;
  }

private:
  uint32_t phase_;
};

}

#endif // OC_DIGITAL_INPUTS_H_
