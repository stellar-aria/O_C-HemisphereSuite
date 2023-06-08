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

// A "tick" is one ISR cycle, which happens 16666.667 times per second, or a
// million times per minute. A "tock" is a metronome beat.

#pragma once
#include <stdint.h>

static constexpr uint16_t CLOCK_TEMPO_MIN = 10;
static constexpr uint16_t CLOCK_TEMPO_MAX = 300;
static constexpr uint32_t CLOCK_TICKS_MIN = 1000000 / CLOCK_TEMPO_MAX;
static constexpr uint32_t CLOCK_TICKS_MAX = 1000000 / CLOCK_TEMPO_MIN;

constexpr int MIDI_OUT_PPQN = 24;
constexpr int CLOCK_MAX_MULTIPLE = 24;
constexpr int CLOCK_MIN_MULTIPLE = -31;  // becomes /32

namespace hemisphere {

class ClockManager {
  static ClockManager *instance;

  enum ClockOutput {
    LEFT_CLOCK1,
    LEFT_CLOCK2,
    RIGHT_CLOCK1,
    RIGHT_CLOCK2,
    MIDI_CLOCK,
    NR_OF_CLOCKS
  };

  uint16_t tempo;           // The set tempo, for display somewhere else
  uint32_t ticks_per_beat;  // Based on the selected tempo in BPM
  bool running = 0;  // Specifies whether the clock is running for interprocess
                     // communication
  bool paused = 0;   // Specifies whethr the clock is paused
  bool forwarded = 0;  // Master clock forwarding is enabled when true

  // tick when a physical clock was received on DIGITAL 1
  uint32_t clock_tick = 0;

  // The tick to count from
  uint32_t beat_tick = 0;

  // The current tock value
  bool tock[NR_OF_CLOCKS] = {0, 0, 0, 0, 0};

  // Multiplier
  int16_t tocks_per_beat[NR_OF_CLOCKS] = {4, 0, 8, 0, MIDI_OUT_PPQN};

  // Multiple counter, 0 is a special case when first starting the clock
  int count[NR_OF_CLOCKS] = {0, 0, 0, 0, 0};

  int clock_ppqn = 4;  // external clock multiple
  bool cycle = 0;      // Alternates for each tock, for display purposes

  bool boop[4];  // Manual triggers

  ClockManager() { SetTempoBPM(120); }

 public:
  static ClockManager *get() {
    if (!instance) instance = new ClockManager;
    return instance;
  }

  void SetMultiply(int multiply, int ch = 0);

  // adjusts the expected clock multiple for external clock pulses
  void SetClockPPQN(int clkppqn);

  /* Set ticks per tock, based on one million ticks per minute divided by beats
   * per minute. This is approximate, because the arithmetical value is likely
   * to be fractional, and we need to live with a certain amount of imprecision
   * here. So I'm not even rounding up.
   */
  void SetTempoBPM(uint16_t bpm);

  void SetTempoFromTaps(uint32_t *taps, int count);

  int GetMultiply(int ch = 0) { return tocks_per_beat[ch]; }
  int GetClockPPQN() { return clock_ppqn; }

  /* Gets the current tempo. This can be used between client processes, like two
   * different hemispheres.
   */
  uint16_t GetTempo() { return tempo; }

  // Reset - Resync multipliers, optionally skipping the first tock
  void Reset(bool count_skip = 0);

  // Nudge - Used to align the internal clock with incoming clock pulses
  // The rationale is that it's better to be short by 1 than to overshoot by 1
  void Nudge(int diff);

  // call this on every tick when clock is running, before all Controllers
  void SyncTrig(bool clocked, bool hard_reset = false);

  void Start(bool p = 0) {
    Reset();
    running = 1;
    paused = p;
  }

  void Stop() {
    running = 0;
    paused = 0;
  }

  void Pause() { paused = 1; }

  void ToggleForwarding() { forwarded = 1 - forwarded; }

  void SetForwarding(bool f) { forwarded = f; }

  bool IsRunning() { return (running && !paused); }

  bool IsPaused() { return paused; }

  bool IsForwarded() { return forwarded; }

  // beep boop
  void Boop(int ch = 0) { boop[ch] = true; }
  bool Beep(int ch = 0) {
    if (boop[ch]) {
      boop[ch] = false;
      return true;
    }
    return false;
  }

  /* Returns true if the clock should fire on this tick, based on the current
   * tempo and multiplier */
  bool Tock(int ch = 0) { return tock[ch]; }

  // Returns true if MIDI Clock should be sent on this tick
  bool MIDITock() { return Tock(MIDI_CLOCK); }

  bool EndOfBeat(int ch = 0) { return count[ch] == 1; }

  bool Cycle(int ch = 0) { return cycle; }
};
}