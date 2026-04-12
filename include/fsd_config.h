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
    volatile int      hw3OffsetManual     = -1;      // -1=auto(from CAN), 0-50=user override (%)
    volatile bool     precondition        = false;   // trigger battery preheating via 0x082
    volatile uint8_t  hwDetected          = 0;       // from 0x398: 0=unknown, 1=HW3, 2=HW4 (informational only)

    // BMS — read-only sniff, no transmission
    volatile bool     bmsSeen             = false;
    volatile uint32_t packVoltage_cV      = 0;   // centivolt  (÷100 = V)
    volatile int32_t  packCurrent_dA      = 0;   // deciampere (÷10  = A, signed)
    volatile uint32_t socPercent_d        = 0;   // deci-%     (÷10  = %)
    volatile int8_t   battTempMin         = 0;   // °C
    volatile int8_t   battTempMax         = 0;   // °C

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
