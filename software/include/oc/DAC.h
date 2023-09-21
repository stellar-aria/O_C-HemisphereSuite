#ifndef OC_DAC_H_
#define OC_DAC_H_

#include <stdint.h>
#include <string.h>
#include "oc/config.h"
#include "oc/options.h"
#include "util/math.h"
#include "util/macros.h"

extern void set8565_CHA(uint32_t data);
extern void set8565_CHB(uint32_t data);
extern void set8565_CHC(uint32_t data);
extern void set8565_CHD(uint32_t data);
extern void SPI_init();

enum OutputVoltageScaling {
  VOLTAGE_SCALING_1V_PER_OCT,    // 0
  VOLTAGE_SCALING_CARLOS_ALPHA,  // 1
  VOLTAGE_SCALING_CARLOS_BETA,   // 2
  VOLTAGE_SCALING_CARLOS_GAMMA,  // 3
  VOLTAGE_SCALING_BOHLEN_PIERCE, // 4
  VOLTAGE_SCALING_QUARTERTONE,   // 5
  #ifdef BUCHLA_SUPPORT
    VOLTAGE_SCALING_1_2V_PER_OCT,  // 6
    VOLTAGE_SCALING_2V_PER_OCT,    // 7
  #endif
  VOLTAGE_SCALING_LAST  
} ;

namespace oc {
constexpr size_t kNumDacChannels = 4;

class DAC {
public:
  static constexpr size_t kHistoryDepth = 8;
  static constexpr uint16_t MAX_VALUE = 65535; // DAC fullscale 

  #ifdef BUCHLA_4U
    static constexpr int kOctaveZero = 0;
  #elif defined(VOR) 
    static int kOctaveZero;
    static constexpr int VBiasUnipolar = 3900;   // onboard DAC @ Vref 1.2V (internal), 1.75x gain
    static constexpr int VBiasBipolar = 2000;    // onboard DAC @ Vref 1.2V (internal), 1.75x gain
    static constexpr int VBiasAsymmetric = 2760; // onboard DAC @ Vref 1.2V (internal), 1.75x gain
  #else
    static constexpr int kOctaveZero = 3;
  #endif

  struct CalibrationData {
    uint16_t calibrated_octaves[kNumDacChannels][OCTAVES + 1];
  };

  static void Init(CalibrationData *calibration_data);

  static uint8_t calibration_data_used(uint8_t channel_id);
  static void set_auto_channel_calibration_data(uint8_t channel_id);
  static void set_default_channel_calibration_data(uint8_t channel_id);
  static void update_auto_channel_calibration_data(uint8_t channel_id, int8_t octave, uint32_t pitch_data);
  static void reset_auto_channel_calibration_data(uint8_t channel_id);
  static void reset_all_auto_channel_calibration_data();
  static void choose_calibration_data();
  static void set_scaling(uint8_t scaling, uint8_t channel_id);
  static void restore_scaling(uint32_t scaling);
  static uint8_t get_voltage_scaling(uint8_t channel_id);
  static uint32_t store_scaling();
  static void set_Vbias(uint32_t data);
  static void init_Vbias();
  
  static void set_all(uint32_t value) {
    for (size_t i = 0; i < kNumDacChannels; ++i)
      values_[i] = USAT16(value);
  }

  static void set(size_t channel, uint32_t value) {
    values_[channel] = USAT16(value);
  }

  static uint32_t value(size_t index) {
    return values_[index];
  }

  // Calculate DAC value from semitone, where 0 = C1 = 0V, C2 = 12 = 1V
  // Expected semitone resolution is 12 bit.
  //
  // @return DAC output value
  static int32_t semitone_to_dac(size_t channel, int32_t semi, int32_t octave_offset) {
    return pitch_to_dac(channel, semi << 7, octave_offset);
  }


  // Calculate DAC value from pitch value with 12 * 128 bit per octave.
  // 0 = C1 = 0V, C2 = 24 << 7 = 1V etc. Automatically shifts for LUT range.
  //
  // @return DAC output value
  static int32_t pitch_to_dac(size_t channel, int32_t pitch, int32_t octave_offset) {
    pitch += (kOctaveZero + octave_offset) * 12 << 7;
    
    CONSTRAIN(pitch, 0, (120 << 7));

    const int32_t octave = pitch / (12 << 7);
    const int32_t fractional = pitch - octave * (12 << 7);

    int32_t sample = calibration_data_->calibrated_octaves[channel][octave];
    if (fractional) {
      int32_t span = calibration_data_->calibrated_octaves[channel][octave + 1] - sample;
      sample += (fractional * span) / (12 << 7);
    }

    return sample;
  }

  // Specialised versions with voltage scaling

  static int32_t semitone_to_scaled_voltage_dac(size_t channel, int32_t semi, int32_t octave_offset, uint8_t voltage_scaling) {
    return pitch_to_scaled_voltage_dac(channel, semi << 7, octave_offset, voltage_scaling);
  }
  
  static int32_t pitch_to_scaled_voltage_dac(size_t channel, int32_t pitch, int32_t octave_offset, uint8_t voltage_scaling) {
    pitch += (octave_offset * 12) << 7;

 
    switch (voltage_scaling) {
      case VOLTAGE_SCALING_1V_PER_OCT:    // 1V/oct
          // do nothing
          break;
      case VOLTAGE_SCALING_CARLOS_ALPHA:  // Wendy Carlos alpha scale - scale by 0.77995
          pitch = (pitch * 25548) >> 15;  // 2^15 * 0.77995 = 25547.571
          break;
      case VOLTAGE_SCALING_CARLOS_BETA:   // Wendy Carlos beta scale - scale by 0.63833 
          pitch = (pitch * 20917) >> 15;  // 2^15 * 0.63833 = 20916.776
          break;
      case VOLTAGE_SCALING_CARLOS_GAMMA:  // Wendy Carlos gamma scale - scale by 0.35099
          pitch = (pitch * 11501) >> 15;  // 2^15 * 0.35099 = 11501.2403
          break;
      case VOLTAGE_SCALING_BOHLEN_PIERCE: // Bohlen-Pierce macrotonal scale - scale by 1.585
          pitch = (pitch * 25969) >> 14;  // 2^14 * 1.585 = 25968.64
          break;
      case VOLTAGE_SCALING_QUARTERTONE:   // Quartertone scaling (just down-scales to 0.5V/oct)
          pitch = pitch >> 1;
          break;
      #ifdef BUCHLA_SUPPORT
      case VOLTAGE_SCALING_1_2V_PER_OCT:  // 1.2V/oct
          pitch = (pitch * 19661) >> 14;
          break;
      case VOLTAGE_SCALING_2V_PER_OCT:    // 2V/oct
          pitch = pitch << 1;
          break;
      #endif    
      default: 
          break;
    }

    pitch += (kOctaveZero * 12) << 7;
   
    CONSTRAIN(pitch, 0, (120 << 7));

    const int32_t octave = pitch / (12 << 7);
    const int32_t fractional = pitch - octave * (12 << 7);

    int32_t sample = calibration_data_->calibrated_octaves[channel][octave];
    if (fractional) {
      int32_t span = calibration_data_->calibrated_octaves[channel][octave + 1] - sample;
      sample += (fractional * span) / (12 << 7);
    }

    return sample;
  }

  // Set channel to semitone value
  static void set_voltage_scaled_semitone(size_t channel, int32_t semitone, int32_t octave_offset, uint8_t voltage_scaling) {
    set(channel, semitone_to_scaled_voltage_dac(channel, semitone, octave_offset, voltage_scaling));
  }

  // Set channel to pitch value
  static void set_pitch(size_t channel, int32_t pitch, int32_t octave_offset) {
    set(channel, pitch_to_dac(channel, pitch, octave_offset));
  }

  // Set integer voltage value, where 0 = 0V, 1 = 1V
  static void set_octave(size_t channel, int v) {
    set(channel, calibration_data_->calibrated_octaves[channel][kOctaveZero + v]);
  }

  // Set all channels to integer voltage value, where 0 = 0V, 1 = 1V
  static void set_all_octave(int v) {
    set_octave(0, v);
    set_octave(1, v);
    set_octave(2, v);
    set_octave(3, v);
  }

  static uint32_t get_zero_offset(size_t channel) {
    return calibration_data_->calibrated_octaves[channel][kOctaveZero];
  }

  static uint32_t get_octave_offset(size_t channel, int octave) {
    return calibration_data_->calibrated_octaves[channel][kOctaveZero + octave];
  }

  static void Update() {

    set8565_CHA(values_[0]);
    set8565_CHB(values_[1]);
    set8565_CHC(values_[2]);
    set8565_CHD(values_[3]);

    size_t tail = history_tail_;
    history_[0][tail] = values_[0];
    history_[1][tail] = values_[1];
    history_[2][tail] = values_[2];
    history_[3][tail] = values_[3];
    history_tail_ = (tail + 1) % kHistoryDepth;
  }

  template <size_t channel>
  static void getHistory(uint16_t *dst){
    size_t head = (history_tail_ + 1) % kHistoryDepth;

    size_t count = kHistoryDepth - head;
    const uint16_t *src = history_[channel] + head;
    while (count--)
      *dst++ = *src++;

    count = head;
    src = history_[channel];
    while (count--)
      *dst++ = *src++;
  }

private:
  static CalibrationData *calibration_data_;
  static uint32_t values_[oc::kNumDacChannels];
  static uint16_t history_[oc::kNumDacChannels][kHistoryDepth];
  static volatile size_t history_tail_;
  static uint8_t DAC_scaling[oc::kNumDacChannels];
};

}; // namespace oc

#endif // OC_DAC_H_
