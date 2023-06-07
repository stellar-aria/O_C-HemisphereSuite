#include "HemisphereApplet.h"

int HS::octave_max = 5;

uint8_t HemisphereApplet::modal_edit_mode = 2; // 0=old behavior, 1=modal editing, 2=modal with wraparound
uint8_t HemisphereApplet::trig_length = 2; // multiplier for HEMISPHERE_CLOCK_TICKS
int HemisphereApplet::inputs[4];
int HemisphereApplet::outputs[4];
int HemisphereApplet::outputs_smooth[4];
int HemisphereApplet::clock_countdown[4];
int HemisphereApplet::adc_lag_countdown[4];
uint32_t HemisphereApplet::last_clock[4];
uint32_t HemisphereApplet::cycle_ticks[4];
bool HemisphereApplet::changed_cv[4];
int HemisphereApplet::last_cv[4];
int HemisphereApplet::cursor_countdown[2];