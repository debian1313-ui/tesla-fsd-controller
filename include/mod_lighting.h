#pragma once
// ── Module: Adaptive lighting guard + high beam force ────────────────────────
// Reads DAS_settings (0x293/659) to detect whether adaptive (auto) headlights
// are enabled in the car's settings. handleSCCMStalk() intercepts every 0x249
// frame and supports two modes:
//   1. Gesture control: 2 quick PULL taps → force ON; 1 tap → force OFF
//   2. Web UI toggle: /api/highbeam endpoint sets cfg.highBeamForce directly
// When force is active, bits[5:4] are overwritten with PUSH (2) so the car
// sees a sustained PUSH level. Mask 0xCF preserves washWipe and counter bits.
//
// Sources: opendbc tesla_model3_party.dbc, flipper-tesla-fsd fsd_handler.c.
//
// 0x293 (659) DAS_settings — bit encoding: (frame.data[bit/8] >> (bit%8)) & 0x01
//   DAS_adaptiveHeadlights  bit 22, width 1  → (frame.data[2] >> 6) & 0x01
//
// 0x249 (585) SCCM_leftStalk (3-byte frame):
//   Byte0: CRC = (0x49 + 0x02 + Byte1 + Byte2) & 0xFF
//   Byte1[3:0]: 4-bit rolling counter (0-15)
//   Byte1[5:4]: SCCM_highBeamStalkStatus  0=IDLE  1=PULL/flash  2=PUSH/sustained
//   Byte1[7:6]: SCCM_washWipeButtonStatus (0=NONE here)
//   Byte2[2:0]: SCCM_turnIndicatorStalkStatus (0=IDLE here)

#include <cstring>
#include "can_frame_types.h"
#include "fsd_config.h"
#include "drivers/can_driver.h"

extern FSDConfig cfg;  // defined in handlers.h

// ── DAS_settings (0x293 / 659) parser ────────────────────────────────────────
// Full readback: adaptive headlights, autosteer, AEB, FCW.
//
// Key signals (bit/8 = byte, bit%8 = shift):
//   DAS_adaptiveHeadlights  bit22|1  (data[2]>>6) & 0x01
//   DAS_aebEnabled          bit18|1  (data[2]>>2) & 0x01
//   DAS_fcwEnabled          bit34|1  (data[4]>>2) & 0x01
//   DAS_autosteerEnabled    bit38|1  (data[4]>>6) & 0x01
inline void handleDASSettings(const CanFrame& frame) {
    if (frame.dlc < 5) return;
    bool adaptive = ((frame.data[2] >> 6) & 0x01) != 0;
    cfg.adaptiveLighting = adaptive;
    if (!adaptive) cfg.highBeamForce = false;   // safety guard
    cfg.aebOn        = ((frame.data[2] >> 2) & 0x01) != 0;  // bit18
    cfg.fcwOn        = ((frame.data[4] >> 2) & 0x01) != 0;  // bit34
    cfg.autosteerOn  = ((frame.data[4] >> 6) & 0x01) != 0;  // bit38
}

// ── SCCM_leftStalk (0x249 / 585) interceptor ─────────────────────────────────
// Intercepts each physical SCCM frame, optionally modifies stalk state, then
// retransmits. Always forwards so the car's SCCM watchdog stays happy.
//
// Gesture control (requires adaptiveLighting):
//   Force OFF → 3 quick PULL taps within 1000 ms → Force ON
//   Force ON  → 1 PULL tap                       → Force OFF
//
// Bit mask 0xCF = 1100_1111: preserves washWipe bits[7:6] and counter bits[3:0],
// overwriting only highBeam bits[5:4].

static constexpr uint32_t HB_GESTURE_WINDOW_MS = 1000;
static constexpr uint8_t  HB_GESTURE_TAPS_ON   = 3;

static uint8_t  hbTapCount   = 0;
static uint32_t hbLastTapMs  = 0;
static bool     hbPullActive = false;   // true while stalk is held PULL

// Call on CAN bus-off to prevent spurious frames during recovery from
// completing a partially-accumulated gesture.
inline void resetStalkGestureState() {
    hbTapCount   = 0;
    hbLastTapMs  = 0;
    hbPullActive = false;
}

inline void handleSCCMStalk(CanFrame& frame, CanDriver& driver) {
    if (frame.dlc >= 3) {
        uint8_t physHB = (frame.data[1] >> 4) & 0x03;
        bool isPull = (physHB == 1);

        // Expire gesture window proactively so a stale partial count
        // cannot be completed by spurious frames after CAN recovery.
        uint32_t now = millis();
        if (hbTapCount > 0 && hbLastTapMs > 0 &&
            now - hbLastTapMs > HB_GESTURE_WINDOW_MS) {
            hbTapCount = 0;
        }

        // Detect rising edge (new tap)
        if (isPull && !hbPullActive) {
            if (now - hbLastTapMs > HB_GESTURE_WINDOW_MS) hbTapCount = 0;
            hbTapCount++;
            hbLastTapMs = now;

            uint32_t up = (now - cfg.uptimeStart) / 1000;
            if (cfg.highBeamForce) {
                // Any single tap while active → disable
                cfg.highBeamForce = false;
                hbTapCount = 0;
                addDiagLog(up, "HB force: stalk OFF");
            } else if (hbTapCount >= HB_GESTURE_TAPS_ON) {
                // Double tap while inactive → enable
                cfg.highBeamForce = true;
                hbTapCount = 0;
                addDiagLog(up, "HB force: stalk ON");
            }
        }
        hbPullActive = isPull;

        // Override outgoing frame if force is active
        if (cfg.highBeamForce) {
            frame.data[1] = (frame.data[1] & 0xCF) | (2 << 4);
            frame.data[0] = (uint8_t)(((0x249 & 0xFF) + ((0x249 >> 8) & 0xFF) +
                                        frame.data[1] + frame.data[2]) & 0xFF);
            // Only re-TX when we are overriding. On a non-MITM tap, echoing an
            // unmodified 0x249 just duplicates what SCCM is already sending —
            // wasted TX that steals bandwidth and drives TEC toward bus-off.
            if (!driver.send(frame)) cfg.errorCount++;
        }
    }
}
