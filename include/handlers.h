#pragma once
// ── handlers.h — Unified CAN dispatch ─────────────────────────────────────
// This file is the only CAN-related header included by main.cpp.
// It owns the single FSDConfig instance and wires up all sub-modules.
//
// Module layout:
//   fsd_config.h    — FSDConfig struct (shared state, no logic)
//   mod_bms.h       — BMS telemetry parsers (read-only, 0x132/0x292/0x312)
//   mod_fsd.h       — FSD injection handlers: Legacy/HW3/HW4 + filter tables
//   mod_telemetry.h — Ring buffer + parsers: 0x257/0x145/0x118/0x108 (read-only)
//   mod_climate.h   — Climate telemetry: 0x28B BODY_R1 (AirTemp inside/outside)
//   handlers.h      — cfg instance + handleMessage dispatch (this file)

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
#include "mod_log.h"
#include "mod_bms.h"
#include "mod_fsd.h"
#include "mod_telemetry.h"
#include "mod_lighting.h"
#include "mod_das_status.h"
#include "mod_climate.h"
#include "mod_perf.h"

#ifdef WIFI_BRIDGE_ENABLED
// Single definition of the DNS hook in this TU (main.cpp).
// lwip_hooks.h (invoked by IDF lwIP C code) sees only the extern "C" decl.
#define MOD_DNS_IMPLEMENTATION
#include "mod_dns.h"
#include "mod_thermal.h"
#include "mod_wifi_bridge.h"
#endif

// ── Unified dispatch ───────────────────────────────────────────────────────
// Called for every received CAN frame. Routes to the appropriate module.
// NOTE: cfg.rxCount is *not* incremented here — canTask increments the
// per-bus counter (rxCount / vhRxCount / prtyRxCount) before dispatching,
// so party-bus silence isn't masked by VH/PRTY activity.
static void handleMessage(CanFrame& frame, CanDriver& driver) {

    // HW detection: GTW_carConfig 0x398 (920)
    // Informational only — does NOT override the user-selected hwMode.
    if (frame.id == 920) {
        if (frame.dlc < 1) return;
        uint8_t das_hw = (frame.data[0] >> 6) & 0x03;
        if      (das_hw == 2) cfg.hwDetected = 1;  // HW3
        else if (das_hw == 3) cfg.hwDetected = 2;  // HW4
        return;
    }

    // GTW_autopilot 0x7FF (2047) mux-2 — AP type telemetry (HW3/HW4, read-only)
    if (frame.id == 2047) {
        if (frame.dlc < 6) return;
        if (readMuxID(frame) != 2) return;
        cfg.gatewayAutopilot = (int8_t)readGTWAutopilot(frame);
        return;
    }

    // BMS telemetry — read-only, no frames transmitted
    if (frame.id == 306) { handleBMSHV(frame);      return; }  // 0x132
    if (frame.id == 658) { handleBMSSOC(frame);     return; }  // 0x292
    if (frame.id == 786) { handleBMSThermal(frame); return; }  // 0x312

    // Telemetry — read-only, opendbc tesla_model3_party.dbc verified signals
    if (frame.id == 599) { handleSpeed(frame);  return; }  // 0x257 DI_speed (DI_vehicleSpeed)
    if (frame.id == 325) { handleBrake(frame);  return; }  // 0x145 ESP_status (brake pedal)
    if (frame.id == 280) { handleGear(frame);   return; }  // 0x118 DI_systemStatus (DI_gear)
    if (frame.id == 264) { handleTorque(frame); return; }  // 0x108 DI_torque (cmd/actual)
    if (frame.id == 643) { handleClimate(frame);               return; }  // 0x28B BODY_R1 (AirTemp)
    if (frame.id == 659) { handleDASSettings(frame);          return; }  // 0x293 DAS_settings
    if (frame.id == 923) { handleDASStatus(frame);            return; }  // 0x39B DAS_status
    if (frame.id == 905) { handleDASStatus2(frame);           return; }  // 0x389 DAS_status2
    if (frame.id == 585) { handleSCCMStalk(frame, driver);    return; }  // 0x249 SCCM_leftStalk
    // 0x399 (921) DAS_status_ISA — ISA chime only, NOT a speed limit source.
    // Speed limits come exclusively from 0x39B (923) via handleDASStatus above.

    // FSD injection — route to hardware-specific handler
    if (!isFilteredId(frame.id)) return;
    switch (cfg.hwMode) {
        case 0:  handleLegacy(frame, driver); break;
        case 1:  handleHW3(frame, driver);    break;
        default: handleHW4(frame, driver);    break;
    }
}
