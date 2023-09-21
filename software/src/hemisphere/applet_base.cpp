#include "hemisphere/applet_base.hpp"

using namespace hemisphere;

int hemisphere::octave_max = 5;

uint8_t AppletBase::modal_edit_mode = 2; // 0=old behavior, 1=modal editing, 2=modal with wraparound
uint8_t AppletBase::trig_length = 2; // multiplier for HEMISPHERE_CLOCK_TICKS
int AppletBase::inputs[4];
int AppletBase::outputs[4];
int AppletBase::outputs_smooth[4];
int AppletBase::clock_countdown[4];
int AppletBase::adc_lag_countdown[4];
uint32_t AppletBase::last_clock[4];
uint32_t AppletBase::cycle_ticks[4];
bool AppletBase::changed_cv[4];
int AppletBase::last_cv[4];
int AppletBase::cursor_countdown[2];

void AppletBase::BaseStart(bool hemisphere_) {
  hemisphere = hemisphere_;

  // Initialize some things for startup
  ForEachChannel(ch) {
    clock_countdown[io_offset + ch] = 0;
    inputs[io_offset + ch] = 0;
    outputs[io_offset + ch] = 0;
    outputs_smooth[io_offset + ch] = 0;
    adc_lag_countdown[io_offset + ch] = 0;
  }
  help_active = 0;
  cursor_countdown[hemisphere] = HEMISPHERE_CURSOR_TICKS;

  // Shutdown FTM capture on Digital 4, used by Tuner
#ifdef FLIP_180
  if (hemisphere == 0)
#else
  if (hemisphere == 1)
#endif
  {
    FreqMeasure.end();
    oc::DigitalInputs::reInit();
  }

  // Maintain previous app state by skipping Start
  if (!applet_started) {
    applet_started = true;
    Start();
  }
}

void AppletBase::BaseController(bool master_clock_on) {
  master_clock_bus = (master_clock_on && hemisphere == RIGHT_HEMISPHERE);
  ForEachChannel(ch) {
    // Set CV inputs
    size_t channel = (size_t)(ch + io_offset);
    inputs[channel] = oc::ADC::raw_pitch_value(channel);
    if (abs(inputs[channel] - last_cv[channel]) > HEMISPHERE_CHANGE_THRESHOLD) {
      changed_cv[channel] = 1;
      last_cv[channel] = inputs[channel];
    } else
      changed_cv[channel] = 0;

    // Handle clock timing
    if (clock_countdown[channel] > 0) {
      if (--clock_countdown[channel] == 0) Out(ch, 0);
    }
  }

  // Cursor countdowns. See CursorBlink(), ResetCursor(), gfxCursor()
  if (--cursor_countdown[hemisphere] < -HEMISPHERE_CURSOR_TICKS)
    cursor_countdown[hemisphere] = HEMISPHERE_CURSOR_TICKS;

  Controller();
}

void AppletBase::DrawHelpScreen() {
  gfxHeader(applet_name());
  SetHelp();
  for (int section = 0; section < 4; section++) {
    int y = section * 12 + 16;
    graphics.setPrintPos(0, y);
    if (section == HEMISPHERE_HELP_DIGITALS) graphics.print("Dig");
    if (section == HEMISPHERE_HELP_CVS) graphics.print("CV");
    if (section == HEMISPHERE_HELP_OUTS) graphics.print("Out");
    if (section == HEMISPHERE_HELP_ENCODER) graphics.print("Enc");
    graphics.invertRect(0, y - 1, 19, 9);

    graphics.setPrintPos(20, y);
    graphics.print(help[section]);
  }
}

// handle modal edit mode toggle or cursor advance
void AppletBase::CursorAction(int &cursor, int max) {
  if (modal_edit_mode) {
    isEditing = !isEditing;
  } else {
    cursor++;
    cursor %= max + 1;
    ResetCursor();
  }
}

void AppletBase::MoveCursor(int &cursor, int direction, int max) {
  cursor += direction;
  if (modal_edit_mode == 2) {  // wrap cursor
    if (cursor < 0)
      cursor = max;
    else
      cursor %= max + 1;
  } else {
    cursor = constrain(cursor, 0, max);
  }
  ResetCursor();
}

int AppletBase::Proportion(int numerator, int denominator, int max_value) {
  simfloat proportion = int2simfloat((int32_t)numerator) / (int32_t)denominator;
  int scaled = simfloat2int(proportion * max_value);
  return scaled;
}

int AppletBase::ProportionCV(int cv_value, int max_pixels) {
  int prop = constrain(
      Proportion(cv_value, HEMISPHERE_MAX_INPUT_CV, max_pixels), 0, max_pixels);
  return prop;
}