#include "oc/autotune.h"
#include "oc/autotune_presets.h"

namespace oc {

    Autotune_data auto_calibration_data[DAC_CHANNEL_LAST];

    /*static*/
    const int AUTOTUNE::NUM_DAC_CHANNELS = DAC_CHANNEL_LAST;

    /*static*/
    void AUTOTUNE::Init() {
      for (size_t i = 0; i < DAC_CHANNEL_LAST; i++)
        memcpy(&auto_calibration_data[i], &oc::autotune_data_default[0], sizeof(Autotune_data));
    }
    /*static*/
    const Autotune_data &AUTOTUNE::GetAutotune_data(int channel) {
        return auto_calibration_data[channel];
    }
} // namespace oc