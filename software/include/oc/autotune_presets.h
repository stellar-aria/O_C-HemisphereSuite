#ifndef OC_AUTOTUNE_PRESETS_H_
#define OC_AUTOTUNE_PRESETS_H_


#include "oc/config.h"

namespace oc {

  struct Autotune_data {
 
    uint8_t use_auto_calibration_;
    uint16_t auto_calibrated_octaves[OCTAVES + 1];
  };

  const Autotune_data autotune_data_default[] = {
    // default
    { 0, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }
  };
}
#endif // OC_AUTOTUNE_PRESETS_H_