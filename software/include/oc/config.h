#ifndef OC_CONFIG_H_
#define OC_CONFIG_H_
#include <stdint.h>
#include <stddef.h>

static constexpr uint32_t OC_CORE_ISR_FREQ = 16666U;
static constexpr uint32_t OC_CORE_TIMER_RATE = (1000000UL / OC_CORE_ISR_FREQ);

static constexpr unsigned long REDRAW_TIMEOUT_MS = 1;
static constexpr uint32_t SCREENSAVER_TIMEOUT_S = 25; // default time out menu (in s)
static constexpr uint32_t SCREENSAVER_TIMEOUT_MAX_S = 120;

namespace oc {
static constexpr size_t kMaxTriggerDelayTicks = 96;
};

#define OCTAVES 10      // # octaves
#define SEMITONES (OCTAVES * 12)

static constexpr unsigned long SPLASHSCREEN_DELAY_MS = 100; // HS deprecates splash screen, but keep a slight delay

static constexpr unsigned long APP_SELECTION_TIMEOUT_MS = 25000;
static constexpr unsigned long SETTINGS_SAVE_TIMEOUT_MS = 1000;

#define OC_UI_DEBUG
#define OC_UI_SEPARATE_ISR

#define OC_ENCODERS_ENABLE_ACCELERATION_DEFAULT true

#define OC_CALIBRATION_DEFAULT_FLAGS (0)

/* ------------ uncomment line below to print boot-up and settings saving/restore info to serial ----- */
//#define PRINT_DEBUG
/* ------------ uncomment line below to enable ASR debug page ---------------------------------------- */
//#define ASR_DEBUG
/* ------------ uncomment line below to enable POLYLFO debug page ------------------------------------ */
//#define POLYLFO_DEBUG
/* ------------ uncomment line below to enable BBGEN debug page -------------------------------------- */
//#define BBGEN_DEBUG
/* ------------ uncomment line below to enable ENVGEN debug page ------------------------------------- */
//#define ENVGEN_DEBUG
//#define ENVGEN_DEBUG_SCREENSAVER
/* ------------ uncomment line below to enable BYTEBEATGEN debug page -------------------------------- */
//#define BYTEBEATGEN_DEBUG
/* ------------ uncomment line below to enable H1200 debug page -------------------------------------- */
//#define H1200_DEBUG
/* ------------ uncomment line below to enable QQ debug page ----------------------------------------- */
//#define QQ_DEBUG
//#define QQ_DEBUG_SCREENSAVER

#endif // OC_CONFIG_H_
