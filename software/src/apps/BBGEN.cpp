
// Copyright (c)  2015, 2016 Patrick Dowling, Max Stadler, Tim Churches
//
// Author of original O+C firmware: Max Stadler (mxmlnstdlr@gmail.com)
// Author of app scaffolding: Patrick Dowling (pld@gurkenkiste.com)
// Modified for bouncing balls: Tim Churches (tim.churches@gmail.com)
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

// Bouncing balls app

#ifdef ENABLE_APP_BBGEN

#include "oc/apps.h"
#include "oc/bitmaps.h"
#include "oc/digital_inputs.h"
#include "oc/strings.h"
#include "util/math.h"
#include "util/settings.h"
#include "oc/menus.h"
#include "peaks/bouncing_balls.h"
#include "oc/ui.h"
#include "oc/ADC.h"

enum BouncingBallSettings {
  BB_SETTING_GRAVITY,
  BB_SETTING_BOUNCE_LOSS,
  BB_SETTING_INITIAL_AMPLITUDE,
  BB_SETTING_INITIAL_VELOCITY,
  BB_SETTING_TRIGGER_INPUT,
  BB_SETTING_RETRIGGER_BOUNCES,
  BB_SETTING_CV1,
  BB_SETTING_CV2,
  BB_SETTING_CV3,
  BB_SETTING_CV4,
  BB_SETTING_HARD_RESET,
  BB_SETTING_LAST
};

enum BallCVMapping {
  BB_CV_MAPPING_NONE,
  BB_CV_MAPPING_GRAVITY,
  BB_CV_MAPPING_BOUNCE_LOSS,
  BB_CV_MAPPING_INITIAL_AMPLITUDE,
  BB_CV_MAPPING_INITIAL_VELOCITY,
  BB_CV_MAPPING_RETRIGGER_BOUNCES,
  BB_CV_MAPPING_LAST
};

namespace menu = oc::menu;

class BouncingBall : public settings::SettingsBase<BouncingBall, BB_SETTING_LAST> {
public:

  static constexpr int kMaxBouncingBallParameters = 5;

  void Init(oc::DigitalInput default_trigger);

  oc::DigitalInput get_trigger_input() const {
    return static_cast<oc::DigitalInput>(values_[BB_SETTING_TRIGGER_INPUT]);
  }

  BallCVMapping get_cv1_mapping() const {
    return static_cast<BallCVMapping>(values_[BB_SETTING_CV1]);
  }

  BallCVMapping get_cv2_mapping() const {
    return static_cast<BallCVMapping>(values_[BB_SETTING_CV2]);
  }

  BallCVMapping get_cv3_mapping() const {
    return static_cast<BallCVMapping>(values_[BB_SETTING_CV3]);
  }

  BallCVMapping get_cv4_mapping() const {
    return static_cast<BallCVMapping>(values_[BB_SETTING_CV4]);
  }

  bool get_hard_reset() const {
    return values_[BB_SETTING_HARD_RESET];
  }

  uint8_t get_initial_amplitude() const {
    return values_[BB_SETTING_INITIAL_AMPLITUDE];
  }

  uint8_t get_initial_velocity() const {
    return values_[BB_SETTING_INITIAL_VELOCITY];
  }

  uint8_t get_gravity() const {
    return values_[BB_SETTING_GRAVITY];
  }

  uint8_t get_bounce_loss() const {
    return values_[BB_SETTING_BOUNCE_LOSS];
  }

  int32_t get_retrigger_bounces() const {
    return static_cast<int32_t>(values_[BB_SETTING_RETRIGGER_BOUNCES]);
  }

#ifdef BBGEN_DEBUG
  uint16_t get_channel_parameter_value(uint8_t param) {
    return s[param];
  }

  int16_t get_channel_retrigger_bounces() {
    return(bb_.get_retrigger_bounces()) ;
  }
#endif // BBGEN_DEBUG

  inline void apply_cv_mapping(BouncingBallSettings cv_setting, const int32_t cvs[oc::kNumAdcChannels], int32_t segments[kMaxBouncingBallParameters]) {
    int mapping = values_[cv_setting];
    uint8_t bb_cv_rshift = 13 ;
    switch (mapping) {
      case BB_CV_MAPPING_GRAVITY:
      case BB_CV_MAPPING_BOUNCE_LOSS:
      case BB_CV_MAPPING_INITIAL_VELOCITY:
        bb_cv_rshift = 13 ;
        break ;
      case BB_CV_MAPPING_INITIAL_AMPLITUDE:
        bb_cv_rshift = 12 ;
        break;
      case BB_CV_MAPPING_RETRIGGER_BOUNCES:
        bb_cv_rshift = 14 ;
        break;
      default:
        bb_cv_rshift = 13 ;
        break;
    }
    if (mapping)
      segments[mapping - BB_CV_MAPPING_GRAVITY] += (cvs[cv_setting - BB_SETTING_CV1]) << (16 - bb_cv_rshift) ;
  }

  template <size_t dac_channel>
  void Update(uint32_t triggers, const int32_t cvs[oc::kNumAdcChannels]) {

    s[0] = SCALE8_16(static_cast<int32_t>(get_gravity()));
    s[1] = SCALE8_16(static_cast<int32_t>(get_bounce_loss()));
    s[2] = SCALE8_16(static_cast<int32_t>(get_initial_amplitude()));
    s[3] = SCALE8_16(static_cast<int32_t>(get_initial_velocity()));
    s[4] = SCALE8_16(static_cast<int32_t>(get_retrigger_bounces()));

    apply_cv_mapping(BB_SETTING_CV1, cvs, s);
    apply_cv_mapping(BB_SETTING_CV2, cvs, s);
    apply_cv_mapping(BB_SETTING_CV3, cvs, s);
    apply_cv_mapping(BB_SETTING_CV4, cvs, s);

    s[0] = USAT16(s[0]);
    s[1] = USAT16(s[1]);
    s[2] = USAT16(s[2]);
    s[3] = USAT16(s[3]);
    s[4] = USAT16(s[4]);

    bb_.Configure(s) ;

    // hard reset forces the bouncing ball to start at level_[0] on rising gate.
    bb_.set_hard_reset(get_hard_reset());

    oc::DigitalInput trigger_input = get_trigger_input();
    uint8_t gate_state = 0;
    if (triggers & DIGITAL_INPUT_MASK(trigger_input))
      gate_state |= peaks::CONTROL_GATE_RISING;

    bool gate_raised = oc::DigitalInputs::read_immediate(trigger_input);
    if (gate_raised)
      gate_state |= peaks::CONTROL_GATE;
    else if (gate_raised_)
      gate_state |= peaks::CONTROL_GATE_FALLING;
    gate_raised_ = gate_raised;

    // TODO Scale range or offset?
    uint32_t value = oc::DAC::get_zero_offset(dac_channel) + bb_.ProcessSingleSample(gate_state, oc::DAC::MAX_VALUE - oc::DAC::get_zero_offset(dac_channel));
    oc::DAC::set<dac_channel>(value);
  }


private:
  peaks::BouncingBall bb_;
  bool gate_raised_;
  int32_t s[kMaxBouncingBallParameters];

};

void BouncingBall::Init(oc::DigitalInput default_trigger) {
  InitDefaults();
  apply_value(BB_SETTING_TRIGGER_INPUT, default_trigger);
  bb_.Init();
  gate_raised_ = false;
}

const char* const bb_cv_mapping_names[BB_CV_MAPPING_LAST] = {
  "off", "grav", "bnce", "ampl", "vel", "retr"
};

// TOTAL EEPROM SIZE: 4 * 9 bytes
SETTINGS_DECLARE(BouncingBall, BB_SETTING_LAST) {
  { 128, 0, 255, "Gravity", NULL, settings::STORAGE_TYPE_U8 },
  { 96, 0, 255, "Bounce loss", NULL, settings::STORAGE_TYPE_U8 },
  { 0, 0, 255, "Amplitude", NULL, settings::STORAGE_TYPE_U8 },
  { 228, 0, 255, "Velocity", NULL, settings::STORAGE_TYPE_U8 },
  { 0, 0, 3, "Trigger input", oc::Strings::trigger_input_names, settings::STORAGE_TYPE_U8 },
  { 0, 0, 255, "Retrigger", NULL, settings::STORAGE_TYPE_U8 },
  { BB_CV_MAPPING_NONE, BB_CV_MAPPING_NONE, BB_CV_MAPPING_LAST - 1, "CV1 -> ", bb_cv_mapping_names, settings::STORAGE_TYPE_U4 },
  { BB_CV_MAPPING_NONE, BB_CV_MAPPING_NONE, BB_CV_MAPPING_LAST - 1, "CV2 -> ", bb_cv_mapping_names, settings::STORAGE_TYPE_U4 },
  { BB_CV_MAPPING_NONE, BB_CV_MAPPING_NONE, BB_CV_MAPPING_LAST - 1, "CV3 -> ", bb_cv_mapping_names, settings::STORAGE_TYPE_U4 },
  { BB_CV_MAPPING_NONE, BB_CV_MAPPING_NONE, BB_CV_MAPPING_LAST - 1, "CV4 -> ", bb_cv_mapping_names, settings::STORAGE_TYPE_U4 },
  { 0, 0, 1, "Hard reset", oc::Strings::no_yes, settings::STORAGE_TYPE_U8 },
};

class QuadBouncingBalls {
public:
  static constexpr int32_t kCvSmoothing = 16;

  // bb = env, balls_ = envelopes_, BouncingBall = EnvelopeGenerator
  // QuadBouncingBalls = QuadEnvelopeGenerator, bbgen = envgen, BBGEN = ENVGEN

  void Init() {
    int input = 0;
    for (auto &bb : balls_) {
      bb.Init(static_cast<oc::DigitalInput>(input));
      ++input;
    }

    ui.left_encoder_value = 0;
    ui.left_edit_mode = MODE_EDIT_SETTINGS;
    ui.selected_channel = 0;
    ui.selected_segment = 0;
    ui.cursor.Init(BB_SETTING_GRAVITY, BB_SETTING_LAST - 1);
  }

  void ISR() {
    cv1.push(oc::ADC::value(0));
    cv2.push(oc::ADC::value(1));
    cv3.push(oc::ADC::value(2));
    cv4.push(oc::ADC::value(3));

    const int32_t cvs[oc::kNumAdcChannels] = { cv1.value(), cv2.value(), cv3.value(), cv4.value() };
    uint32_t triggers = oc::DigitalInputs::clocked();

    balls_[0].Update<0>(triggers, cvs);
    balls_[1].Update<1>(triggers, cvs);
    balls_[2].Update<2>(triggers, cvs);
    balls_[3].Update<3>(triggers, cvs);
  }

  enum LeftEditMode {
    MODE_SELECT_CHANNEL,
    MODE_EDIT_SETTINGS
  };

  struct {
    LeftEditMode left_edit_mode;
    int left_encoder_value;

    int selected_channel;
    int selected_segment;
    menu::ScreenCursor<menu::kScreenLines> cursor;
  } ui;

  BouncingBall &selected() {
    return balls_[ui.selected_channel];
  }

  BouncingBall balls_[4];

  SmoothedValue<int32_t, kCvSmoothing> cv1;
  SmoothedValue<int32_t, kCvSmoothing> cv2;
  SmoothedValue<int32_t, kCvSmoothing> cv3;
  SmoothedValue<int32_t, kCvSmoothing> cv4;
};

QuadBouncingBalls bbgen;

void BBGEN_init() {
  bbgen.Init();
}

size_t BBGEN_storageSize() {
  return 4 * BouncingBall::storageSize();
}

size_t BBGEN_save(void *storage) {
  size_t s = 0;
  for (auto &bb : bbgen.balls_)
    s += bb.Save(static_cast<byte *>(storage) + s);
  return s;
}

size_t BBGEN_restore(const void *storage) {
  size_t s = 0;
  for (auto &bb : bbgen.balls_)
    s += bb.Restore(static_cast<const byte *>(storage) + s);
  return s;
}

void BBGEN_handleAppEvent(oc::AppEvent event) {
  switch (event) {
    case oc::APP_EVENT_RESUME:
      bbgen.ui.cursor.set_editing(false);
      break;
    case oc::APP_EVENT_SUSPEND:
    case oc::APP_EVENT_SCREENSAVER_ON:
    case oc::APP_EVENT_SCREENSAVER_OFF:
      break;
  }
}

void BBGEN_loop() {
}

void BBGEN_menu() {

  menu::QuadTitleBar::Draw();
  for (uint_fast8_t i = 0; i < 4; ++i) {
    menu::QuadTitleBar::SetColumn(i);
    graphics.print((char)('A' + i));
  }
  menu::QuadTitleBar::Selected(bbgen.ui.selected_channel);

  auto const &bb = bbgen.selected();
  menu::SettingsList<menu::kScreenLines, 0, menu::kDefaultValueX> settings_list(bbgen.ui.cursor);
  menu::SettingsListItem list_item;
  while (settings_list.available()) {
    const int current = settings_list.Next(list_item);
    list_item.DrawDefault(bb.get_value(current), BouncingBall::value_attr(current));
  }
}

void BBGEN_topButton() {
  auto &selected_bb = bbgen.selected();
  selected_bb.change_value(BB_SETTING_GRAVITY + bbgen.ui.selected_segment, 32);
}

void BBGEN_lowerButton() {
  auto &selected_bb = bbgen.selected();
  selected_bb.change_value(BB_SETTING_GRAVITY + bbgen.ui.selected_segment, -32);
}

void BBGEN_handleButtonEvent(const UI::Event &event) {
  if (UI::EVENT_BUTTON_PRESS == event.type) {
    switch (event.control) {
      case oc::CONTROL_BUTTON_UP:
        BBGEN_topButton();
        break;
      case oc::CONTROL_BUTTON_DOWN:
        BBGEN_lowerButton();
        break;
      case oc::CONTROL_BUTTON_L:
        break;
      case oc::CONTROL_BUTTON_R:
        bbgen.ui.cursor.toggle_editing();
        break;
    }
  }
}

void BBGEN_handleEncoderEvent(const UI::Event &event) {

  if (oc::CONTROL_ENCODER_L == event.control) {
    int left_value = bbgen.ui.selected_channel + event.value;
    CONSTRAIN(left_value, 0, 3);
    bbgen.ui.selected_channel = left_value;
  } else if (oc::CONTROL_ENCODER_R == event.control) {
    if (bbgen.ui.cursor.editing()) {
      auto &selected = bbgen.selected();
      selected.change_value(bbgen.ui.cursor.cursor_pos(), event.value);
    } else {
      bbgen.ui.cursor.Scroll(event.value);
    }
  }
}

#ifdef BBGEN_DEBUG
void BBGEN_debug() {
  graphics.setPrintPos(2, 12);
  graphics.print(bbgen.cv1.value());
  graphics.setPrintPos(32, 12);
  graphics.print(bbgen.balls_[0].get_channel_retrigger_bounces());
  graphics.setPrintPos(2, 22);
  graphics.print(bbgen.cv2.value());
  graphics.setPrintPos(2, 32);
  graphics.print(bbgen.cv3.value());
  graphics.setPrintPos(2, 42);
  graphics.print(bbgen.cv4.value());
}
#endif // BBGEN_DEBUG

void BBGEN_screensaver() {
  oc::scope_render();
}

void FASTRUN BBGEN_isr() {
  bbgen.ISR();
}

#endif