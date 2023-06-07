#pragma once
// Copyright (c) 2018, Jason Justian
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "oc/DAC.h"
#include "oc/digital_inputs.h"
#include "oc/visualfx.h"
#include "oc/patterns.h"
#include "FreqMeasure.h"
#include "oc/ui.h"
#include "oc/apps.h"
#include "oc/menus.h"
namespace menu = OC::menu;

#include "HemisphereApplet.h"
#include "hemisphere/Application.h"
#include "hemisphere/icons.h"
#include "hemisphere/MIDI.h"
#include "hemisphere/ClockManager.h"

// The settings specify the selected applets, and 64 bits of data for each applet
enum HEMISPHERE_SETTINGS {
    HEMISPHERE_SELECTED_LEFT_ID,
    HEMISPHERE_SELECTED_RIGHT_ID,
    HEMISPHERE_LEFT_DATA_B1,
    HEMISPHERE_RIGHT_DATA_B1,
    HEMISPHERE_LEFT_DATA_B2,
    HEMISPHERE_RIGHT_DATA_B2,
    HEMISPHERE_LEFT_DATA_B3,
    HEMISPHERE_RIGHT_DATA_B3,
    HEMISPHERE_LEFT_DATA_B4,
    HEMISPHERE_RIGHT_DATA_B4,
    HEMISPHERE_CLOCK_DATA1,
    HEMISPHERE_CLOCK_DATA2,
    HEMISPHERE_CLOCK_DATA3,
    HEMISPHERE_CLOCK_DATA4,
    HEMISPHERE_SETTING_LAST
};

static constexpr int HEMISPHERE_AVAILABLE_APPLETS = ARRAY_SIZE(HS::available_applets);
static const int HEM_NR_OF_PRESETS = 4;

static const char * hem_preset_name[HEM_NR_OF_PRESETS] = { "A", "B", "C", "D" };

/* Hemisphere Preset
 * - conveniently store/recall multiple configurations
 */
class HemispherePreset : public SystemExclusiveHandler,
    public settings::SettingsBase<HemispherePreset, HEMISPHERE_SETTING_LAST> {
public:
    int GetAppletId(int h) {
        return (h == LEFT_HEMISPHERE) ? values_[HEMISPHERE_SELECTED_LEFT_ID]
                                      : values_[HEMISPHERE_SELECTED_RIGHT_ID];
    }
    void SetAppletId(int h, int id) {
        apply_value(h, id);
    }
    bool is_valid() {
        return values_[HEMISPHERE_SELECTED_LEFT_ID] != 0;
    }

    // restore state by setting applets and giving them data
    void LoadClockData() {
        HS::clock_setup_applet.OnDataReceive(0, (uint64_t(values_[HEMISPHERE_CLOCK_DATA4]) << 48) |
                                                (uint64_t(values_[HEMISPHERE_CLOCK_DATA3]) << 32) |
                                                (uint64_t(values_[HEMISPHERE_CLOCK_DATA2]) << 16) |
                                                 uint64_t(values_[HEMISPHERE_CLOCK_DATA1]));
    }
    void StoreClockData() {
        uint64_t data = HS::clock_setup_applet.OnDataRequest(0);
        apply_value(HEMISPHERE_CLOCK_DATA1, data & 0xffff);
        apply_value(HEMISPHERE_CLOCK_DATA2, (data >> 16) & 0xffff);
        apply_value(HEMISPHERE_CLOCK_DATA3, (data >> 32) & 0xffff);
        apply_value(HEMISPHERE_CLOCK_DATA4, (data >> 48) & 0xffff);
    }


    // Manually get data for one side
    uint64_t GetData(int h) {
        return (uint64_t(values_[8 + h]) << 48) |
               (uint64_t(values_[6 + h]) << 32) |
               (uint64_t(values_[4 + h]) << 16) |
               (uint64_t(values_[2 + h]));
    }

    /* Manually store state data for one side */
    void SetData(int h, uint64_t data) {
        apply_value(2 + h, data & 0xffff);
        apply_value(4 + h, (data >> 16) & 0xffff);
        apply_value(6 + h, (data >> 32) & 0xffff);
        apply_value(8 + h, (data >> 48) & 0xffff);
    }

    // TODO: I haven't updated the SysEx data structure here because I don't use it.
    // Clock data would probably be useful if it's not too big. -NJM
    void OnSendSysEx() {
        // Describe the data structure for the audience
        uint8_t V[18];
        V[0] = (uint8_t)values_[HEMISPHERE_SELECTED_LEFT_ID];
        V[1] = (uint8_t)values_[HEMISPHERE_SELECTED_RIGHT_ID];
        V[2] = (uint8_t)(values_[HEMISPHERE_LEFT_DATA_B1] & 0xff);
        V[3] = (uint8_t)((values_[HEMISPHERE_LEFT_DATA_B1] >> 8) & 0xff);
        V[4] = (uint8_t)(values_[HEMISPHERE_RIGHT_DATA_B1] & 0xff);
        V[5] = (uint8_t)((values_[HEMISPHERE_RIGHT_DATA_B1] >> 8) & 0xff);
        V[6] = (uint8_t)(values_[HEMISPHERE_LEFT_DATA_B2] & 0xff);
        V[7] = (uint8_t)((values_[HEMISPHERE_LEFT_DATA_B2] >> 8) & 0xff);
        V[8] = (uint8_t)(values_[HEMISPHERE_RIGHT_DATA_B2] & 0xff);
        V[9] = (uint8_t)((values_[HEMISPHERE_RIGHT_DATA_B2] >> 8) & 0xff);
        V[10] = (uint8_t)(values_[HEMISPHERE_LEFT_DATA_B3] & 0xff);
        V[11] = (uint8_t)((values_[HEMISPHERE_LEFT_DATA_B3] >> 8) & 0xff);
        V[12] = (uint8_t)(values_[HEMISPHERE_RIGHT_DATA_B3] & 0xff);
        V[13] = (uint8_t)((values_[HEMISPHERE_RIGHT_DATA_B3] >> 8) & 0xff);
        V[14] = (uint8_t)(values_[HEMISPHERE_LEFT_DATA_B4] & 0xff);
        V[15] = (uint8_t)((values_[HEMISPHERE_LEFT_DATA_B4] >> 8) & 0xff);
        V[16] = (uint8_t)(values_[HEMISPHERE_RIGHT_DATA_B4] & 0xff);
        V[17] = (uint8_t)((values_[HEMISPHERE_RIGHT_DATA_B4] >> 8) & 0xff);

        // Pack it up, ship it out
        UnpackedData unpacked;
        unpacked.set_data(18, V);
        PackedData packed = unpacked.pack();
        SendSysEx(packed, 'H');
    }

    void OnReceiveSysEx() {
        uint8_t V[18];
        if (ExtractSysExData(V, 'H')) {
            values_[HEMISPHERE_SELECTED_LEFT_ID] = V[0];
            values_[HEMISPHERE_SELECTED_RIGHT_ID] = V[1];
            values_[HEMISPHERE_LEFT_DATA_B1] = ((uint16_t)V[3] << 8) + V[2];
            values_[HEMISPHERE_RIGHT_DATA_B1] = ((uint16_t)V[5] << 8) + V[4];
            values_[HEMISPHERE_LEFT_DATA_B2] = ((uint16_t)V[7] << 8) + V[6];
            values_[HEMISPHERE_RIGHT_DATA_B2] = ((uint16_t)V[9] << 8) + V[8];
            values_[HEMISPHERE_LEFT_DATA_B3] = ((uint16_t)V[11] << 8) + V[10];
            values_[HEMISPHERE_RIGHT_DATA_B3] = ((uint16_t)V[13] << 8) + V[12];
            values_[HEMISPHERE_LEFT_DATA_B4] = ((uint16_t)V[15] << 8) + V[14];
            values_[HEMISPHERE_RIGHT_DATA_B4] = ((uint16_t)V[17] << 8) + V[16];
            //LoadClockData();
        }
    }

};

// HemispherePreset hem_config; // special place for Clock data and Config data, 64 bits each

static HemispherePreset hem_presets[HEM_NR_OF_PRESETS];
static HemispherePreset *hem_active_preset;

////////////////////////////////////////////////////////////////////////////////
//// Hemisphere Manager
////////////////////////////////////////////////////////////////////////////////

class HemisphereManager : public HSApplication {
public:
    void Start() {
        select_mode = -1; // Not selecting
        midi_in_hemisphere = -1; // No MIDI In

        help_hemisphere = -1;
        clock_setup = 0;

        SetApplet(0, get_applet_index_by_id(18)); // DualTM
        SetApplet(1, get_applet_index_by_id(15)); // EuclidX
    }

    void Resume() {
        if (!hem_active_preset)
            LoadFromPreset(0);
    }
    void Suspend() {
        if (hem_active_preset) {
            // Preset A will auto-save
            if (preset_id == 0) StoreToPreset(0);
            hem_active_preset->OnSendSysEx();
        }
    }

    void StoreToPreset(HemispherePreset* preset) {
        hem_active_preset = preset;
        for (int h = 0; h < 2; h++)
        {
            int index = my_applet[h];
            hem_active_preset->SetAppletId(h, HS::available_applets[index].id);

            uint64_t data = HS::available_applets[index].OnDataRequest(h);
            hem_active_preset->SetData(h, data);
        }
        hem_active_preset->StoreClockData();
    }
    void StoreToPreset(int id) {
        StoreToPreset( (HemispherePreset*)(hem_presets + id) );
        preset_id = id;
    }
    void LoadFromPreset(int id) {
        hem_active_preset = (HemispherePreset*)(hem_presets + id);
        if (hem_active_preset->is_valid()) {
            hem_active_preset->LoadClockData();
            for (int h = 0; h < 2; h++)
            {
                int index = get_applet_index_by_id( hem_active_preset->GetAppletId(h) );
                SetApplet(h, index);
                HS::available_applets[index].OnDataReceive(h, hem_active_preset->GetData(h));
            }
        }
        preset_id = id;
    }

    // does not modify the preset, only the manager
    void SetApplet(int hemisphere, int index) {
        my_applet[hemisphere] = index;
        if (midi_in_hemisphere == hemisphere) midi_in_hemisphere = -1;
        if (HS::available_applets[index].id & 0x80) midi_in_hemisphere = hemisphere;
        HS::available_applets[index].Start(hemisphere);
    }

    void ChangeApplet(int h, int dir) {
        int index = get_next_applet_index(my_applet[h], dir);
        SetApplet(select_mode, index);
    }

    bool SelectModeEnabled() {
        return select_mode > -1;
    }

    void Controller() {
        // TODO: eliminate the need for this with top-level MIDI handling
        if (midi_in_hemisphere == -1) {
            // Only one ISR can look for MIDI messages at a time, so we need to check
            // for another MIDI In applet before looking for sysex. Note that applets
            // that use MIDI In should check for sysex themselves; see Midi In for an
            // example.
            if (usbMIDI.read() && usbMIDI.getType() == usbMIDI.SystemExclusive) {
                if (hem_active_preset)
                    hem_active_preset->OnReceiveSysEx();
            }
        }

        bool clock_sync = OC::DigitalInputs::clocked<OC::DIGITAL_INPUT_1>();
        bool reset = OC::DigitalInputs::clocked<OC::DIGITAL_INPUT_4>();

        // Paused means wait for clock-sync to start
        if (clock_m->IsPaused() && clock_sync) clock_m->Start();
        // TODO: automatically stop...

        // Advance internal clock, sync to external clock / reset
        if (clock_m->IsRunning()) clock_m->SyncTrig( clock_sync, reset );

        // NJM: always execute ClockSetup controller - it handles MIDI clock out
        HS::clock_setup_applet.Controller(LEFT_HEMISPHERE, clock_m->IsForwarded());

        for (int h = 0; h < 2; h++)
        {
            int index = my_applet[h];
            HS::available_applets[index].Controller(h, clock_m->IsForwarded());
        }
    }

    void View() {
        if (config_menu) {
            DrawConfigMenu();
            return;
        }

        if (clock_setup) {
            HS::clock_setup_applet.View(LEFT_HEMISPHERE);
            return;
        }

        if (help_hemisphere > -1) {
            int index = my_applet[help_hemisphere];
            HS::available_applets[index].View(help_hemisphere);
        } else {
            for (int h = 0; h < 2; h++)
            {
                int index = my_applet[h];
                HS::available_applets[index].View(h);
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

    void DelegateEncoderPush(const UI::Event &event) {
        bool down = (event.type == UI::EVENT_BUTTON_DOWN);
        int h = (event.control == OC::CONTROL_BUTTON_L) ? LEFT_HEMISPHERE : RIGHT_HEMISPHERE;

        if (config_menu) {
            // button release for config screen
            if (!down) ConfigButtonPush(h);
            return;
        }

        // button down
        if (down) {
            // Clock Setup is more immediate for manual triggers
            if (clock_setup) HS::clock_setup_applet.OnButtonPress(LEFT_HEMISPHERE);
            // TODO: consider a new OnButtonDown handler for applets
            return;
        }

        // button release
        if (select_mode == h) {
            select_mode = -1; // Pushing a button for the selected side turns off select mode
        } else if (!clock_setup) {
            // regular applets get button release
            int index = my_applet[h];
            HS::available_applets[index].OnButtonPress(h);
        }
    }

    void DelegateSelectButtonPush(const UI::Event &event) {
        bool down = (event.type == UI::EVENT_BUTTON_DOWN);
        int hemisphere = (event.control == OC::CONTROL_BUTTON_UP) ? LEFT_HEMISPHERE : RIGHT_HEMISPHERE;

        if (config_menu) {
            // cancel preset select, or config screen on select button release
            if (!down) {
                if (preset_cursor) {
                    preset_cursor = 0;
                }
                else config_menu = 0;
            }
            return;
        }

        // -- button down
        if (down) {
            if (OC::CORE::ticks - click_tick < HEMISPHERE_DOUBLE_CLICK_TIME) {
                // This is a double-click. Activate corresponding help screen or Clock Setup
                if (hemisphere == first_click)
                    SetHelpScreen(hemisphere);
                else if (OC::CORE::ticks - click_tick < HEMISPHERE_SIM_CLICK_TIME) // dual press for clock setup uses shorter timing
                    clock_setup = 1;

                // leave Select Mode, and reset the double-click timer
                select_mode = -1;
                click_tick = 0;
            } else {
                // If a help screen is already selected, and the button is for
                // the opposite one, go to the other help screen
                if (help_hemisphere > -1) {
                    if (help_hemisphere != hemisphere) SetHelpScreen(hemisphere);
                    else SetHelpScreen(-1); // Leave help screen if corresponding button is clicked
                }

                // mark this single click
                click_tick = OC::CORE::ticks;
                first_click = hemisphere;
            }
            return;
        }

        // -- button release
        if (!clock_setup) {
            // Select Mode
            if (hemisphere == select_mode) select_mode = -1; // Exit Select Mode if same button is pressed
            else if (help_hemisphere < 0) // Otherwise, set Select Mode - UNLESS there's a help screen
                select_mode = hemisphere;
        }

        if (click_tick)
            clock_setup = 0; // Turn off clock setup with any single-click button release
    }

    void DelegateEncoderMovement(const UI::Event &event) {
        int h = (event.control == OC::CONTROL_ENCODER_L) ? LEFT_HEMISPHERE : RIGHT_HEMISPHERE;
        if (config_menu) {
            ConfigEncoderAction(h, event.value);
            return;
        }

        if (clock_setup) {
            HS::clock_setup_applet.OnEncoderMove(LEFT_HEMISPHERE, event.value);
        } else if (select_mode == h) {
            ChangeApplet(h, event.value);
        } else {
            int index = my_applet[h];
            HS::available_applets[index].OnEncoderMove(h, event.value);
        }
    }

    void ToggleClockRun() {
        if (clock_m->IsRunning()) {
            clock_m->Stop();
            usbMIDI.sendRealTime(usbMIDI.Stop);
        } else {
            bool p = clock_m->IsPaused();
            clock_m->Start( !p );
            if (p) usbMIDI.sendRealTime(usbMIDI.Start);
        }
    }

    void ToggleClockSetup() {
        clock_setup = 1 - clock_setup;
    }

    void ToggleConfigMenu() {
        config_menu = !config_menu;
        if (config_menu) SetHelpScreen(-1);
    }

    void SetHelpScreen(int hemisphere) {
        if (help_hemisphere > -1) { // Turn off the previous help screen
            int index = my_applet[help_hemisphere];
            HS::available_applets[index].ToggleHelpScreen(help_hemisphere);
        }

        if (hemisphere > -1) { // Turn on the next hemisphere's screen
            int index = my_applet[hemisphere];
            HS::available_applets[index].ToggleHelpScreen(hemisphere);
        }

        help_hemisphere = hemisphere;
    }

private:
    int preset_id = 0;
    int preset_cursor = 0;
    int my_applet[2]; // Indexes to available_applets
    int select_mode;
    bool clock_setup;
    bool config_menu;
    bool isEditing = false;
    int config_cursor = 0;

    int help_hemisphere; // Which of the hemispheres (if any) is in help mode, or -1 if none
    int midi_in_hemisphere; // Which of the hemispheres (if any) is using MIDI In
    uint32_t click_tick; // Measure time between clicks for double-click
    int first_click; // The first button pushed of a double-click set, to see if the same one is pressed
    ClockManager *clock_m = clock_m->get();

    enum HEMConfigCursor {
        LOAD_PRESET, SAVE_PRESET,
        TRIG_LENGTH,
        CURSOR_MODE,

        MAX_CURSOR = CURSOR_MODE
    };

    void ConfigEncoderAction(int h, int dir) {
        if (!isEditing && !preset_cursor) {
            config_cursor += dir;
            config_cursor = constrain(config_cursor, 0, MAX_CURSOR);
            ResetCursor();
            return;
        }

        if (config_cursor == TRIG_LENGTH) {
            HemisphereApplet::trig_length = (uint32_t) constrain( int(HemisphereApplet::trig_length + dir), 1, 127);
        }
        else if (config_cursor == SAVE_PRESET || config_cursor == LOAD_PRESET) {
            preset_cursor = constrain(preset_cursor + dir, 1, HEM_NR_OF_PRESETS);
        }
    }
    void ConfigButtonPush(int h) {
        if (preset_cursor) {
            // Save or Load on button push
            if (config_cursor == SAVE_PRESET)
                StoreToPreset(preset_cursor-1);
            else
                LoadFromPreset(preset_cursor-1);

            preset_cursor = 0; // deactivate preset selection
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
            HemisphereApplet::CycleEditMode();
            break;
        }
    }

    void DrawConfigMenu() {
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
        gfxPrint(HemisphereApplet::trig_length);

        const char * cursor_mode_name[3] = { "legacy", "modal", "modal+wrap" };
        gfxPrint(1, 45, "Cursor:  ");
        gfxPrint(cursor_mode_name[HemisphereApplet::modal_edit_mode]);
        
        switch (config_cursor) {
        case LOAD_PRESET:
        case SAVE_PRESET:
            gfxIcon(55 + (config_cursor - LOAD_PRESET)*45, 25, UP_ICON);
            break;

        case TRIG_LENGTH:
            if (isEditing) gfxInvert(79, 34, 25, 9);
            else gfxCursor(80, 43, 24);
            break;
        case CURSOR_MODE:
            gfxIcon(43, 45, RIGHT_ICON);
            break;
        }
    }

    void DrawPresetSelector() {
        gfxHeader("Hemisphere Presets");
        int y = 5 + preset_cursor*10;
        gfxPrint(1, y, (config_cursor == SAVE_PRESET) ? "Save" : "Load");
        gfxIcon(26, y, RIGHT_ICON);
        for (int i = 0; i < HEM_NR_OF_PRESETS; ++i) {
            y = 15 + i*10;
            gfxPrint(35, y, hem_preset_name[i]);

            if (!hem_presets[i].is_valid())
                gfxPrint(" (empty)");
            else if (i == preset_id)
                gfxIcon(45, y, ZAP_ICON);
        }
    }

    int get_applet_index_by_id(int id) {
        int index = 0;
        for (int i = 0; i < HEMISPHERE_AVAILABLE_APPLETS; i++)
        {
            if (HS::available_applets[i].id == id) index = i;
        }
        return index;
    }

    int get_next_applet_index(int index, int dir) {
        index += dir;
        if (index >= HEMISPHERE_AVAILABLE_APPLETS) index = 0;
        if (index < 0) index = HEMISPHERE_AVAILABLE_APPLETS - 1;

        // If an applet uses MIDI In, it can only be selected in one
        // hemisphere, and is designated by bit 7 set in its id.
        if (HS::available_applets[index].id & 0x80) {
            if (midi_in_hemisphere == (1 - select_mode)) {
                return get_next_applet_index(index, dir);
            }
        }

        return index;
    }
};

static HemisphereManager manager;

void ReceiveManagerSysEx();

////////////////////////////////////////////////////////////////////////////////
//// O_C App Functions
////////////////////////////////////////////////////////////////////////////////

// App stubs
void HEMISPHERE_init();
size_t HEMISPHERE_storageSize();
size_t HEMISPHERE_save(void *storage);
size_t HEMISPHERE_restore(const void *storage);
void FASTRUN HEMISPHERE_isr();
void HEMISPHERE_handleAppEvent(OC::AppEvent event);
void HEMISPHERE_loop(); // Essentially deprecated in favor of ISR
void HEMISPHERE_menu();
void HEMISPHERE_screensaver(); // Deprecated in favor of screen blanking
void HEMISPHERE_handleButtonEvent(const UI::Event &event);
void HEMISPHERE_handleEncoderEvent(const UI::Event &event);
