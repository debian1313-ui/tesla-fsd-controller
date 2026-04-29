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
//   mod_climate.h   — Climate telemetry: 0x283 BODY_R1 (AirTemp inside/outside)
//   handlers.h      — cfg instance + handleMessage dispatch (this file)

#include "fsd_config.h"
#include "can_frame_types.h"
#include "drivers/can_driver.h"

// JSON-string escape into a bounded destination. Backslash-prefixes `"` and
// `\\` (the only two characters JSON requires); copies everything else
// verbatim. Output is always NUL-terminated; truncates silently if `cap`
// runs out, so caller must size `dst` for at most 2× source + 1 NUL.
//
// Shared between /api/status (apSSID/staSSID/perfModel/carSwVer), the diag
// upload payload (carSwVer), and /api/diag/status (msg) so all firmware
// JSON strings escape uniformly.
static inline void jsonEscapeInto(const char* src, char* dst, size_t cap) {
    if (!dst || cap == 0) return;
    char* out = dst;
    char* end = dst + cap - 3;  // room for one final escape byte + NUL
    while (src && *src && out < end) {
        if (*src == '"' || *src == '\\') *out++ = '\\';
        *out++ = *src++;
    }
    *out = 0;
}

// ── Single shared state instance ──────────────────────────────────────────
// Accessed by Core0 (web/WiFi) and Core1 (CAN task).
// Individual volatile 32-bit reads/writes are atomic on ESP32 Xtensa.
// Defined here (not static) so sub-modules can forward-declare via extern.
// Safe: this project has a single translation unit (main.cpp). No ODR risk.
FSDConfig cfg;  // NOLINT(misc-definitions-in-headers)

// ── Boot timing diagnostics (v1.4.36) ─────────────────────────────────────
// First-seen millis() for key car-readiness frames and the first successful
// FSD injection send. Pure observation — used to triage "FSD→AP at cold boot"
// reports by exposing whether we injected before the car gateway/DAS were
// ready. Logged once each in the 1 Hz diag loop in main.cpp.
static volatile uint32_t bt_first920ms   = 0;  // 0x398 GTW_carConfig
static volatile uint32_t bt_first923ms   = 0;  // 0x39B DAS_status
static volatile uint32_t bt_first1021ms  = 0;  // 0x3FD FSD activation frame
static volatile uint32_t bt_first2047ms  = 0;  // 0x7FF GTW_autopilot
static volatile uint32_t bt_firstFsdMod  = 0;  // first FSD-path driver.send() that returned true
static inline void btMark(volatile uint32_t& slot) {
    if (slot == 0) slot = millis();
}

// Sub-modules (included after cfg is defined so they can use it via extern)
#include "mod_log.h"
#include "mod_diag_collect.h"
#include "mod_bms.h"
#include "mod_fsd.h"
#include "mod_telemetry.h"
#include "mod_lighting.h"
#include "mod_das_status.h"
#include "mod_climate.h"
#include "mod_perf.h"
#include "mod_ota.h"
#include "mod_gps_speed.h"
// Thermal sampling is now universal — needs to be visible in all envs so the
// /api/status JSON can carry chipTempC. Wi-Fi-specific reactions still gate on
// WIFI_BRIDGE_ENABLED below.
#include "mod_thermal.h"

#ifdef WIFI_BRIDGE_ENABLED
// Single definition of the DNS hook in this TU (main.cpp).
// lwip_hooks.h (invoked by IDF lwIP C code) sees only the extern "C" decl.
#define MOD_DNS_IMPLEMENTATION
#include "mod_dns.h"
#include "mod_telemetry_ping.h"
#include "mod_diag_upload.h"
#include "mod_wifi_bridge.h"
#endif

// ── Unified dispatch ───────────────────────────────────────────────────────
// Called for every received CAN frame. Routes to the appropriate module.
// cfg.rxCount is incremented in canTask before dispatching.
static void handleMessage(CanFrame& frame, CanDriver& driver) {
    // Passive snapshot for diagnostic upload — fingerprints car / firmware
    // independently of the injection path. Cheap (one bit + memcpy).
    diagOnFrame(frame.id, frame.data, frame.dlc);

    // Boot timing — first-seen of key car-readiness frames (cheap, no-op after first hit)
    switch (frame.id) {
        case 920:  btMark(bt_first920ms);  break;
        case 923:  btMark(bt_first923ms);  break;
        case 1021: btMark(bt_first1021ms); break;
        case 2047: btMark(bt_first2047ms); break;
    }

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
    if (frame.id == 643) { handleClimate(frame);               return; }  // 0x283 BODY_R1 (AirTemp)
    if (frame.id == 659) { handleDASSettings(frame);          return; }  // 0x293 DAS_settings
    if (frame.id == 923) {                                                // 0x39B DAS_status
        handleDASStatus(frame);
        handleDASStatusISAOverride(frame, driver);  // v1.4.28 ISA override
        return;
    }
    if (frame.id == 905) { handleDASStatus2(frame);           return; }  // 0x389 DAS_status2
    if (frame.id == 585) { handleSCCMStalk(frame, driver);    return; }  // 0x249 SCCM_leftStalk
    // 0x2F8 (760) UI_gpsVehicleSpeed — sniff first, then fall through to isFilteredId()
    // so the Legacy handler can TX-modify the frame (v1.4.27 legacyOffset feature).
    if (frame.id == 760) handleGpsVehicleSpeed(frame);
    // 0x399 (921) DAS_status_ISA — dual-source fused speed limit reader.
    // NOTE: do NOT return here — the HW4 FSD injection handler below also
    // processes 921 for ISA chime suppression. Falls through to isFilteredId().
    if (frame.id == 921) handleDASStatusISA(frame);

    // FSD injection — route to hardware-specific handler
    if (!isFilteredId(frame.id)) return;
    switch (cfg.hwMode) {
        case 0:  handleLegacy(frame, driver); break;
        case 1:  handleHW3(frame, driver);    break;
        default: handleHW4(frame, driver);    break;
    }
}
