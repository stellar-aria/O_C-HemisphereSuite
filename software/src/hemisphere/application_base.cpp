#include "hemisphere/application_base.hpp"
#include "hemisphere/clock_manager.hpp"
#include "hemisphere/icons.hpp"
#include "oc/ADC.h"
#include "oc/DAC.h"
#include "oc/digital_inputs.h"

using namespace hemisphere;

#ifndef int2simfloat
#define int2simfloat(x) (x << 14)
#define simfloat2int(x) (x >> 14)
typedef int32_t simfloat;
#endif

void ApplicationBase::BaseController() {
  for (uint8_t ch = 0; ch < 4; ch++) {
    // Set ADC input values
    inputs[ch] = oc::ADC::raw_pitch_value((ADC_CHANNEL)ch);
    if (abs(inputs[ch] - last_cv[ch]) > CHANGE_THRESHOLD) {
      changed_cv[ch] = 1;
      last_cv[ch] = inputs[ch];
    } else
      changed_cv[ch] = 0;

    if (clock_countdown[ch] > 0) {
      if (--clock_countdown[ch] == 0) Out(ch, 0);
    }
  }

  // Cursor countdowns. See CursorBlink(), ResetCursor(), gfxCursor()
  if (--cursor_countdown < -CURSOR_TICKS)
    cursor_countdown = CURSOR_TICKS;

  Controller();
}

void ApplicationBase::BaseStart() {
  // Initialize some things for startup
  for (uint8_t ch = 0; ch < 4; ch++) {
    clock_countdown[ch] = 0;
    adc_lag_countdown[ch] = 0;
  }
  cursor_countdown = CURSOR_TICKS;

  Start();
}

void ApplicationBase::BaseView() {
  View();
  last_view_tick = oc::core::ticks;
}

int ApplicationBase::Proportion(int numerator, int denominator, int max_value) {
  simfloat proportion = int2simfloat((int32_t)numerator) / (int32_t)denominator;
  int scaled = simfloat2int(proportion * max_value);
  return scaled;
}

//////////////// Hemisphere-like IO methods
////////////////////////////////////////////////////////////////////////////////
void ApplicationBase::Out(int ch, int value, int octave) {
  oc::DAC::set_pitch((DAC_CHANNEL)ch, value, octave);
  outputs[ch] = value + (octave * (12 << 7));
}

int ApplicationBase::In(int ch) { return inputs[ch]; }

// Apply small center detent to input, so it reads zero before a threshold
int ApplicationBase::DetentedIn(int ch) {
  return (In(ch) > 64 || In(ch) < -64) ? In(ch) : 0;
}

bool ApplicationBase::Changed(int ch) { return changed_cv[ch]; }

bool ApplicationBase::Gate(int ch) {
  bool high = 0;
  if (ch == 0) high = oc::DigitalInputs::read_immediate<oc::DIGITAL_INPUT_1>();
  if (ch == 1) high = oc::DigitalInputs::read_immediate<oc::DIGITAL_INPUT_2>();
  if (ch == 2) high = oc::DigitalInputs::read_immediate<oc::DIGITAL_INPUT_3>();
  if (ch == 3) high = oc::DigitalInputs::read_immediate<oc::DIGITAL_INPUT_4>();
  return high;
}

void ApplicationBase::GateOut(int ch, bool high) {
  Out(ch, 0, (high ? PULSE_VOLTAGE : 0));
}

bool ApplicationBase::Clock(int ch) {
  bool clocked = 0;
  ClockManager *clock_m = clock_m->get();

  if (clock_m->IsRunning() && clock_m->GetMultiply(ch) != 0)
    clocked = clock_m->Tock(ch);
  else {
    if (ch == 0) clocked = oc::DigitalInputs::clocked<oc::DIGITAL_INPUT_1>();
    if (ch == 1) clocked = oc::DigitalInputs::clocked<oc::DIGITAL_INPUT_2>();
    if (ch == 2) clocked = oc::DigitalInputs::clocked<oc::DIGITAL_INPUT_3>();
    if (ch == 3) clocked = oc::DigitalInputs::clocked<oc::DIGITAL_INPUT_4>();
  }

  // manual triggers
  clocked = clocked || clock_m->Beep(ch);

  if (clocked) {
    cycle_ticks[ch] = oc::core::ticks - last_clock[ch];
    last_clock[ch] = oc::core::ticks;
  }
  return clocked;
}

void ApplicationBase::ClockOut(int ch, int ticks) {
  clock_countdown[ch] = ticks;
  Out(ch, 0, PULSE_VOLTAGE);
}

// Buffered I/O functions for use in Views
int ApplicationBase::ViewIn(int ch) { return inputs[ch]; }
int ApplicationBase::ViewOut(int ch) { return outputs[ch]; }
int ApplicationBase::ClockCycleTicks(int ch) { return cycle_ticks[ch]; }

/* ADC Lag: There is a small delay between when a digital input can be read
 * and when an ADC can be read. The ADC value lags behind a bit in time. So
 * StartADCLag() and EndADCLag() are used to determine when an ADC can be
 * read. The pattern goes like this
 *
 * if (Clock(ch)) StartADCLag(ch);
 *
 * if (EndOfADCLog(ch)) {
 *     int cv = In(ch);
 *     // etc...
 * }
 */
void ApplicationBase::StartADCLag(int ch) { adc_lag_countdown[ch] = 96; }
bool ApplicationBase::EndOfADCLag(int ch) { return (--adc_lag_countdown[ch] == 0); }

//////////////// Hemisphere-like graphics methods for easy porting
////////////////////////////////////////////////////////////////////////////////
void ApplicationBase::gfxCursor(int x, int y, int w) {
  if (CursorBlink()) gfxLine(x, y, x + w - 1, y);
}

void ApplicationBase::gfxPos(int x, int y) { graphics.setPrintPos(x, y); }

void ApplicationBase::gfxPrint(int x, int y, const char *str) {
  graphics.setPrintPos(x, y);
  graphics.print(str);
}

void ApplicationBase::gfxPrint(int x, int y, int num) {
  graphics.setPrintPos(x, y);
  graphics.print(num);
}

void ApplicationBase::gfxPrint(int x_adv,
                           int num) {  // Print number with character padding
  for (int c = 0; c < (x_adv / 6); c++) gfxPrint(" ");
  gfxPrint(num);
}

void ApplicationBase::gfxPrint(const char *str) { graphics.print(str); }

void ApplicationBase::gfxPrint(int num) { graphics.print(num); }

void ApplicationBase::gfxPixel(int x, int y) { graphics.setPixel(x, y); }

void ApplicationBase::gfxFrame(int x, int y, int w, int h) {
  graphics.drawFrame(x, y, w, h);
}

void ApplicationBase::gfxRect(int x, int y, int w, int h) {
  graphics.drawRect(x, y, w, h);
}

void ApplicationBase::gfxInvert(int x, int y, int w, int h) {
  graphics.invertRect(x, y, w, h);
}

void ApplicationBase::gfxLine(int x, int y, int x2, int y2) {
  graphics.drawLine(x, y, x2, y2);
}

void ApplicationBase::gfxDottedLine(int x, int y, int x2, int y2, uint8_t p) {
#ifdef HS_GFX_MOD
  graphics.drawLine(x, y, x2, y2, p);
#else
  graphics.drawLine(x, y, x2, y2);
#endif
}

void ApplicationBase::gfxCircle(int x, int y, int r) {
  graphics.drawCircle(x, y, r);
}

void ApplicationBase::gfxBitmap(int x, int y, int w, const uint8_t *data) {
  graphics.drawBitmap8(x, y, w, data);
}

// Like gfxBitmap, but always 8x8
void ApplicationBase::gfxIcon(int x, int y, const uint8_t *data) {
  gfxBitmap(x, y, 8, data);
}

uint8_t ApplicationBase::pad(int range, int number) {
  uint8_t padding = 0;
  while (range > 1) {
    if (abs(number) < range) padding += 6;
    range = range / 10;
  }
  if (number < 0 && padding > 0) padding -= 6;  // Compensate for minus sign
  return padding;
}

void ApplicationBase::gfxHeader(const char *str) {
  gfxPrint(1, 2, str);
  gfxLine(0, 10, 127, 10);
  gfxLine(0, 12, 127, 12);
}

int ApplicationBase::ProportionCV(int cv_value, int max_pixels) {
  int prop = constrain(Proportion(cv_value, FIVE_VOLTS, max_pixels),
                       -max_pixels, max_pixels);
  return prop;
}

// Check cursor blink cycle
bool ApplicationBase::CursorBlink() { return (cursor_countdown > 0); }

void ApplicationBase::ResetCursor() { cursor_countdown = CURSOR_TICKS; }
