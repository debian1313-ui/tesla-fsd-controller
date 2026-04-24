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
//
// Wire encoding (0x3FD mux-2 data[0][6:7] + data[1][0:5], 8-bit raw):
//   Tesla decodes raw as percentage of posted limit: pct = raw / 4.
//   Firmware caps at 50%, so raw range [0, 200]. Source: tesla-open-can-mod
//   include/app.h:211 (manualSpeedOffset = pct * 4) and include/can_helpers.h:113
//   (offsetPct 0-50).
static constexpr int kHw3SpeedOffsetMaxPct = 50;  // wire raw cap = 200

// ── HW3 offset slew limiter ──────────────────────────────────────────────
// When the posted speed limit drops (e.g. 80 → 40 kph), the computed raw
// offset byte also drops. Sending the new lower byte immediately makes the
// car brake hard. Instead, ramp the outgoing byte down at ≤ rate pct/sec of
// posted limit (user-tunable via cfg.hw3SlewRatePctPerSec). Upward steps pass.
//
// Unit: pct/sec (percent of posted limit, per second). Limit-invariant, so
// the same number gives a consistent "feel" across 40 / 60 / 80 kph roads.
// Default 5 pct/sec ≈ 3 kph/s deceleration at a 60 kph limit.
// Internal conversion: raw/sec = pct/sec × 4 (wire encoding is pct × 4).
static constexpr uint8_t kHw3SlewRateMin = 1;    // 1 pct/s ≈ 0.6 kph/s at 60 limit
static constexpr uint8_t kHw3SlewRateMax = 25;   // 25 pct/s ≈ 15 kph/s at 60 limit
static constexpr uint8_t kHw3SlewRateDefault = 5;

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

// Two wire-encoding variants observed in the field. cfg.hw3WireEncoding picks.
enum Hw3WireEnc : uint8_t { HW3_ENC_KPH5 = 0, HW3_ENC_PCT4 = 1, HW3_ENC_MAX = HW3_ENC_PCT4 };

// KPH5 encoding caps at 40 kph offset (raw 200). Observed correct on
// 2024 Model Y HW3 (v1.4.28 reporter).
static constexpr int kHw3EncKph5MaxKph = 40;
static constexpr int kHw3EncKph5Scale  = 5;  // raw = offsetKph × 5
static constexpr int kHw3EncPct4Scale  = 4;  // raw = pct × 4

// raw = pct × 4 (tesla-open-can-mod reference; Issue #9 reporter).
static inline uint8_t encodeHW3OffsetPct4(int pct) {
    int clamped = std::max(0, std::min(pct, kHw3SpeedOffsetMaxPct));
    return (uint8_t)(clamped * kHw3EncPct4Scale);
}

// raw = offsetKph × 5 (v1.4.25 behavior; 2024 Model Y HW3 reporter).
static inline uint8_t encodeHW3OffsetKph5(int offsetKph) {
    int clamped = std::max(0, std::min(offsetKph, kHw3EncKph5MaxKph));
    return (uint8_t)(clamped * kHw3EncKph5Scale);
}

// Single entry point for HW3 0x3FD mux-2 offset writes when caller has km/h.
// Dispatches by cfg.hw3WireEncoding; caller supplies both km/h offset and
// posted limit so either path has what it needs without a second lookup.
static inline uint8_t encodeHW3Offset(int offsetKph, int fusedLimitKph) {
    if (offsetKph <= 0 || fusedLimitKph <= 0) return 0;
    if (cfg.hw3WireEncoding == HW3_ENC_KPH5) {
        return encodeHW3OffsetKph5(offsetKph);
    }
    int pct = (offsetKph * 100 + fusedLimitKph / 2) / fusedLimitKph;
    return encodeHW3OffsetPct4(pct);
}

// High-speed branch stores pct per bucket; dispatch based on wire encoding.
// PCT4 encodes pct directly (avoids kph round-trip precision loss);
// KPH5 converts pct → kph once, then to raw.
static inline uint8_t encodeHW3OffsetFromPct(int pct, int fusedLimitKph) {
    if (pct <= 0 || fusedLimitKph <= 0) return 0;
    if (cfg.hw3WireEncoding == HW3_ENC_PCT4) {
        return encodeHW3OffsetPct4(pct);
    }
    int offsetKph = (fusedLimitKph * pct + 50) / 100;
    return encodeHW3OffsetKph5(offsetKph);
}

// ── CAN ID filter tables (used by handleMessage) ──────────────────────────
static constexpr uint32_t LEGACY_IDS[] = {69, 760, 1006, 1080};
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
        case 0:  return 4;
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

// ── Handler: Legacy (0x3EE / 0x045 / 0x2F8 / 0x438) ─────────────────────
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
    // 0x2F8 (760) — UI_gpsVehicleSpeed: write UI_userSpeedOffset (bit40|6, raw = kph+30).
    // Byte 5 layout: bits 0-5 = offset (0-63), bit 6 reserved, bit 7 = UI_userSpeedOffsetUnits
    // (0=MPH, 1=KPH). We preserve bits 6-7 so the offset unit follows the car's setting.
    if (frame.id == 760) {
        if (cfg.legacyOffset == 0) return;
        if (frame.dlc < 6) return;
        uint8_t raw = (uint8_t)(cfg.legacyOffset + 30);
        frame.data[5] = (frame.data[5] & 0xC0) | (raw & 0x3F);
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
            if (cfg.legacyByte7Enable) {
                frame.data[7] = (frame.data[7] & 0xF0) | (cfg.legacyByte7Value & 0x0F);
            }
            setSpeedProfileV12V13(frame, cfg.speedProfile);
            
            if (driver.send(frame)) cfg.modifiedCount++;
            else                    cfg.errorCount++;
        }
        if (index == 1 && cfg.fsdTriggered && cfg.fsdEnable) {
            setBit(frame, 19, false);
            if (cfg.removeVisionSpeedLimit) setBit(frame, 48, false);  // UI_enableVisionSpeedControl = off
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
            driver.send(frame);  // nag suppression only, not counted as FSD modification
        }
        if (index == 2 && cfg.fsdTriggered && cfg.fsdEnable) {
            // Start from stock offset raw (what Tesla would send — hw3SpeedOffset
            // stored as pct*5 in the [0,100] range, matching open-can-mod).
            uint8_t activeRaw = (uint8_t)std::max(std::min((int)cfg.hw3SpeedOffset, 255), 0);

            // UI enforces that Auto and Custom are mutually exclusive; the Custom-first
            // ordering below is defensive if the client-side mutex ever fails.
            // ≥80 kph: independent hw3HighSpeedEnable branch writes pct×4 directly; when
            // that toggle is off we keep the legacy stock-passthrough behavior.
            {
                uint8_t fl = (cfg.fusedSpeedLimit > 0 && cfg.fusedSpeedLimit < 31) ? cfg.fusedSpeedLimit : 0;
                if (fl > 0) {
                    int fusedLimitKph = (int)fl * 5;
                    if (fusedLimitKph < kHw3StockOffsetCutoverKph) {
                        if (cfg.hw3CustomSpeed || cfg.hw3AutoSpeed) {
                            int targetSpeedKph = cfg.hw3CustomSpeed
                                ? computeHW3CustomTargetSpeedKph(fusedLimitKph)
                                : computeHW3MinimumTargetSpeedKph(fusedLimitKph);
                            if (targetSpeedKph > 0) {
                                int desiredOffsetKph = std::max(targetSpeedKph - fusedLimitKph, 0);
                                activeRaw = encodeHW3Offset(desiredOffsetKph, fusedLimitKph);
                            }
                        }
                    } else {
                        if (cfg.hw3HighSpeedEnable) {
                            int idx = (fusedLimitKph - kHw3HighSpeedBucketBaseKph)
                                      / kHw3HighSpeedBucketStepKph;
                            if (idx < 0) idx = 0;
                            if (idx >= kHw3HighSpeedBucketCount) idx = kHw3HighSpeedBucketCount - 1;
                            uint8_t pct = cfg.hw3HighSpeedTargetPct[idx];
                            if (pct > 0) {
                                activeRaw = encodeHW3OffsetFromPct((int)pct, fusedLimitKph);
                            }
                        }
                    }
                }
            }

            // Diag: record what the encoder asked for before any slew shaping.
            cfg.hw3OffsetTargetRaw = activeRaw;

            // Slew-limit decreases only. When the posted limit suddenly drops
            // (map data catches up, entering town, etc.), computing activeRaw
            // from the new lower limit would collapse the offset in one frame
            // → car brakes hard. Cap how much the sent raw byte can fall per
            // second. Increases pass through untouched so entering a higher-
            // limit road is still instant.
            //
            // Skip slew entirely when the posted limit is ≥80 kph: in that
            // band the only raw-byte changes come from the user flipping the
            // high-speed toggle / pct, not from limit drops that matter for
            // passenger comfort. User asked for ≥80 to be instant.
            uint8_t curFl = (cfg.fusedSpeedLimit > 0 && cfg.fusedSpeedLimit < 31) ? cfg.fusedSpeedLimit : 0;
            bool slewApplies = cfg.hw3OffsetSlew &&
                               curFl > 0 &&
                               ((int)curFl * 5 < kHw3StockOffsetCutoverKph);
            if (slewApplies) {
                uint32_t now = millis();
                uint8_t  last = cfg.hw3OffsetLastRaw;
                // Clamp runtime-configurable rate to [min, max]; a stale/bad NVS
                // byte (0, 255) falls back to the compile-time default. Unit is
                // pct/sec of posted limit, converted to raw/sec by ×4.
                uint8_t  ratePctPerSec = cfg.hw3SlewRatePctPerSec;
                if (ratePctPerSec < kHw3SlewRateMin || ratePctPerSec > kHw3SlewRateMax) {
                    ratePctPerSec = kHw3SlewRateDefault;
                }
                uint32_t rateRawPerSec = (uint32_t)ratePctPerSec * 4;
                if (activeRaw < last && cfg.hw3OffsetLastSentMs != 0) {
                    uint32_t dt = now - cfg.hw3OffsetLastSentMs;
                    uint32_t maxDrop = (rateRawPerSec * dt + 500) / 1000;  // round-to-nearest
                    uint8_t  floorRaw = (last > maxDrop) ? (uint8_t)(last - maxDrop) : 0;
                    if (activeRaw < floorRaw) {
                        activeRaw = floorRaw;
                        cfg.hw3OffsetSlewCount++;
                    }
                }
                cfg.hw3OffsetLastRaw    = activeRaw;
                cfg.hw3OffsetLastSentMs = now;
            } else {
                // Keep state fresh so toggling on later doesn't think we're
                // coming off a huge step; record last byte so there's no
                // transient spike the first frame after the toggle flips on.
                cfg.hw3OffsetLastRaw    = activeRaw;
                cfg.hw3OffsetLastSentMs = millis();
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
        frame.data[7] = computeVehicleChecksum(frame);
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
            setBit(frame, 19, false);
            setBit(frame, 47, true);
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
