#ifndef OC_GPIO_H_
#define OC_GPIO_H_
#include <Arduino.h>
#include "oc/options.h"

#ifdef FLIP_180
constexpr auto CV4 = 19;
constexpr auto CV3 = 18;
constexpr auto CV2 = 20;
constexpr auto CV1 = 17;
  
constexpr auto TR4 = 0;
constexpr auto TR3 = 1;
constexpr auto TR2 = 2;
constexpr auto TR1 = 3;
  
constexpr auto but_top = 4;
constexpr auto but_bot = 5;
#else
constexpr auto CV1 = 19;
constexpr auto CV2 = 18;
constexpr auto CV3 = 20;
constexpr auto CV4 = 17;
  
constexpr auto TR1 = 0;
constexpr auto TR2 = 1;
constexpr auto TR3 = 2;
constexpr auto TR4 = 3;
  
constexpr auto but_top = 5;
constexpr auto but_bot = 4;
#endif

#define OLED_DC 6
#define OLED_RST 7
#define OLED_CS 8

// OLED CS is active low
#define OLED_CS_ACTIVE LOW
#define OLED_CS_INACTIVE HIGH

#define DAC_CS 10

#ifdef VOR
  #define but_mid 9
#else
  #define DAC_RST 9
#endif

// NOTE: encoder pins R1/R2 changed for rev >= 2c
#ifdef FLIP_180
  #define encL1 16
  #define encL2 15
  #define butL  14
  
  #define encR1 22
  #define encR2 21
  #define butR  23
#else
  #define encR1 16
  #define encR2 15
  #define butR  14
  
  #define encL1 22
  #define encL2 21
  #define butL  23
#endif

// NOTE: back side :(
#define OC_GPIO_DEBUG_PIN1 24
#define OC_GPIO_DEBUG_PIN2 25

#define OC_GPIO_BUTTON_PINMODE INPUT_PULLUP
#define OC_GPIO_TRx_PINMODE INPUT_PULLUP
#define OC_GPIO_ENC_PINMODE INPUT_PULLUP

/* local copy of pinMode (cf. cores/pins_teensy.c), using faster slew rate */

namespace oc { 
  
void inline pinMode(uint8_t pin, uint8_t mode) {
  
    volatile uint32_t *config;
  
    if (pin >= CORE_NUM_DIGITAL) return;
    config = portConfigRegister(pin);
  
    if (mode == OUTPUT || mode == OUTPUT_OPENDRAIN) {
  #ifdef KINETISK
      *portModeRegister(pin) = 1;
  #else
      *portModeRegister(pin) |= digitalPinToBitMask(pin); // TODO: atomic
  #endif
      /* use fast slew rate for output */
      *config = PORT_PCR_DSE | PORT_PCR_MUX(1);
      if (mode == OUTPUT_OPENDRAIN) {
          *config |= PORT_PCR_ODE;
      } else {
          *config &= ~PORT_PCR_ODE;
                  }
    } else {
  #ifdef KINETISK
      *portModeRegister(pin) = 0;
  #else
      *portModeRegister(pin) &= ~digitalPinToBitMask(pin);
  #endif
      if (mode == INPUT) {
        *config = PORT_PCR_MUX(1);
      } else if (mode == INPUT_PULLUP) {
        *config = PORT_PCR_MUX(1) | PORT_PCR_PE | PORT_PCR_PS;
      } else if (mode == INPUT_PULLDOWN) {
        *config = PORT_PCR_MUX(1) | PORT_PCR_PE;
      } else { // INPUT_DISABLE
        *config = 0;
      }
    }
  }
}

#endif // OC_GPIO_H_
