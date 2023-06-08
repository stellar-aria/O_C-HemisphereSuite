#pragma once
#include "hemisphere/application_base.hpp"
#include "preset.hpp"
#include "oc/ui.h"

namespace hemisphere {

static constexpr int kNumAvailableApplets = ARRAY_SIZE(hemisphere::available_applets);

////////////////////////////////////////////////////////////////////////////////
//// Hemisphere Manager
////////////////////////////////////////////////////////////////////////////////
class Manager : public ApplicationBase {
public:
    Manager() = default;
    void Start();

    void Resume();
    void Suspend();

    void StoreToPreset(Preset* preset);
    void StoreToPreset(int id);
    void LoadFromPreset(int id);

    // does not modify the preset, only the manager
    void SetApplet(int hemisphere, int index);

    void ChangeApplet(int h, int dir);
    bool SelectModeEnabled();

    void Controller();

    void View();

    void DelegateEncoderPush(const UI::Event &event);

    void DelegateSelectButtonPush(const UI::Event &event);

    void DelegateEncoderMovement(const UI::Event &event);

    void ToggleClockRun();

    void ToggleClockSetup();

    void ToggleConfigMenu();

    void SetHelpScreen(int hemisphere);

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

    void ConfigEncoderAction(int h, int dir);
    void ConfigButtonPush(int h);

    void DrawConfigMenu();

    void DrawPresetSelector();
    int get_applet_index_by_id(int id);

    int get_next_applet_index(int index, int dir);
};
}