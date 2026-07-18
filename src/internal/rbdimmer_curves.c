/**
 * @file rbdimmer_curves.c
 * @brief Brightness curve lookup tables and level-to-delay conversion
 * @internal
 *
 * Pure math — no GPIO, no timers, no FreeRTOS.
 * Tables are computed once at init time to avoid floating-point math in the
 * ISR path.
 */

#include "rbdimmer_curves.h"
#include <math.h>

// ---------------------------------------------------------------------------
// Lookup tables (101 entries: indices 0..100 = brightness percent)
// ---------------------------------------------------------------------------

static uint8_t table_linear[101];
static uint8_t table_rms[101];
static uint8_t table_log[101];

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void rbdimmer_curves_init(void) {
    // Linear: delay is simply (100 - level)%
    for (int i = 0; i <= 100; i++) {
        table_linear[i] = (uint8_t)(100 - i);
    }

    // RMS-compensated: angle = arccos(sqrt(level_normalized)) / pi
    for (int i = 0; i <= 100; i++) {
        float level_normalized = i / 100.0f;
        if (level_normalized <= 0.0f) {
            table_rms[i] = 100;
        } else if (level_normalized >= 1.0f) {
            table_rms[i] = 0;
        } else {
            float angle_rad = acosf(sqrtf(level_normalized));
            float delay_percent = angle_rad / (float)M_PI;
            table_rms[i] = (uint8_t)roundf(delay_percent * 100.0f);
        }
    }

    // Logarithmic: perceptually linear for human eye
    for (int i = 0; i <= 100; i++) {
        float level_normalized = i / 100.0f;
        if (level_normalized <= 0.0f) {
            table_log[i] = 100;
        } else if (level_normalized >= 1.0f) {
            table_log[i] = 0;
        } else {
            float log_value = log10f(1.0f + 9.0f * level_normalized) / log10f(10.0f);
            float delay_percent = 1.0f - log_value;
            table_log[i] = (uint8_t)roundf(delay_percent * 100.0f);
        }
    }
}

uint32_t rbdimmer_curves_level_to_delay(uint16_t level_percent,
                                         uint32_t half_cycle_us) {
    // Configurable via Kconfig; Arduino fallback values match tested defaults.
#define LEVEL_MAX_PCT  9800
#define LEVEL_MIN_PCT  300

    if (level_percent >= 9800) {
        level_percent = LEVEL_MAX_PCT;  // map 100% → max delay (~100 us @ 50 Hz)
    }
    if (level_percent < LEVEL_MIN_PCT) {
        // Too close to end of half-cycle — TRIAC fires unreliably. Treat as OFF.
        return 0;
    }

    //uint32_t delay_percent = level_percent;
    //uint32_t delay_us = (half_cycle_us * delay_percent) / 100;
    uint32_t delay_us = half_cycle_us - level_percent;

    if (delay_us < RBDIMMER_MIN_DELAY_US) {
        delay_us = RBDIMMER_MIN_DELAY_US;
    }
    if (delay_us > half_cycle_us - RBDIMMER_DEFAULT_PULSE_WIDTH_US) {
        delay_us = half_cycle_us - RBDIMMER_DEFAULT_PULSE_WIDTH_US;
    }

    return delay_us;
}
