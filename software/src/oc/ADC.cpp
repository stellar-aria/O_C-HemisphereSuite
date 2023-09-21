#include "oc/ADC.h"
#include "oc/gpio.h"
#include "oc/hw.h"

#include <algorithm>

namespace oc {

/*static*/ size_t ADC::scan_channel_;
/*static*/ ADC::CalibrationData *ADC::calibration_data_;
/*static*/ uint32_t ADC::raw_[ADC::kNumAdcChannels];
/*static*/ uint32_t ADC::smoothed_[ADC::kNumAdcChannels];


/*static*/ void ADC::Init(CalibrationData *calibration_data) {
  calibration_data_ = calibration_data;
  std::fill(raw_, raw_ + ADC::kNumAdcChannels, 0);
  std::fill(smoothed_, smoothed_ + ADC::kNumAdcChannels, 0);
}

/*static*/ void ADC::Scan() {
  const size_t channel = scan_channel_;
  const uint16_t value = hw.controls[channel].GetRawValue() >> 4; // TODO: overhaul ADC to use floats from libdaisy
  update(channel, value);
  scan_channel_ = (channel + 1) % kNumAdcChannels;
}

/*static*/ void ADC::CalibratePitch(int32_t c2, int32_t c4) {
  // This is the method used by the Mutable Instruments calibration and
  // extrapolates from two octaves. I guess an alternative would be to get the
  // lowest (-3v) and highest (+6v) and interpolate between them
  // *vague handwaving*
  if (c2 < c4) {
    int32_t scale = (24 * 128 * 4096L) / (c4 - c2);
    calibration_data_->pitch_cv_scale = scale;
  }
}

}; // namespace oc
