#include "hemisphere/clock_manager.hpp"

#include "oc/core.h"
#include "util/templates.hpp"

using namespace hemisphere;

ClockManager *ClockManager::instance = nullptr;

void ClockManager::SetMultiply(int multiply, int ch) {
  multiply = clamp(multiply, CLOCK_MIN_MULTIPLE, CLOCK_MAX_MULTIPLE);
  tocks_per_beat[ch] = multiply;
}

// adjusts the expected clock multiple for external clock pulses
void ClockManager::SetClockPPQN(int clkppqn) {
  clock_ppqn = clamp(clkppqn, 0, 24);
}

/* Set ticks per tock, based on one million ticks per minute divided by beats
 * per minute. This is approximate, because the arithmetical value is likely to
 * be fractional, and we need to live with a certain amount of imprecision here.
 * So I'm not even rounding up.
 */
void ClockManager::SetTempoBPM(uint16_t bpm) {
  bpm = clamp(bpm, CLOCK_TEMPO_MIN, CLOCK_TEMPO_MAX);
  ticks_per_beat = 1000000 / bpm;
  tempo = bpm;
}

void ClockManager::SetTempoFromTaps(uint32_t *taps, int count) {
  uint32_t total = 0;
  for (int i = 0; i < count; ++i) {
    total += taps[i];
  }

  // update the tempo
  uint32_t clock_diff = total / count;
  ticks_per_beat =
      clamp(clock_diff, CLOCK_TICKS_MIN,
            CLOCK_TICKS_MAX);        // time since last clock is new tempo
  tempo = 1000000 / ticks_per_beat;  // imprecise, for display purposes
}

// Reset - Resync multipliers, optionally skipping the first tock
void ClockManager::Reset(bool count_skip) {
  beat_tick = oc::core::ticks;
  for (int ch = 0; ch < NR_OF_CLOCKS; ch++) {
    if (tocks_per_beat[ch] > 0 || 0 == count_skip) count[ch] = count_skip;
  }
  cycle = 1 - cycle;
}

// Nudge - Used to align the internal clock with incoming clock pulses
// The rationale is that it's better to be short by 1 than to overshoot by 1
void ClockManager::Nudge(int diff) {
  if (diff > 0) diff--;
  if (diff < 0) diff++;
  beat_tick += diff;
}

// call this on every tick when clock is running, before all Controllers
void ClockManager::SyncTrig(bool clocked, bool hard_reset) {
  // if (!IsRunning()) return;
  if (hard_reset) Reset();

  uint32_t now = oc::core::ticks;

  // Reset only when all multipliers have been met
  bool reset = 1;

  // count and calculate Tocks
  for (int ch = 0; ch < NR_OF_CLOCKS; ch++) {
    if (tocks_per_beat[ch] == 0) {  // disabled
      tock[ch] = 0;
      continue;
    }

    if (tocks_per_beat[ch] > 0) {  // multiply
      uint32_t next_tock_tick =
          beat_tick + count[ch] * ticks_per_beat /
                          static_cast<uint32_t>(tocks_per_beat[ch]);
      tock[ch] = now >= next_tock_tick;
      if (tock[ch]) ++count[ch];  // increment multiplier counter

      reset = reset &&
              (count[ch] > tocks_per_beat[ch]);  // multiplier has been exceeded
    } else {  // division: -1 becomes /2, -2 becomes /3, etc.
      int div = 1 - tocks_per_beat[ch];
      uint32_t next_beat = beat_tick + (count[ch] ? ticks_per_beat : 0);
      bool beat_exceeded = (now > next_beat);
      if (beat_exceeded) {
        ++count[ch];
        tock[ch] = (count[ch] % div) == 1;
      } else
        tock[ch] = 0;

      // resync on every beat
      reset = reset && beat_exceeded;
      if (tock[ch]) count[ch] = 1;
    }
  }
  if (reset) Reset(1);  // skip the one we're already on

  // handle syncing to physical clocks
  if (clocked && clock_tick && clock_ppqn) {
    uint32_t clock_diff = now - clock_tick;
    if (clock_ppqn * clock_diff > CLOCK_TICKS_MAX)
      clock_tick = 0;  // too slow, reset clock tracking

    // if there is a previous clock tick, update tempo and sync
    if (clock_tick && clock_diff) {
      // update the tempo
      ticks_per_beat =
          clamp(clock_ppqn * clock_diff, CLOCK_TICKS_MIN,
                CLOCK_TICKS_MAX);        // time since last clock is new tempo
      tempo = 1000000 / ticks_per_beat;  // imprecise, for display purposes

      int ticks_per_clock = ticks_per_beat / clock_ppqn;  // rounded down

      // time since last beat
      int tick_offset = now - beat_tick;

      // too long ago? time til next beat
      if (tick_offset > ticks_per_clock / 2) {
        tick_offset -= ticks_per_beat;
      }

      // within half a clock pulse of the nearest beat AND significantly large
      if (abs(tick_offset) < ticks_per_clock / 2 && abs(tick_offset) > 4)
        Nudge(tick_offset);  // nudge the beat towards us
    }
  }
  // clock has been physically ticked
  if (clocked) {
    clock_tick = now;
  }
}