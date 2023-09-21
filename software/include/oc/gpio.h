#pragma once
#include "oc/options.h"
#include "daisy_seed.h"

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
constexpr daisy::Pin OC_GPIO_DEBUG_PIN1 = daisy::seed::D6;
constexpr daisy::Pin OC_GPIO_DEBUG_PIN2 = daisy::seed::D7;

#define OC_GPIO_BUTTON_PINMODE INPUT_PULLUP
#define OC_GPIO_TRx_PINMODE INPUT_PULLUP
#define OC_GPIO_ENC_PINMODE INPUT_PULLUP
