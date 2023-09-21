#ifndef OC_AUTOTUNE_H_
#define OC_AUTOTUNE_H_

#include "oc/autotune_presets.h"
#include "oc/DAC.h"

namespace oc {

typedef oc::Autotune_data Autotune_data;

class AUTOTUNE {
public:
  static const int NUM_DAC_CHANNELS;
  static void Init();
  static const Autotune_data &GetAutotune_data(int channel);
};

extern Autotune_data auto_calibration_data[oc::kNumDacChannels];
};

#endif // OC_AUTOTUNE_H_