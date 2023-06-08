// Copyright (c) 2018, Jason Justian
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

/*
 * HSAppIO.h
 *
 * HSAppIO is a base class for full O_C apps that are designed to work (or act)
 * like Hemisphere apps, for consistency in development, or ease of porting apps
 * or applets in either direction.
 */

#pragma once

#include <stdint.h>

namespace hemisphere {

class ApplicationBase {
 public:
#if defined(BUCHLA_4U) || defined(VOR)
  static constexpr auto PULSE_VOLTAGE = 8;
#else
  static constexpr auto PULSE_VOLTAGE = 5;
#endif

  static constexpr auto CURSOR_TICKS = 12000;
  static constexpr auto FIVE_VOLTS = 7680;
  static constexpr auto THREE_VOLTS = 4608;
  static constexpr auto CHANGE_THRESHOLD = 32;

  virtual void Start();
  virtual void Controller();
  virtual void View();
  virtual void Resume();

  void BaseController();

  void BaseStart();

  void BaseView();

  int Proportion(int numerator, int denominator, int max_value);

  //////////////// Hemisphere-like IO methods
  ////////////////////////////////////////////////////////////////////////////////
  void Out(int ch, int value, int octave = 0);

  int In(int ch);

  // Apply small center detent to input, so it reads zero before a threshold
  int DetentedIn(int ch);

  bool Changed(int ch);

  bool Gate(int ch);
  void GateOut(int ch, bool high);

  bool Clock(int ch);

  void ClockOut(int ch, int ticks = 100);

  // Buffered I/O functions for use in Views
  int ViewIn(int ch);
  int ViewOut(int ch);
  int ClockCycleTicks(int ch);

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
  void StartADCLag(int ch);
  bool EndOfADCLag(int ch);

  //////////////// Hemisphere-like graphics methods for easy porting
  ////////////////////////////////////////////////////////////////////////////////
  void gfxCursor(int x, int y, int w);

  void gfxPos(int x, int y);

  void gfxPrint(int x, int y, const char *str);

  void gfxPrint(int x, int y, int num);

  void gfxPrint(int x_adv, int num);

  void gfxPrint(const char *str);

  void gfxPrint(int num);

  void gfxPixel(int x, int y);

  void gfxFrame(int x, int y, int w, int h);

  void gfxRect(int x, int y, int w, int h);

  void gfxInvert(int x, int y, int w, int h);

  void gfxLine(int x, int y, int x2, int y2);

  void gfxDottedLine(int x, int y, int x2, int y2, uint8_t p = 2);

  void gfxCircle(int x, int y, int r);

  void gfxBitmap(int x, int y, int w, const uint8_t *data);

  // Like gfxBitmap, but always 8x8
  void gfxIcon(int x, int y, const uint8_t *data);

  uint8_t pad(int range, int number);

  void gfxHeader(const char *str);

  int ProportionCV(int cv_value, int max_pixels);

 protected:
  // Check cursor blink cycle
  bool CursorBlink();

  void ResetCursor();

 private:
  int clock_countdown[4];    // For clock output timing
  int adc_lag_countdown[4];  // Lag countdown for each input channel
  int cursor_countdown;      // Timer for cursor blinkin'
  uint32_t last_view_tick;   // Time since the last view, for activating screen
                             // blanking
  int inputs[4];             // Last ADC values
  int outputs[4];  // Last DAC values; inputs[] and outputs[] are used to allow
                   // access to values in Views
  bool changed_cv[4];  // Has the input changed by more than 1/8 semitone since
                       // the last read?
  int last_cv[4];      // For change detection
  uint32_t last_clock[4];   // Tick number of the last clock observed by the
                            // child class
  uint32_t cycle_ticks[4];  // Number of ticks between last two clocks
};
}  // namespace hemisphere
