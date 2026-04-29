#pragma once
// ── Module: Diagnostic snapshot collector ────────────────────────────────────
// Lives passively next to handleMessage — every CAN frame ticks two tiny
// pieces of state used by the diag-upload bundle:
//
//   1. Raw 8 bytes of 0x398 GTW_carConfig — encodes chassis type, das_hw,
//      forwardRadarHw, country, rhd, euVehicle, autopilot, etc. We store the
//      raw payload (not parsed fields) so the server can decode any signal
//      later without a firmware rebuild.
//
//   2. seen-IDs bitmap covering CAN IDs 0..2047 (256 bytes). Tesla Party CAN
//      IDs all fall in this range. The set of seen IDs is a strong vehicle
//      fingerprint — distinguishes HW3 vs HW4, model year, region — without
//      reaching for software version.
//
// Both are read at upload time only. No NVS — fingerprint reconstructs
// from CAN traffic within seconds of boot.

#include <cstdint>
#include <cstring>

struct DiagCarConfig {
    bool    seen;
    uint8_t bytes[8];
};

// `static` here means one private copy per translation unit. Today the project
// is single-TU (handlers.h is included only by main.cpp), so this is fine.
// If the build is ever split into multiple TUs, switch these globals to
// `inline` (requires C++17) or move definitions to a .cpp — otherwise
// diagOnFrame() and buildPayload() would write to / read from different copies.
static DiagCarConfig gDiagCarConfig = { false, {0} };

// 2048 bits = 256 bytes — covers Tesla Party CAN ID range comfortably.
static constexpr uint16_t DIAG_SEEN_BITS  = 2048;
static constexpr uint16_t DIAG_SEEN_BYTES = DIAG_SEEN_BITS / 8;
static uint8_t gDiagSeenIds[DIAG_SEEN_BYTES] = {0};

// Last raw frame for the IDs that drive speed-limit logic plus the key
// vehicle-state sensors (current speed / brake / gear). Server can decode any
// signal post-hoc without rebuilding firmware. Footprint: ~12 × 10 bytes.
struct DiagFrameSnap {
    bool    seen;
    uint8_t dlc;
    uint8_t bytes[8];
};

// Server keys by CAN ID, so order is incidental. Append-only is still preferred
// to keep git diffs clean.
//
// SAFETY: do NOT add VIN-bearing CAN IDs here (e.g. 0x405 / 0x528 / 0x542 /
// 0x562). The diagnostic upload UI promises "no VIN, no location" — adding any
// of those would violate that disclaimer and require updating the privacy
// notice in mod_diag_upload.h + the web UI tip text.
static constexpr uint32_t DIAG_SPEED_IDS[] = {
    923,   // 0x39B DAS_status                    (fused/vision limit + nag/ACC level)
    921,   // 0x399 DAS_status_ISA                (alternate fused limit source)
    659,   // 0x293 DAS_settings                  (autosteer/aeb/fcw enabled bits)
    760,   // 0x2F8 UI_gpsVehicleSpeed            (Legacy speed-limit sniff)
    1021,  // 0x3FD FSD frame mux-2               (stock HW3/HW4 offset)
    599,   // 0x257 DI_speed                      (current speed)
    325,   // 0x145 ESP_status                    (brake pedal)
    280,   // 0x118 DI_systemStatus               (gear)
    1016,  // 0x3F8 SCCM_steeringAngle / stalk    (HW3/HW4 stalk → speedProfile)
    1006,  // 0x3EE FSD activation frame          (Legacy mux 0/1, modified)
    1080,  // 0x438 UI_driverAssistAnonDebugParams (Legacy overrideSpeedLimit)
    69,    // 0x045 SCCM_leftStalk                (Legacy stalk → speedProfile)
};
static constexpr uint8_t DIAG_SPEED_ID_COUNT =
    sizeof(DIAG_SPEED_IDS) / sizeof(DIAG_SPEED_IDS[0]);
static DiagFrameSnap gDiagSpeedFrames[DIAG_SPEED_ID_COUNT] = {};

inline void diagOnFrame(uint32_t id, const uint8_t* data, uint8_t dlc) {
    if (id == 920 /* 0x398 GTW_carConfig */ && dlc >= 8 && data) {
        memcpy(gDiagCarConfig.bytes, data, 8);
        gDiagCarConfig.seen = true;
    }
    if (id < DIAG_SEEN_BITS) {
        gDiagSeenIds[id >> 3] |= (uint8_t)(1u << (id & 7));
    }
    // Speed-critical raw cache. Tight loop on a fixed small array; modern
    // CAN bus is ~500 frames/s so 8 compares per frame is negligible.
    for (uint8_t i = 0; i < DIAG_SPEED_ID_COUNT; i++) {
        if (DIAG_SPEED_IDS[i] != id) continue;
        if (!data) return;
        DiagFrameSnap& s = gDiagSpeedFrames[i];
        uint8_t n = dlc > 8 ? 8 : dlc;
        memset(s.bytes, 0, sizeof(s.bytes));
        if (n) memcpy(s.bytes, data, n);
        s.dlc  = n;
        s.seen = true;
        return;
    }
}
