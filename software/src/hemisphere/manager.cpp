#include "hemisphere/manager.hpp"

using namespace hemisphere;

void Manager::Start() {
  select_mode = -1;         // Not selecting
  midi_in_hemisphere = -1;  // No MIDI In

  help_hemisphere = -1;
  clock_setup = 0;

  SetApplet(0, get_applet_index_by_id(18));  // DualTM
  SetApplet(1, get_applet_index_by_id(15));  // EuclidX
}

void Manager::Resume() {
  if (!active_preset) LoadFromPreset(0);
}
void Manager::Suspend() {
  if (active_preset) {
    // Preset A will auto-save
    if (preset_id == 0) StoreToPreset(0);
    active_preset->OnSendSysEx();
  }
}

void Manager::StoreToPreset(Preset *preset) {
  active_preset = preset;
  for (int h = 0; h < 2; h++) {
    int index = my_applet[h];
    active_preset->SetAppletId(h, hemisphere::available_applets[index].id);

    uint64_t data = hemisphere::available_applets[index].OnDataRequest(h);
    active_preset->SetData(h, data);
  }
  active_preset->StoreClockData();
}
void Manager::StoreToPreset(int id) {
  StoreToPreset((Preset *)(presets + id));
  preset_id = id;
}
void Manager::LoadFromPreset(int id) {
  active_preset = (Preset *)(presets + id);
  if (active_preset->is_valid()) {
    active_preset->LoadClockData();
    for (int h = 0; h < 2; h++) {
      int index = get_applet_index_by_id(active_preset->GetAppletId(h));
      SetApplet(h, index);
      hemisphere::available_applets[index].OnDataReceive(h, active_preset->GetData(h));
    }
  }
  preset_id = id;
}

// does not modify the preset, only the manager
void Manager::SetApplet(int hemisphere, int index) {
  my_applet[hemisphere] = index;
  if (midi_in_hemisphere == hemisphere) midi_in_hemisphere = -1;
  if (hemisphere::available_applets[index].id & 0x80) midi_in_hemisphere = hemisphere;
  hemisphere::available_applets[index].Start(hemisphere);
}

void Manager::ChangeApplet(int h, int dir) {
  int index = get_next_applet_index(my_applet[h], dir);
  SetApplet(select_mode, index);
}

bool Manager::SelectModeEnabled() { return select_mode > -1; }

void Manager::Controller() {
  // TODO: eliminate the need for this with top-level MIDI handling
  if (midi_in_hemisphere == -1) {
    // Only one ISR can look for MIDI messages at a time, so we need to check
    // for another MIDI In applet before looking for sysex. Note that applets
    // that use MIDI In should check for sysex themselves; see Midi In for an
    // example.
    if (usbMIDI.read() && usbMIDI.getType() == usbMIDI.SystemExclusive) {
      if (active_preset) active_preset->OnReceiveSysEx();
    }
  }

  bool clock_sync = oc::DigitalInputs::clocked<oc::DIGITAL_INPUT_1>();
  bool reset = oc::DigitalInputs::clocked<oc::DIGITAL_INPUT_4>();

  // Paused means wait for clock-sync to start
  if (clock_m->IsPaused() && clock_sync) clock_m->Start();
  // TODO: automatically stop...

  // Advance internal clock, sync to external clock / reset
  if (clock_m->IsRunning()) clock_m->SyncTrig(clock_sync, reset);

  // NJM: always execute ClockSetup controller - it handles MIDI clock out
  hemisphere::clock_setup_applet.Controller(LEFT_HEMISPHERE, clock_m->IsForwarded());

  for (int h = 0; h < 2; h++) {
    int index = my_applet[h];
    hemisphere::available_applets[index].Controller(h, clock_m->IsForwarded());
  }
}

void Manager::View() {
  if (config_menu) {
    DrawConfigMenu();
    return;
  }

  if (clock_setup) {
    hemisphere::clock_setup_applet.View(LEFT_HEMISPHERE);
    return;
  }

  if (help_hemisphere > -1) {
    int index = my_applet[help_hemisphere];
    hemisphere::available_applets[index].View(help_hemisphere);
  } else {
    for (int h = 0; h < 2; h++) {
      int index = my_applet[h];
      hemisphere::available_applets[index].View(h);
    }

    if (clock_m->IsRunning()) {
      // Metronome icon
      gfxIcon(56, 1, clock_m->Cycle() ? METRO_L_ICON : METRO_R_ICON);
    } else if (clock_m->IsPaused()) {
      gfxIcon(56, 1, PAUSE_ICON);
    }

    if (clock_m->IsForwarded()) {
      // CV Forwarding Icon
      gfxIcon(120, 1, CLOCK_ICON);
    }

    if (select_mode == LEFT_HEMISPHERE) graphics.drawFrame(0, 0, 64, 64);
    if (select_mode == RIGHT_HEMISPHERE) graphics.drawFrame(64, 0, 64, 64);
  }
}

void Manager::DelegateEncoderPush(const UI::Event &event) {
  bool down = (event.type == UI::EVENT_BUTTON_DOWN);
  int h = (event.control == oc::CONTROL_BUTTON_L) ? LEFT_HEMISPHERE
                                                  : RIGHT_HEMISPHERE;

  if (config_menu) {
    // button release for config screen
    if (!down) ConfigButtonPush(h);
    return;
  }

  // button down
  if (down) {
    // Clock Setup is more immediate for manual triggers
    if (clock_setup) hemisphere::clock_setup_applet.OnButtonPress(LEFT_HEMISPHERE);
    // TODO: consider a new OnButtonDown handler for applets
    return;
  }

  // button release
  if (select_mode == h) {
    select_mode =
        -1;  // Pushing a button for the selected side turns off select mode
  } else if (!clock_setup) {
    // regular applets get button release
    int index = my_applet[h];
    hemisphere::available_applets[index].OnButtonPress(h);
  }
}

void Manager::DelegateSelectButtonPush(const UI::Event &event) {
  bool down = (event.type == UI::EVENT_BUTTON_DOWN);
  int hemisphere = (event.control == oc::CONTROL_BUTTON_UP) ? LEFT_HEMISPHERE
                                                            : RIGHT_HEMISPHERE;

  if (config_menu) {
    // cancel preset select, or config screen on select button release
    if (!down) {
      if (preset_cursor) {
        preset_cursor = 0;
      } else
        config_menu = 0;
    }
    return;
  }

  // -- button down
  if (down) {
    if (oc::core::ticks - click_tick < HEMISPHERE_DOUBLE_CLICK_TIME) {
      // This is a double-click. Activate corresponding help screen or Clock
      // Setup
      if (hemisphere == first_click)
        SetHelpScreen(hemisphere);
      else if (oc::core::ticks - click_tick <
               HEMISPHERE_SIM_CLICK_TIME)  // dual press for clock setup uses
                                           // shorter timing
        clock_setup = 1;

      // leave Select Mode, and reset the double-click timer
      select_mode = -1;
      click_tick = 0;
    } else {
      // If a help screen is already selected, and the button is for
      // the opposite one, go to the other help screen
      if (help_hemisphere > -1) {
        if (help_hemisphere != hemisphere)
          SetHelpScreen(hemisphere);
        else
          SetHelpScreen(
              -1);  // Leave help screen if corresponding button is clicked
      }

      // mark this single click
      click_tick = oc::core::ticks;
      first_click = hemisphere;
    }
    return;
  }

  // -- button release
  if (!clock_setup) {
    // Select Mode
    if (hemisphere == select_mode)
      select_mode = -1;  // Exit Select Mode if same button is pressed
    else if (help_hemisphere <
             0)  // Otherwise, set Select Mode - UNLESS there's a help screen
      select_mode = hemisphere;
  }

  if (click_tick)
    clock_setup =
        0;  // Turn off clock setup with any single-click button release
}

void Manager::DelegateEncoderMovement(const UI::Event &event) {
  int h = (event.control == oc::CONTROL_ENCODER_L) ? LEFT_HEMISPHERE
                                                   : RIGHT_HEMISPHERE;
  if (config_menu) {
    ConfigEncoderAction(h, event.value);
    return;
  }

  if (clock_setup) {
    hemisphere::clock_setup_applet.OnEncoderMove(LEFT_HEMISPHERE, event.value);
  } else if (select_mode == h) {
    ChangeApplet(h, event.value);
  } else {
    int index = my_applet[h];
    hemisphere::available_applets[index].OnEncoderMove(h, event.value);
  }
}

void Manager::ToggleClockRun() {
  if (clock_m->IsRunning()) {
    clock_m->Stop();
    usbMIDI.sendRealTime(usbMIDI.Stop);
  } else {
    bool p = clock_m->IsPaused();
    clock_m->Start(!p);
    if (p) usbMIDI.sendRealTime(usbMIDI.Start);
  }
}

void Manager::ToggleClockSetup() { clock_setup = 1 - clock_setup; }

void Manager::ToggleConfigMenu() {
  config_menu = !config_menu;
  if (config_menu) SetHelpScreen(-1);
}

void Manager::SetHelpScreen(int hemisphere) {
  if (help_hemisphere > -1) {  // Turn off the previous help screen
    int index = my_applet[help_hemisphere];
    hemisphere::available_applets[index].ToggleHelpScreen(help_hemisphere);
  }

  if (hemisphere > -1) {  // Turn on the next hemisphere's screen
    int index = my_applet[hemisphere];
    hemisphere::available_applets[index].ToggleHelpScreen(hemisphere);
  }

  help_hemisphere = hemisphere;
}

enum HEMConfigCursor {
  LOAD_PRESET,
  SAVE_PRESET,
  TRIG_LENGTH,
  CURSOR_MODE,

  MAX_CURSOR = CURSOR_MODE
};

void Manager::ConfigEncoderAction(int h, int dir) {
  if (!isEditing && !preset_cursor) {
    config_cursor += dir;
    config_cursor = constrain(config_cursor, 0, MAX_CURSOR);
    ResetCursor();
    return;
  }

  if (config_cursor == TRIG_LENGTH) {
    AppletBase::trig_length =
        (uint32_t)constrain(int(AppletBase::trig_length + dir), 1, 127);
  } else if (config_cursor == SAVE_PRESET || config_cursor == LOAD_PRESET) {
    preset_cursor = constrain(preset_cursor + dir, 1, kNumPresets);
  }
}
void Manager::ConfigButtonPush(int h) {
  if (preset_cursor) {
    // Save or Load on button push
    if (config_cursor == SAVE_PRESET)
      StoreToPreset(preset_cursor - 1);
    else
      LoadFromPreset(preset_cursor - 1);

    preset_cursor = 0;  // deactivate preset selection
    config_menu = 0;
    isEditing = false;
    return;
  }

  switch (config_cursor) {
    case SAVE_PRESET:
    case LOAD_PRESET:
      preset_cursor = preset_id + 1;
      break;

    case TRIG_LENGTH:
      isEditing = !isEditing;
      break;

    case CURSOR_MODE:
      AppletBase::CycleEditMode();
      break;
  }
}

void Manager::DrawConfigMenu() {
  // --- Preset Selector
  if (preset_cursor) {
    DrawPresetSelector();
    return;
  }

  // --- Config Selection
  gfxHeader("Hemisphere Config");
  gfxPrint(1, 15, "Preset: ");
  gfxPrint(48, 15, "Load / Save");

  gfxPrint(1, 35, "Trig Length: ");
  gfxPrint(AppletBase::trig_length);

  const char *cursor_mode_name[3] = {"legacy", "modal", "modal+wrap"};
  gfxPrint(1, 45, "Cursor:  ");
  gfxPrint(cursor_mode_name[AppletBase::modal_edit_mode]);

  switch (config_cursor) {
    case LOAD_PRESET:
    case SAVE_PRESET:
      gfxIcon(55 + (config_cursor - LOAD_PRESET) * 45, 25, UP_ICON);
      break;

    case TRIG_LENGTH:
      if (isEditing)
        gfxInvert(79, 34, 25, 9);
      else
        gfxCursor(80, 43, 24);
      break;
    case CURSOR_MODE:
      gfxIcon(43, 45, RIGHT_ICON);
      break;
  }
}

void Manager::DrawPresetSelector() {
  gfxHeader("Hemisphere Presets");
  int y = 5 + preset_cursor * 10;
  gfxPrint(1, y, (config_cursor == SAVE_PRESET) ? "Save" : "Load");
  gfxIcon(26, y, RIGHT_ICON);
  for (int i = 0; i < kNumPresets; ++i) {
    y = 15 + i * 10;
    gfxPrint(35, y, preset_name[i]);

    if (!presets[i].is_valid())
      gfxPrint(" (empty)");
    else if (i == preset_id)
      gfxIcon(45, y, ZAP_ICON);
  }
}

int Manager::get_applet_index_by_id(int id) {
  int index = 0;
  for (int i = 0; i < kNumAvailableApplets; i++) {
    if (hemisphere::available_applets[i].id == id) index = i;
  }
  return index;
}

int Manager::get_next_applet_index(int index, int dir) {
  index += dir;
  if (index >= kNumAvailableApplets) index = 0;
  if (index < 0) index = kNumAvailableApplets - 1;

  // If an applet uses MIDI In, it can only be selected in one
  // hemisphere, and is designated by bit 7 set in its id.
  if (hemisphere::available_applets[index].id & 0x80) {
    if (midi_in_hemisphere == (1 - select_mode)) {
      return get_next_applet_index(index, dir);
    }
  }

  return index;
}
