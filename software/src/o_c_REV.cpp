// Copyright (c) 2015, 2016 Max Stadler, Patrick Dowling
//
// Original Author : Max Stadler
// Heavily modified: Patrick Dowling (pld@gurkenkiste.com)
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

// Main startup/loop for O&C firmware


#include <Arduino.h>
#include <EEPROM.h>

#include "oc/apps.h"
#include "oc/core.h"
#include "oc/DAC.h"
#include "oc/debug.h"
#include "oc/gpio.h"
#include "oc/ADC.h"
#include "oc/calibration.h"
#include "oc/digital_inputs.h"
#include "oc/menus.h"
#include "oc/strings.h"
#include "oc/ui.h"
#include "oc/options.h"
#include "drivers/display.h"
#include "util/debugpins.h"
#include "VBiasManager.h"

unsigned long LAST_REDRAW_TIME = 0;
uint_fast8_t MENU_REDRAW = true;
oc::UiMode ui_mode = oc::UI_MODE_MENU;
const bool DUMMY = false;

/*  ------------------------ UI timer ISR ---------------------------   */

IntervalTimer UI_timer;

void FASTRUN UI_timer_ISR() {
  OC_DEBUG_PROFILE_SCOPE(oc::DEBUG::UI_cycles);
  oc::ui.Poll();
  OC_DEBUG_RESET_CYCLES(oc::ui.ticks(), 2048, oc::DEBUG::UI_cycles);
}

/*  ------------------------ core timer ISR ---------------------------   */
IntervalTimer CORE_timer;
volatile bool oc::core::app_isr_enabled = false;
volatile uint32_t oc::core::ticks = 0;

void FASTRUN CORE_timer_ISR() {
  DEBUG_PIN_SCOPE(OC_GPIO_DEBUG_PIN2);
  OC_DEBUG_PROFILE_SCOPE(oc::DEBUG::ISR_cycles);

  // DAC and display share SPI. By first updating the DAC values, then starting
  // a DMA transfer to the display things are fairly nicely interleaved. In the
  // next ISR, the display transfer is finalized (CS update).

  display::Flush();
  oc::DAC::Update();
  display::Update();

  // The ADC scan uses async startSingleRead/readSingle and single channel each
  // loop, so should be fast enough even at 60us (check ADC::busy_waits() == 0)
  // to verify. Effectively, the scan rate is ISR / 4 / ADC::kAdcSmoothing
  // 100us: 10kHz / 4 / 4 ~ .6kHz
  // 60us: 16.666K / 4 / 4 ~ 1kHz
  // kAdcSmoothing == 4 has some (maybe 1-2LSB) jitter but seems "Good Enough".
  oc::ADC::Scan();

  // Pin changes are tracked in separate ISRs, so depending on prio it might
  // need extra precautions.
  oc::DigitalInputs::Scan();

#ifndef OC_UI_SEPARATE_ISR
  TODO needs a counter
  UI_timer_ISR();
#endif

  ++oc::core::ticks;
  if (oc::core::app_isr_enabled)
    oc::apps::ISR();

  OC_DEBUG_RESET_CYCLES(oc::core::ticks, 16384, oc::DEBUG::ISR_cycles);
}

/*       ---------------------------------------------------------         */

void setup() {
  delay(50);
  NVIC_SET_PRIORITY(IRQ_PORTB, 0); // TR1 = 0 = PTB16
  SPI_init();
  SERIAL_PRINTLN("* O&C BOOTING...");
  SERIAL_PRINTLN("* %s", OC_VERSION);

  oc::DEBUG::Init();
  oc::DigitalInputs::Init();
  delay(400); 
  oc::ADC::Init(&oc::calibration_data.adc); // Yes, it's using the calibration_data before it's loaded...
  oc::DAC::Init(&oc::calibration_data.dac);

  display::Init();

  GRAPHICS_BEGIN_FRAME(true);
  GRAPHICS_END_FRAME();

  calibration_load();
  
  display::AdjustOffset(oc::calibration_data.display_offset);

  oc::menu::Init();
  oc::ui.Init();
  oc::ui.configure_encoders(oc::calibration_data.encoder_config());

  SERIAL_PRINTLN("* CORE ISR @%luus", OC_CORE_TIMER_RATE);
  CORE_timer.begin(CORE_timer_ISR, OC_CORE_TIMER_RATE);
  CORE_timer.priority(OC_CORE_TIMER_PRIO);

#ifdef OC_UI_SEPARATE_ISR
  SERIAL_PRINTLN("* UI ISR @%luus", OC_UI_TIMER_RATE);
  UI_timer.begin(UI_timer_ISR, OC_UI_TIMER_RATE);
  UI_timer.priority(OC_UI_TIMER_PRIO);
#endif

  // Display splash screen and optional calibration
  bool reset_settings = false;
  ui_mode = oc::ui.Splashscreen(reset_settings);

  if (ui_mode == oc::UI_MODE_CALIBRATE) {
    oc::ui.Calibrate();
    ui_mode = oc::UI_MODE_MENU;
  }
  oc::ui.set_screensaver_timeout(oc::calibration_data.screensaver_timeout);

  // initialize apps
  oc::apps::Init(reset_settings);

#ifdef VOR
  VBiasManager *vbias_m = vbias_m->get();
  vbias_m->ChangeBiasToState(VBiasManager::BI);
#endif
}

/*  ---------    main loop  --------  */

void FASTRUN loop() {

  oc::core::app_isr_enabled = true;
  uint32_t menu_redraws = 0;
  while (true) {

    // don't change current_app while it's running
    if (oc::UI_MODE_APP_SETTINGS == ui_mode) {
      oc::ui.AppSettings();
      ui_mode = oc::UI_MODE_MENU;
    }

    // Refresh display
    if (MENU_REDRAW) {
      GRAPHICS_BEGIN_FRAME(false); // Don't busy wait
        if (oc::UI_MODE_MENU == ui_mode) {
          OC_DEBUG_RESET_CYCLES(menu_redraws, 512, oc::DEBUG::MENU_draw_cycles);
          OC_DEBUG_PROFILE_SCOPE(oc::DEBUG::MENU_draw_cycles);
          oc::apps::current_app->DrawMenu();
          ++menu_redraws;

          #ifdef VOR
          // JEJ:On app screens, show the bias popup, if necessary
          VBiasManager *vbias_m = vbias_m->get();
          vbias_m->DrawPopupPerhaps();
          #endif

        } else {
          oc::apps::current_app->DrawScreensaver();
        }
        MENU_REDRAW = 0;
        LAST_REDRAW_TIME = millis();
      GRAPHICS_END_FRAME();
    }

    // Run current app
    oc::apps::current_app->loop();

    // UI events
    oc::UiMode mode = oc::ui.DispatchEvents(oc::apps::current_app);

    // State transition for app
    if (mode != ui_mode) {
      if (oc::UI_MODE_SCREENSAVER == mode)
        oc::apps::current_app->HandleAppEvent(oc::APP_EVENT_SCREENSAVER_ON);
      else if (oc::UI_MODE_SCREENSAVER == ui_mode)
        oc::apps::current_app->HandleAppEvent(oc::APP_EVENT_SCREENSAVER_OFF);
      ui_mode = mode;
    }

    if (millis() - LAST_REDRAW_TIME > REDRAW_TIMEOUT_MS)
      MENU_REDRAW = 1;
  }
}


