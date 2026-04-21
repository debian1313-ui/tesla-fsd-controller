#pragma once
#include <cstdint>

static constexpr int kHw3CustomTargetCount = 5;  // 30/40/50/60/70 kph limit buckets

// ── Runtime-configurable state (shared between CAN task and web server) ──
// All fields are volatile — written by Core1 (CAN), read by Core0 (WiFi/web).
// Individual 32-bit-or-smaller volatile reads are atomic on ESP32 Xtensa.
// Multi-field compound updates are NOT atomic; see handlers.h for usage notes.
struct FSDConfig {
    // FSD control
    volatile bool     fsdEnable           = false;
    volatile uint8_t  hwMode              = 2;       // 0=LEGACY, 1=HW3, 2=HW4
    volatile uint8_t  speedProfile        = 1;       // 0-4
    volatile bool     profileModeAuto     = true;    // true=auto from stalk, false=manual
    volatile bool     isaChimeSuppress    = false;
    volatile bool     emergencyDetection  = true;
    volatile bool     forceActivate       = false;   // bypass isFSDSelectedInUI (regions without TLSSC)
    volatile bool     overrideSpeedLimit  = false;   // Legacy: set UI_visionSpeedSlider=100 in frame 1080
    volatile int      hw3SpeedOffset      = 0;       // stock offset from mux-0 data[3] as pct*5 (0-100); display only
    volatile bool     hw3AutoSpeed        = true;    // HW3 auto speed targeting: <60→64, =60→100, 60-79→85, ≥80→stock passthrough
    volatile bool     hw3CustomSpeed      = false;   // HW3 custom target table (overrides hw3AutoSpeed when true); ≥80 always stock passthrough
    // Defaults match Auto so a fresh unit behaves identically until the user edits.
    volatile uint8_t  hw3CustomTarget[kHw3CustomTargetCount] = {64, 64, 64, 100, 85};
    volatile uint8_t  hw4OffsetRaw       = 0;       // HW4 mux-2 data[1][5:0]; 0=off (presets: 7=+5,10=+7,14=+10,21=+15 km/h)
    volatile uint8_t  hwDetected          = 0;       // from 0x398: 0=unknown, 1=HW3, 2=HW4 (informational only)
    volatile int8_t   gatewayAutopilot    = -1;      // from 0x7FF mux-2: -1=unseen, 0=NONE,1=HIGHWAY,2=ENHANCED,3=SELF_DRIVING,4=BASIC
    volatile bool     trackModeEnable     = false;   // HW3: echo 0x313 with UI_trackModeRequest=ON (off by default)

    // Climate — read from 0x28B (BODY_R1, GTW bus)
    volatile bool     tempSeen          = false;
    volatile uint8_t  tempInsideRaw     = 0;   // AirTemp_Insd  data[5]: raw × 0.25 = °C
    volatile uint8_t  tempOutsideRaw    = 0;   // AirTemp_Outsd data[7]: raw × 0.5 − 40 = °C

    // BMS — read-only sniff, no transmission
    volatile bool     bmsSeen             = false;
    volatile uint32_t packVoltage_cV      = 0;   // centivolt  (÷100 = V)
    volatile int32_t  packCurrent_dA      = 0;   // deciampere (÷10  = A, signed)
    volatile uint32_t socPercent_d        = 0;   // deci-%     (÷10  = %)
    volatile int8_t   battTempMin         = 0;   // °C
    volatile int8_t   battTempMax         = 0;   // °C

    // Lighting — read from 0x293 (DAS_settings)
    volatile bool     adaptiveLighting    = false;  // DAS_adaptiveHeadlights bit 22
    volatile bool     highBeamForce       = false;  // user override via UI or stalk gesture

    // DAS status — read from 0x39B (DAS_status), 0x399 (DAS_status ISA) and 0x389 (DAS_status2)
    volatile uint8_t  fusedSpeedLimit     = 0;   // DAS_fusedSpeedLimit        0x39B byte1[4:0]  ×5=kph; 0=none (camera+map)
    volatile uint8_t  visionSpeedLimit    = 0;   // DAS_visionOnlySpeedLimit   0x39B bit16|5     ×5=kph; 0=none (camera only)
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

    // Performance test — 0→100 acceleration and 100→0 braking
    volatile uint8_t  perfAccelState      = 0;   // 0=idle,1=armed,2=running,3=done
    volatile uint8_t  perfBrakeState      = 0;
    volatile uint32_t perfAccelMs         = 0;   // result ms
    volatile uint32_t perfBrakeMs         = 0;   // result ms
    volatile uint8_t  perfBrakeEntryKph   = 0;   // actual speed (kph) when braking started

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
