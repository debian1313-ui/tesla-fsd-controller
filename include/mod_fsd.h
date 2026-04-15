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

// ── CAN ID filter tables (used by handleMessage) ──────────────────────────
static constexpr uint32_t LEGACY_IDS[] = {69, 1006};
static constexpr uint32_t HW3_IDS[]    = {1016, 1021};
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
        case 0:  return 2;
        case 1:  return 2;
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
            driver.send(frame);  // nag suppression only, not counted as FSD modification
        }
    }
}

// ── Handler: HW3 (0x3FD / 0x3F8) ─────────────────────────────────────────
static void handleHW3(CanFrame& frame, CanDriver& driver) {
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
            cfg.hw3SpeedOffset = std::max(std::min(((int)((frame.data[3] >> 1) & 0x3F) - 30) * 5, 100), 0);
            setBit(frame, 46, true);
            setSpeedProfileV12V13(frame, cfg.speedProfile);
            if (driver.send(frame)) cfg.modifiedCount++;
            else                    cfg.errorCount++;
        }
        if (index == 1) {
            setBit(frame, 19, false);
            driver.send(frame);  // nag suppression only, not counted as FSD modification
        }
        if (index == 2 && cfg.fsdTriggered && cfg.fsdEnable) {
            int offset;
            if (cfg.hw3SmartEnable) {
                // Smart mode: pick offset tier based on current speed limit
                // raw 0 = UNKNOWN_SNA, raw 31 = NONE — both invalid
                uint8_t fl = (cfg.fusedSpeedLimit > 0 && cfg.fusedSpeedLimit < 31) ? cfg.fusedSpeedLimit : 0;
                uint8_t vl = (cfg.visionSpeedLimit > 0 && cfg.visionSpeedLimit < 31) ? cfg.visionSpeedLimit : 0;
                uint8_t lim = fl > 0 ? fl : vl;
                if (lim > 0) {
                    int speedKph = lim * 5;
                    uint8_t tier;
                    if      (speedKph < cfg.hw3SmartT1) tier = 1;
                    else if (speedKph < cfg.hw3SmartT2) tier = 2;
                    else if (speedKph < cfg.hw3SmartT3) tier = 3;
                    else if (speedKph < cfg.hw3SmartT4) tier = 4;
                    else                                 tier = 5;
                    int kmh = (tier==1)?cfg.hw3SmartO1:(tier==2)?cfg.hw3SmartO2:
                              (tier==3)?cfg.hw3SmartO3:(tier==4)?cfg.hw3SmartO4:cfg.hw3SmartO5;
                    cfg.hw3SmartActiveTier = tier;
                    cfg.hw3SmartLastKmh    = (uint8_t)kmh;
                    offset = kmh * 5;
                } else {
                    // Speed limit unknown — hold last valid offset, do not drop to zero
                    offset = cfg.hw3SmartLastKmh * 5;
                    // tier stays at last known value (do not reset to 0)
                }
            } else {
                // Manual mode: fixed offset (or auto from mux-0)
                cfg.hw3SmartActiveTier = 0;
                offset = (cfg.hw3OffsetManual >= 0) ? cfg.hw3OffsetManual : cfg.hw3SpeedOffset;
            }
            // Hard cap: base speed limit + offset must not exceed 140 kph
            {
                uint8_t fl2 = (cfg.fusedSpeedLimit > 0 && cfg.fusedSpeedLimit < 31) ? cfg.fusedSpeedLimit : 0;
                uint8_t vl2 = (cfg.visionSpeedLimit > 0 && cfg.visionSpeedLimit < 31) ? cfg.visionSpeedLimit : 0;
                uint8_t limRaw = fl2 > 0 ? fl2 : vl2;
                if (limRaw > 0) {
                    int limKph = limRaw * 5;
                    int maxOffsetKph = std::max(0, 140 - limKph);
                    int maxOffsetCAN = maxOffsetKph * 5;
                    offset = std::min(offset, maxOffsetCAN);
                }
            }
            offset = std::max(std::min(offset, 100), 0);
            frame.data[0] &= ~(0b11000000);
            frame.data[1] &= ~(0b00111111);
            frame.data[0] |= (offset & 0x03) << 6;
            frame.data[1] |= (offset >> 2);
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
            setBit(frame, 19, false);
            setBit(frame, 47, true);
            if (driver.send(frame)) cfg.modifiedCount++;
            else                    cfg.errorCount++;
        }
        if (index == 2) {
            frame.data[7] &= ~(0x07 << 4);
            frame.data[7] |= (cfg.speedProfile & 0x07) << 4;
            if (driver.send(frame)) cfg.modifiedCount++;
            else                    cfg.errorCount++;
        }
    }
}
