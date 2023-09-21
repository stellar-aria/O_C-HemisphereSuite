#ifndef OC_ADC_H_
#define OC_ADC_H_

#include "oc/config.h"

#include <stdint.h>
#include <string.h>

//#define ENABLE_ADC_DEBUG

namespace oc {

class ADC {
public:
  static constexpr uint8_t kNumAdcChannels = 4;
  static constexpr uint8_t kAdcResolution = 12;
  static constexpr uint32_t kAdcSmoothing = 4;
  static constexpr uint32_t kAdcSmoothBits = 8; // fractional bits for smoothing
  static constexpr uint16_t kDefaultPitchCVScale = SEMITONES << 7;

  // These values should be tweaked so startSingleRead/readSingle run in main ISR update time
  // 16 bit has best-case 13 bits useable, but we only want 12 so we discard 4 anyway
  static constexpr uint8_t kAdcScanResolution = 16;
  static constexpr uint8_t kAdcScanAverages = 16;

  static constexpr uint32_t kAdcValueShift = kAdcSmoothBits;


  struct CalibrationData {
    uint16_t offset[ADC::kNumAdcChannels];
    uint16_t pitch_cv_scale;
    int16_t pitch_cv_offset;
  };

  static void Init(CalibrationData *calibration_data);

  // Read the value of the last conversion and update current channel, then
  // start the next conversion. If necessary, some channels could be given
  // priority by scanning them more often. Even better might be some kind of
  // continuous/DMA sampling to make things even more independent of the main
  // ISR timing restrictions.
  static void Scan();

  static int32_t value(size_t channel) {
    return calibration_data_->offset[channel] - (smoothed_[channel] >> kAdcValueShift);
  }

  static uint32_t raw_value(size_t channel) {
    return raw_[channel] >> kAdcValueShift;
  }

  static uint32_t smoothed_raw_value(size_t channel) {
    return smoothed_[channel] >> kAdcValueShift;
  }

  static int32_t pitch_value(size_t channel) {
    return (value(channel) * calibration_data_->pitch_cv_scale) >> 12;
  }

  static int32_t raw_pitch_value(size_t channel) {
    int32_t value = calibration_data_->offset[channel] - raw_value(channel);
    return (value * calibration_data_->pitch_cv_scale) >> 12;
  }

  static void CalibratePitch(int32_t c2, int32_t c4);

private:
  static void update(size_t channel, uint32_t value) {
    value = (value  >> (kAdcScanResolution - kAdcResolution)) << kAdcSmoothBits;
    raw_[channel] = value;
    // division should be shift if kAdcSmoothing is power-of-two
    value = (smoothed_[channel] * (kAdcSmoothing - 1) + value) / kAdcSmoothing;
    smoothed_[channel] = value;
  }

  static size_t scan_channel_;
  static CalibrationData *calibration_data_;

  static uint32_t raw_[ADC::kNumAdcChannels];
  static uint32_t smoothed_[ADC::kNumAdcChannels];
};

};

#endif // OC_ADC_H_
