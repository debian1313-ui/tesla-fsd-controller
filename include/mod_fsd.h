#pragma once
// ── Module: FSD injection handlers ────────────────────────────────────────
// Three hardware-mode handlers that read/modify/retransmit FSD CAN frames.
// Selector logic lives in handlers.h (handleMessage).
//
// Handled IDs (inbound + modified retransmit):
//   Legacy  — 0x045 (69)   stalk position  [read-only]
//             0x3EE (1006) FSD frame        [mux 0/1 modified]
//   HW3     — 0x3F8 (1016) stalk           [read-only]
//             0x3FD (1021) FSD frame        [mux 0/1/2 modified]
//   HW4     — 0x399 (921)  ISA chime       [modified]
//             0x3F8 (1016) stalk           [read-only]
//             0x3FD (1021) FSD frame        [mux 0/1/2 modified]

#include <algorithm>
#include "can_frame_types.h"
#include "drivers/can_driver.h"
#include "can_helpers.h"
#include "fsd_config.h"

extern FSDConfig cfg;  // defined in handlers.h

// File-scope mutex guarding coherent snapshots of the HW3 smart tier/offset
// table. canTask reads all 9 fields under this mux; /api/set writers take it
// so a mid-update can't produce a mixed read (e.g. T2<T1 transient).
static portMUX_TYPE gHw3SmartMux = portMUX_INITIALIZER_UNLOCKED;

// ── CAN ID filter tables (used by handleMessage) ──────────────────────────
static constexpr uint32_t LEGACY_IDS[] = {69, 1006, 1080};
static constexpr uint32_t HW3_IDS[]    = {787, 1016, 1021};
static constexpr uint32_t HW4_IDS[]    = {921, 1016, 1021};

inline const uint32_t* getFilterIds() {
    switch (cfg.hwMode) {
        case 0:  return LEGACY_IDS;
        case 1:  return HW3_IDS;
        default: return HW4_IDS;
    }
}
inline uint8_t getFilterIdCount() {
    switch (cfg.hwMode) {
        case 0:  return 3;
        case 1:  return 3;
        default: return 3;
    }
}
inline bool isFilteredId(uint32_t id) {
    auto* ids = getFilterIds();
    auto  cnt = getFilterIdCount();
    for (uint8_t i = 0; i < cnt; i++) {
        if (ids[i] == id) return true;
    }
    return false;
}

// ── Handler: Legacy (0x3EE / 0x045) ──────────────────────────────────────
static void handleLegacy(CanFrame& frame, CanDriver& driver) {
    // 0x438 (1080) — UI_driverAssistAnonDebugParams: UI_visionSpeedSlider = 100 (bit56, 7-bit)
    if (frame.id == 1080) {
        if (frame.dlc < 8) return;
        if (!cfg.overrideSpeedLimit) return;
        frame.data[7] = (frame.data[7] & 0x80) | 100;  // bits[6:0] = 100%, preserve bit7
        if (driver.send(frame)) cfg.modifiedCount++;
        else                    cfg.errorCount++;
        return;
    }
    // 0x045 (69) — stalk position → speed profile (auto mode only)
    if (frame.id == 69 && cfg.profileModeAuto) {
        if (frame.dlc < 2) return;
        uint8_t pos = frame.data[1] >> 5;
        if      (pos <= 1) cfg.speedProfile = 2;
        else if (pos == 2) cfg.speedProfile = 1;
        else               cfg.speedProfile = 0;
        return;
    }
    // 0x3EE (1006) — FSD activation frame (mux 0/1)
    if (frame.id == 1006) {
        if (frame.dlc < 8) return;
        auto index = readMuxID(frame);
        if (index == 0) cfg.fsdTriggered = cfg.forceActivate || isFSDSelectedInUI(frame);
        if (index == 0 && cfg.fsdTriggered && cfg.fsdEnable) {
            setBit(frame, 46, true);
            setSpeedProfileV12V13(frame, cfg.speedProfile);
            if (driver.send(frame)) cfg.modifiedCount++;
            else                    cfg.errorCount++;
        }
        if (index == 1) {
            setBit(frame, 19, false);
            setBit(frame, 48, false);  // UI_enableVisionSpeedControl = off
            driver.send(frame);  // nag suppression only, not counted as FSD modification
        }
    }
}

// ── Handler: HW3 (0x3FD / 0x3F8 / 0x313) ────────────────────────────────
static void handleHW3(CanFrame& frame, CanDriver& driver) {
    // 0x313 (787) — UI_trackModeSettings: echo with trackModeRequest=ON
    if (frame.id == 787) {
        if (!cfg.trackModeEnable) return;
        if (frame.dlc < 8) return;
        setTrackModeRequest(frame, 0x01);
        frame.data[7] = computeVehicleChecksum(frame);
        if (driver.send(frame)) cfg.modifiedCount++;
        else                    cfg.errorCount++;
        return;
    }
    // 0x3F8 (1016) — stalk position → speed profile (auto mode only)
    if (frame.id == 1016 && cfg.profileModeAuto) {
        if (frame.dlc < 6) return;
        uint8_t fd = (frame.data[5] & 0b11100000) >> 5;
        switch (fd) {
            case 1: cfg.speedProfile = 2; break;
            case 2: cfg.speedProfile = 1; break;
            case 3: cfg.speedProfile = 0; break;
        }
        return;
    }
    // 0x3FD (1021) — FSD activation frame (mux 0/1/2)
    if (frame.id == 1021) {
        if (frame.dlc < 8) return;
        auto index = readMuxID(frame);
        if (index == 0) cfg.fsdTriggered = cfg.forceActivate || isFSDSelectedInUI(frame);
        if (index == 0 && cfg.fsdTriggered && cfg.fsdEnable) {
            // mux-0 byte 3 bits 1-6: Tesla's own offset preference.
            // Interpret as percentage (0-100) for consistency with smart/manual pipelines.
            cfg.hw3SpeedOffset = std::max(std::min(((int)((frame.data[3] >> 1) & 0x3F) - 30) * 5, 100), 0);
            setBit(frame, 46, true);
            setSpeedProfileV12V13(frame, cfg.speedProfile);
            if (driver.send(frame)) cfg.modifiedCount++;
            else                    cfg.errorCount++;
        }
        if (index == 1) {
            setBit(frame, 19, false);
            setBit(frame, 49, false);  // UI_enableVisionSpeedControl = off
            driver.send(frame);  // nag suppression only, not counted as FSD modification
        }
        if (index == 2 && cfg.fsdTriggered && cfg.fsdEnable) {
            // Override policy:
            //   smart on               → compute pct from tier rules
            //   manual set (>=0)       → use cfg.hw3OffsetManual
            //   auto (smart off, -1)   → pass Tesla's own mux-2 bytes through unchanged
            // Wire encoding for override: canVal = pct × 4 (per upstream release/app.h:211).
            int pct = -1;
            uint8_t fl = (cfg.fusedSpeedLimit > 0 && cfg.fusedSpeedLimit < 31) ? cfg.fusedSpeedLimit : 0;
            uint8_t vl = (cfg.visionSpeedLimit > 0 && cfg.visionSpeedLimit < 31) ? cfg.visionSpeedLimit : 0;
            uint8_t limRaw = fl > 0 ? fl : vl;
            int limKph = limRaw * 5;
            if (cfg.hw3SmartEnable) {
                if (limKph > 0) {
                    // Snapshot tier thresholds + offsets under critical section so
                    // /api/set can't mid-update any individual field and make
                    // limKph straddle an inconsistent boundary (T2 < T1, etc.).
                    uint8_t sT1, sT2, sT3, sT4, sO1, sO2, sO3, sO4, sO5;
                    portENTER_CRITICAL(&gHw3SmartMux);
                    sT1 = cfg.hw3SmartT1; sT2 = cfg.hw3SmartT2;
                    sT3 = cfg.hw3SmartT3; sT4 = cfg.hw3SmartT4;
                    sO1 = cfg.hw3SmartO1; sO2 = cfg.hw3SmartO2; sO3 = cfg.hw3SmartO3;
                    sO4 = cfg.hw3SmartO4; sO5 = cfg.hw3SmartO5;
                    portEXIT_CRITICAL(&gHw3SmartMux);
                    uint8_t tier;
                    if      (limKph <= sT1) tier = 1;
                    else if (limKph <= sT2) tier = 2;
                    else if (limKph <= sT3) tier = 3;
                    else if (limKph <= sT4) tier = 4;
                    else                     tier = 5;
                    pct = (tier==1)?sO1:(tier==2)?sO2:(tier==3)?sO3:(tier==4)?sO4:sO5;
                    cfg.hw3SmartActiveTier = tier;
                    cfg.hw3SmartLastPct    = (uint8_t)std::min(pct, 50);
                } else {
                    // Speed limit unknown — hold last valid percentage, do not drop to zero
                    pct = cfg.hw3SmartLastPct;
                }
            } else {
                cfg.hw3SmartActiveTier = 0;
                if (cfg.hw3OffsetManual >= 0) pct = cfg.hw3OffsetManual;
                // else: pct stays -1 → passthrough (no mux-2 override)
            }
            if (pct >= 0) {
                // Hard cap: base speed limit + offset must not exceed 140 kph.
                if (limKph > 0) {
                    int maxOffsetKph = std::max(0, 140 - limKph);
                    int maxPct       = (maxOffsetKph * 100) / limKph;
                    pct = std::min(pct, maxPct);
                }
                pct = std::max(std::min(pct, 50), 0);     // upstream UI cap
                int canVal = std::min(pct * 4, 200);
                frame.data[0] &= ~(0b11000000);
                frame.data[1] &= ~(0b00111111);
                frame.data[0] |= (canVal & 0x03) << 6;
                frame.data[1] |= (canVal >> 2);
            }
            if (driver.send(frame)) cfg.modifiedCount++;
            else                    cfg.errorCount++;
        }
    }
}

// ── Handler: HW4 (0x3FD / 0x3F8 / 0x399) ────────────────────────────────
static void handleHW4(CanFrame& frame, CanDriver& driver) {
    // 0x399 (921) — ISA speed-limit chime suppression only
    // Speed limits are read from 0x39B (923) by handleDASStatus — do NOT read here
    if (frame.id == 921) {
        if (frame.dlc < 8) return;
        if (!cfg.isaChimeSuppress) return;
        frame.data[1] |= 0x20;
        uint8_t sum = 0;
        for (int i = 0; i < 7; i++) sum += frame.data[i];
        sum += (921 & 0xFF) + (921 >> 8);
        frame.data[7] = sum & 0xFF;
        if (driver.send(frame)) cfg.modifiedCount++;
        else                    cfg.errorCount++;
        return;
    }
    // 0x3F8 (1016) — stalk position → speed profile (auto mode only)
    if (frame.id == 1016 && cfg.profileModeAuto) {
        if (frame.dlc < 6) return;
        auto fd = (frame.data[5] & 0b11100000) >> 5;
        switch (fd) {
            case 1: cfg.speedProfile = 3; break;
            case 2: cfg.speedProfile = 2; break;
            case 3: cfg.speedProfile = 1; break;
            case 4: cfg.speedProfile = 0; break;
            case 5: cfg.speedProfile = 4; break;
        }
        return;
    }
    // 0x3FD (1021) — FSD activation frame (mux 0/1/2)
    if (frame.id == 1021) {
        if (frame.dlc < 8) return;
        auto index = readMuxID(frame);
        if (index == 0) {
            cfg.fsdTriggered = cfg.forceActivate || isFSDSelectedInUI(frame);
#ifdef DEBUG_MODE
            for (int i = 0; i < 8; i++) cfg.dbgFrame[i] = frame.data[i];
            cfg.dbgFrameCaptured = true;
#endif
        }
        if (index == 0 && cfg.fsdTriggered && cfg.fsdEnable) {
            setBit(frame, 46, true);
            setBit(frame, 60, true);
            if (cfg.emergencyDetection) setBit(frame, 59, true);
            if (driver.send(frame)) cfg.modifiedCount++;
            else                    cfg.errorCount++;
        }
        if (index == 1) {
            // bit19=false: nag suppression (same as HW3)
            // bit47=true:  HW4-specific FSD ready signal; counted as modification (unlike HW3 nag-only)
            // bit49=false: UI_enableVisionSpeedControl = off
            setBit(frame, 19, false);
            setBit(frame, 47, true);
            setBit(frame, 49, false);
            if (driver.send(frame)) cfg.modifiedCount++;
            else                    cfg.errorCount++;
        }
        if (index == 2) {
            frame.data[7] &= ~(0x07 << 4);
            frame.data[7] |= (cfg.speedProfile & 0x07) << 4;
            if (cfg.hw4OffsetRaw > 0)
                frame.data[1] = (frame.data[1] & 0xC0) | (cfg.hw4OffsetRaw & 0x3F);
            if (driver.send(frame)) cfg.modifiedCount++;
            else                    cfg.errorCount++;
        }
    }
}
