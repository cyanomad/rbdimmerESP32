/**
 * @file rbdimmer_curves.h
 * @brief Level-to-delay conversion (50 Hz only, no brightness curves)
 * @internal
 *
 * Pure math module — no hardware dependencies.
 *
 * Curve support (linear/RMS/logarithmic lookup tables) was removed.
 * This project targets 50 Hz mains only, so a half-cycle is exactly
 * ~10000 us, and level_percent (0-10000) is used directly as the
 * desired TRIAC conduction time in microseconds - no percentage-to-delay
 * curve needed.
 */

#ifndef RBDIMMER_CURVES_H
#define RBDIMMER_CURVES_H

#include <stdint.h>
#include "rbdimmerESP32.h"   // RBDIMMER_MIN_DELAY_US, RBDIMMER_DEFAULT_PULSE_WIDTH_US

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Convert a level (desired conduction time, in microseconds) to
 *        TRIAC firing delay.
 *
 * @param level_percent  Desired conduction time, 0-10000 us (0 = off,
 *                        ~10000 = fully on for a 50 Hz / ~10000 us half-cycle)
 * @param half_cycle_us  Mains half-cycle duration in microseconds (measured
 *                        at runtime, so may vary slightly around 10000)
 * @return               Delay in microseconds (0 = do not fire this
 *                        half-cycle; otherwise clamped to a safe range)
 */
uint32_t rbdimmer_curves_level_to_delay(uint16_t level_percent,
                                         uint32_t half_cycle_us);

#ifdef __cplusplus
}
#endif

#endif /* RBDIMMER_CURVES_H */
