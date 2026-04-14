#pragma once
// ── Module: Performance test ──────────────────────────────────────────────────
// 0→100 km/h acceleration and 100→0 km/h braking timers.
//
// State machine (per test):
//   0 IDLE    — not armed; waiting for user to ARM via /api/perf
//   1 ARMED   — waiting for launch/brake trigger
//   2 RUNNING — timing in progress
//   3 DONE    — result available in cfg.perfAccelMs / cfg.perfBrakeMs
//
// Triggers (using telemetry accessors from mod_telemetry.h):
//   0→100 launch : speed leaves 5 km/h (speedRaw ≥ 50) with positive torque
//   0→100 done   : speed ≥ 100 km/h (speedRaw ≥ 1000)
//   0→100 abort  : speed drops back below 2 km/h (false start) → back to ARMED
//
//   100→0 launch : speed ≥ 95 km/h (speedRaw ≥ 950) AND brake pressed
//   100→0 done   : speed ≤ 2 km/h (speedRaw ≤ 20)
//   100→0 abort  : brake released while speed still > 20 km/h → back to ARMED

#include "fsd_config.h"

extern FSDConfig cfg;

static uint32_t perfAccelStartMs = 0;
static uint32_t perfBrakeStartMs = 0;

// Called from canTask after each handleMessage, using current telemetry values.
// speedRaw = kph×10  (e.g. 1000 = 100 km/h)
inline void updatePerfTest(uint16_t speedRaw, int16_t torqueRear, bool brake) {
    // ── 0→100 acceleration ────────────────────────────────────────────────
    switch (cfg.perfAccelState) {
        case 1: // ARMED — waiting for launch
            // Start at ~1 km/h with positive torque to minimise the 0→launch gap
            if (speedRaw >= 10 && torqueRear > 0) {
                cfg.perfAccelState  = 2;
                perfAccelStartMs    = millis();
            }
            break;
        case 2: // RUNNING
            if (speedRaw >= 1000) {                         // hit 100 km/h
                cfg.perfAccelState = 3;
                cfg.perfAccelMs    = millis() - perfAccelStartMs;
            } else if (speedRaw < 5) {                      // false start — rearm
                cfg.perfAccelState = 1;
            }
            break;
        default: break;
    }

    // ── 100→0 braking ────────────────────────────────────────────────────
    switch (cfg.perfBrakeState) {
        case 1: // ARMED — waiting for high-speed brake press
            if (speedRaw >= 950 && brake) {
                cfg.perfBrakeState      = 2;
                cfg.perfBrakeEntryKph   = (uint8_t)(speedRaw / 10);  // kph (e.g. 970 → 97)
                perfBrakeStartMs        = millis();
            }
            break;
        case 2: // RUNNING
            if (speedRaw <= 20) {                           // stopped
                cfg.perfBrakeState = 3;
                cfg.perfBrakeMs    = millis() - perfBrakeStartMs;
            } else if (!brake && speedRaw > 200) {          // released brake too early
                cfg.perfBrakeState = 1;
            }
            break;
        default: break;
    }
}
