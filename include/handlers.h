#pragma once
// ── handlers.h — Unified CAN dispatch ─────────────────────────────────────
// This file is the only CAN-related header included by main.cpp.
// It owns the single FSDConfig instance and wires up all sub-modules.
//
// Module layout:
//   fsd_config.h   — FSDConfig struct (shared state, no logic)
//   mod_bms.h      — BMS telemetry parsers (read-only, 0x132/0x292/0x312)
//   mod_precond.h  — Battery preconditioning frame builder (0x082)
//   mod_fsd.h      — FSD injection handlers: Legacy/HW3/HW4 + filter tables
//   handlers.h     — cfg instance + handleMessage dispatch (this file)

#include "fsd_config.h"
#include "can_frame_types.h"
#include "drivers/can_driver.h"

// ── Single shared state instance ──────────────────────────────────────────
// Accessed by Core0 (web/WiFi) and Core1 (CAN task).
// Individual volatile 32-bit reads/writes are atomic on ESP32 Xtensa.
// Defined here (not static) so sub-modules can forward-declare via extern.
// Safe: this project has a single translation unit (main.cpp). No ODR risk.
FSDConfig cfg;  // NOLINT(misc-definitions-in-headers)

// Sub-modules (included after cfg is defined so they can use it via extern)
#include "mod_bms.h"
#include "mod_precond.h"
#include "mod_fsd.h"

// ── Unified dispatch ───────────────────────────────────────────────────────
// Called for every received CAN frame. Routes to the appropriate module.
static void handleMessage(CanFrame& frame, CanDriver& driver) {
    cfg.rxCount++;

    // HW detection: GTW_carConfig 0x398 (920)
    // Informational only — does NOT override the user-selected hwMode.
    if (frame.id == 920) {
        if (frame.dlc < 1) return;
        uint8_t das_hw = (frame.data[0] >> 6) & 0x03;
        if      (das_hw == 2) cfg.hwDetected = 1;  // HW3
        else if (das_hw == 3) cfg.hwDetected = 2;  // HW4
        return;
    }

    // BMS telemetry — read-only, no frames transmitted
    if (frame.id == 306) { handleBMSHV(frame);      return; }  // 0x132
    if (frame.id == 658) { handleBMSSOC(frame);     return; }  // 0x292
    if (frame.id == 786) { handleBMSThermal(frame); return; }  // 0x312

    // FSD injection — route to hardware-specific handler
    if (!isFilteredId(frame.id)) return;
    switch (cfg.hwMode) {
        case 0:  handleLegacy(frame, driver); break;
        case 1:  handleHW3(frame, driver);    break;
        default: handleHW4(frame, driver);    break;
    }
}
