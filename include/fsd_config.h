#pragma once
#include <cstdint>

// ── Runtime-configurable state (shared between CAN task and web server) ──
// All fields are volatile — written by Core1 (CAN), read by Core0 (WiFi/web).
// Individual 32-bit-or-smaller volatile reads are atomic on ESP32 Xtensa.
// Multi-field compound updates are NOT atomic; see handlers.h for usage notes.
struct FSDConfig {
    // FSD control
    volatile bool     fsdEnable           = true;
    volatile uint8_t  hwMode              = 2;       // 0=LEGACY, 1=HW3, 2=HW4
    volatile uint8_t  speedProfile        = 1;       // 0-4
    volatile bool     profileModeAuto     = true;    // true=auto from stalk, false=manual
    volatile bool     isaChimeSuppress    = false;
    volatile bool     emergencyDetection  = true;
    volatile bool     forceActivate       = false;   // bypass isFSDSelectedInUI (regions without TLSSC)
    volatile int      hw3SpeedOffset      = 0;       // cached from mux-0 frame, used in mux-2
    volatile int      hw3OffsetManual     = -1;      // -1=auto(from CAN), 0-50=user override (km/h)
    volatile bool     hw3SpeedCapEnable   = false;   // cap offset to 20% of visionSpeedLimit
    volatile bool     precondition        = false;   // trigger battery preheating via 0x082
    volatile uint8_t  hwDetected          = 0;       // from 0x398: 0=unknown, 1=HW3, 2=HW4 (informational only)

    // BMS — read-only sniff, no transmission
    volatile bool     bmsSeen             = false;
    volatile uint32_t packVoltage_cV      = 0;   // centivolt  (÷100 = V)
    volatile int32_t  packCurrent_dA      = 0;   // deciampere (÷10  = A, signed)
    volatile uint32_t socPercent_d        = 0;   // deci-%     (÷10  = %)
    volatile int8_t   battTempMin         = 0;   // °C
    volatile int8_t   battTempMax         = 0;   // °C

    // Lighting — read from 0x293 (DAS_settings)
    volatile bool     adaptiveLighting    = false;  // DAS_adaptiveHeadlights bit 22
    volatile bool     highBeamForce       = false;  // user override (only when adaptiveLighting)

    // DAS status — read from 0x39B (DAS_status) and 0x389 (DAS_status2)
    volatile uint8_t  visionSpeedLimit    = 0;   // DAS_visionOnlySpeedLimit   bit16|5  ×5=kph; 0=none
    volatile uint8_t  nagLevel            = 0;   // DAS_autopilotHandsOnState  bit42|4  0=ok, 1-15=nag
    volatile uint8_t  fcwLevel            = 0;   // DAS_forwardCollisionWarning bit22|2  0=none
    volatile uint8_t  accState            = 0;   // DAS_ACC_report             bit26|5  0=off, >0=AP active
    volatile uint8_t  sideCollision       = 0;   // DAS_sideCollisionWarning   bit32|2  0=none,1=left,2=right,3=both
    volatile uint8_t  laneDeptWarning     = 0;   // DAS_laneDepartureWarning   bit37|3  0=none
    volatile uint8_t  laneChangeState     = 0;   // DAS_autoLaneChangeState    bit46|5  0=idle

    // DAS settings readback — from 0x293
    volatile bool     autosteerOn         = false; // DAS_autosteerEnabled  bit38
    volatile bool     aebOn               = false; // DAS_aebEnabled        bit18
    volatile bool     fcwOn               = false; // DAS_fcwEnabled        bit34

    // AP auto-restart — inject 0x293 with autosteerEnabled=1 on disengage
    volatile bool     apRestart           = false;
    volatile uint8_t  apRestartCache[8]   = {};    // last received 0x293 raw bytes
    volatile bool     apRestartValid      = false; // cache has at least one frame

    // Statistics
    volatile uint32_t rxCount             = 0;
    volatile uint32_t modifiedCount       = 0;
    volatile uint32_t errorCount          = 0;
    volatile bool     canOK               = false;
    volatile bool     fsdTriggered        = false;
    volatile uint32_t uptimeStart         = 0;

#ifdef DEBUG_MODE
    // Debug: last captured frame 1021 mux-0 raw bytes
    volatile bool     dbgFrameCaptured    = false;
    volatile uint8_t  dbgFrame[8]         = {};
#endif
};
