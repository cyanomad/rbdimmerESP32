/**
 * @file rbdimmer_curves.c
 * @brief Level-to-delay conversion (50 Hz only, no brightness curves)
 * @internal
 *
 * Pure math — no GPIO, no timers, no FreeRTOS.
 *
 * This project targets 50 Hz mains only, so a half-cycle is a fixed
 * ~10000 us. level_percent (0-10000) is used directly as the desired
 * TRIAC conduction time in microseconds - no percentage curve, no
 * lookup table. This intentionally replaces the old linear/RMS/log
 * curve system, which is no longer used anywhere in this project.
 */

#include "rbdimmer_curves.h"

// Reserve headroom at both ends of the range so the TRIAC always gets a
// valid firing delay:
//  - LEVEL_MAX: leaves at least RBDIMMER_MIN_DELAY_US before the *this*
//    zero-cross so the TRIAC/opto has settled before firing.
//  - LEVEL_MIN: below this, the remaining conduction request is so small
//    that firing this half-cycle isn't worth doing reliably - skip it
//    (delay=0 is a sentinel the ISR reads as "do not arm this cycle",
//    see on_zero_cross_phase() in rbdimmer_channel.c - NOT "fire at t=0").
#define LEVEL_MAX_PCT  9800
#define LEVEL_MIN_PCT  300

uint32_t rbdimmer_curves_level_to_delay(uint16_t level_percent,
                                         uint32_t half_cycle_us) {
    // Defensive: half_cycle_us should always be a real measured value
    // (~10000 us at 50 Hz) by the time this is called, but guard against
    // 0 (e.g. called before frequency detection completes) to avoid any
    // unsigned underflow below.
    if (half_cycle_us == 0) {
        return 0;
    }

    if (level_percent >= LEVEL_MAX_PCT) {
        level_percent = LEVEL_MAX_PCT;
    }
    if (level_percent < LEVEL_MIN_PCT) {
        return 0;  // Do not fire this half-cycle (see comment above)
    }

    uint32_t delay_us = half_cycle_us - level_percent;

    if (delay_us < RBDIMMER_MIN_DELAY_US) {
        delay_us = RBDIMMER_MIN_DELAY_US;
    }
    if (half_cycle_us > RBDIMMER_DEFAULT_PULSE_WIDTH_US &&
        delay_us > half_cycle_us - RBDIMMER_DEFAULT_PULSE_WIDTH_US) {
        delay_us = half_cycle_us - RBDIMMER_DEFAULT_PULSE_WIDTH_US;
    }

    return delay_us;
}
