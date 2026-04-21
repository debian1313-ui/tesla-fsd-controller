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

// ── HW3 auto speed policy (mirrors tesla-open-can-mod hw3_speed_policy.h) ──
// Field-calibrated request floors for metric clusters. Auto targets / bucket
// shape / cutover live in fsd_config.h so defaults + policy share one source.
static constexpr int kHw3SpeedOffsetRawPerKph = 5;   // wire encoding: canVal = offsetKph × 5
static constexpr int kHw3SpeedOffsetMaxKph    = 40;  // wire raw cap = 200

static inline int computeHW3MinimumTargetSpeedKph(int fusedLimitKph) {
    if (fusedLimitKph == 60)                       return kHw3AutoTargetAt60Kph;
    if (fusedLimitKph <  kHw3AutoTargetBelow60Kph) return kHw3AutoTargetBelow60Kph;
    if (fusedLimitKph <  kHw3StockOffsetCutoverKph) return kHw3AutoTargetForVisible80Kph;
    return fusedLimitKph;
}

// Custom mode: user-defined target-speed lookup bucketed by
// kHw3CustomBucketStepKph starting at kHw3CustomBucketBaseKph.
// Returns 0 when input is outside the table range — caller falls back to passthrough.
static inline int computeHW3CustomTargetSpeedKph(int fusedLimitKph) {
    if (fusedLimitKph <  kHw3CustomBucketBaseKph ||
        fusedLimitKph >= kHw3StockOffsetCutoverKph) return 0;
    int idx = (fusedLimitKph - kHw3CustomBucketBaseKph) / kHw3CustomBucketStepKph;
    return (int)cfg.hw3CustomTarget[idx];
}

static inline uint8_t encodeHW3OffsetRawFromKph(int offsetKph) {
    int clamped = std::max(0, std::min(offsetKph, kHw3SpeedOffsetMaxKph));
    return (uint8_t)(clamped * kHw3SpeedOffsetRawPerKph);
}

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
        if (index == 1 && cfg.fsdTriggered && cfg.fsdEnable) {
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
    // Mirrors tesla-open-can-mod: mux-0 captures the stock EAP offset from
    // byte 3, mux-2 writes the active offset back (passthrough above 80 kph
    // fused limit, calibrated floor below).
    if (frame.id == 1021) {
        if (frame.dlc < 8) return;
        auto index = readMuxID(frame);
        if (index == 0) cfg.fsdTriggered = cfg.forceActivate || isFSDSelectedInUI(frame);
        if (index == 0 && cfg.fsdTriggered && cfg.fsdEnable) {
            // byte 3 bits 1-6 hold Tesla's own stock offset preference encoded
            // as (kph + 30) / 5? No — opendbc says it's offset_kph-ish; here we
            // follow tesla-open-can-mod verbatim: ((d3>>1)&0x3F - 30)*5 clamped [0,100].
            cfg.hw3SpeedOffset = std::max(std::min(((int)((frame.data[3] >> 1) & 0x3F) - 30) * 5, 100), 0);
            setBit(frame, 46, true);
            setSpeedProfileV12V13(frame, cfg.speedProfile);
            if (driver.send(frame)) cfg.modifiedCount++;
            else                    cfg.errorCount++;
        }
        if (index == 1 && cfg.fsdTriggered && cfg.fsdEnable) {
            setBit(frame, 19, false);
            setBit(frame, 49, false);  // UI_enableVisionSpeedControl = off
            driver.send(frame);  // nag suppression only, not counted as FSD modification
        }
        if (index == 2 && cfg.fsdTriggered && cfg.fsdEnable) {
            // Start from stock offset raw (what Tesla would send — hw3SpeedOffset
            // stored as pct*5 in the [0,100] range, matching open-can-mod).
            uint8_t activeRaw = (uint8_t)std::max(std::min((int)cfg.hw3SpeedOffset, 255), 0);

            // UI enforces that Auto and Custom are mutually exclusive; the Custom-first
            // ordering below is defensive if the client-side mutex ever fails.
            // ≥80 kph fused limit always passes stock through (factory EAP ladder is good there).
            if (cfg.hw3CustomSpeed || cfg.hw3AutoSpeed) {
                uint8_t fl = (cfg.fusedSpeedLimit > 0 && cfg.fusedSpeedLimit < 31) ? cfg.fusedSpeedLimit : 0;
                if (fl > 0) {
                    int fusedLimitKph = (int)fl * 5;
                    if (fusedLimitKph < kHw3StockOffsetCutoverKph) {
                        int targetSpeedKph = cfg.hw3CustomSpeed
                            ? computeHW3CustomTargetSpeedKph(fusedLimitKph)
                            : computeHW3MinimumTargetSpeedKph(fusedLimitKph);
                        if (targetSpeedKph > 0) {
                            int desiredOffsetKph = std::max(targetSpeedKph - fusedLimitKph, 0);
                            activeRaw = encodeHW3OffsetRawFromKph(desiredOffsetKph);
                        }
                    }
                }
            }

            frame.data[0] &= ~(0b11000000);
            frame.data[1] &= ~(0b00111111);
            frame.data[0] |= (activeRaw & 0x03) << 6;
            frame.data[1] |= (activeRaw >> 2);
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
        if (index == 1 && cfg.fsdTriggered && cfg.fsdEnable) {
            // bit19=false: nag suppression (same as HW3)
            // bit47=true:  HW4-specific FSD ready signal; counted as modification (unlike HW3 nag-only)
            // bit49=false: UI_enableVisionSpeedControl = off
            setBit(frame, 19, false);
            setBit(frame, 47, true);
            setBit(frame, 49, false);
            if (driver.send(frame)) cfg.modifiedCount++;
            else                    cfg.errorCount++;
        }
        if (index == 2 && cfg.fsdTriggered && cfg.fsdEnable) {
            frame.data[7] &= ~(0x07 << 4);
            frame.data[7] |= (cfg.speedProfile & 0x07) << 4;
            if (cfg.hw4OffsetRaw > 0)
                frame.data[1] = (frame.data[1] & 0xC0) | (cfg.hw4OffsetRaw & 0x3F);
            if (driver.send(frame)) cfg.modifiedCount++;
            else                    cfg.errorCount++;
        }
    }
}
