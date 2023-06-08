#pragma once
#include "hemisphere/applet_base.hpp"
#include "hemisphere/midi.hpp"
#include "util/settings.h"

namespace hemisphere {
// The settings specify the selected applets, and 64 bits of data for each
// applet
enum class Setting {
  SELECTED_LEFT_ID,
  SELECTED_RIGHT_ID,
  LEFT_DATA_B1,
  RIGHT_DATA_B1,
  LEFT_DATA_B2,
  RIGHT_DATA_B2,
  LEFT_DATA_B3,
  RIGHT_DATA_B3,
  LEFT_DATA_B4,
  RIGHT_DATA_B4,
  CLOCK_DATA1,
  CLOCK_DATA2,
  CLOCK_DATA3,
  CLOCK_DATA4,
  LAST
};

constexpr int kNumPresets = 4;

constexpr const char *preset_name[kNumPresets] = {"A", "B", "C", "D"};

/* Hemisphere Preset
 * - conveniently store/recall multiple configurations
 */
class Preset
    : public SystemExclusiveHandler,
      public settings::SettingsBase<Preset, static_cast<size_t>(Setting::LAST)> {
 public:
  int GetAppletId(int h);
  void SetAppletId(int h, int id);
  bool is_valid();

  // restore state by setting applets and giving them data
  void LoadClockData();
  void StoreClockData();
  
  // Manually get data for one side
  uint64_t GetData(int h);

  /* Manually store state data for one side */
  void SetData(int h, uint64_t data);

  void OnSendSysEx();
  void OnReceiveSysEx();
};

// hemisphere::Preset hem_config; // special place for Clock data and Config data,
// 64 bits each

static Preset presets[kNumPresets];
static Preset *active_preset;
};