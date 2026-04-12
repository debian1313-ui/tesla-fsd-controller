#pragma once
// ── Module: Telemetry ring buffer ────────────────────────────────────────────
// Parses additional CAN frames for vehicle state (speed, gear, torque, brake).
// Provides a 600-record circular buffer sampled at 1 Hz from canTask.
// CSV download served via /api/log in main.cpp.
//
// Single-TU project — static buffer lives here, not duplicated. No ODR risk.
// NOLINT(misc-definitions-in-headers) applies to the logBuf/logHead/logCount
// statics below.
//
// Newly parsed IDs (opendbc tesla_model3_party.dbc, verified):
//   0x257 (599) — DI_speed         [DI_vehicleSpeed bit12|12 ×0.08−40 kph; stored ×10]
//   0x145 (325) — ESP_status       [ESP_driverBrakeApply byte3 bits[6:5] non-zero=applied]
//   0x118 (280) — DI_systemStatus  [DI_gear bit21|3: 0=INVALID,1=P,2=R,3=N,4=D,7=SNA]
//   0x108 (264) — DI_torque        [DI_torqueCommand bit12|13 signed ×2 Nm; DI_torqueActual bit27|13 signed ×2 Nm]

#include <cstdint>
#include "can_frame_types.h"
#include "fsd_config.h"

extern FSDConfig cfg;  // defined in handlers.h

// ── Live-update state (written by CAN parsers, read by recordTelemetrySample) ─
static volatile uint16_t telem_speed_raw   = 0;      // kph×10 (physical = value/10 kph)
static volatile uint8_t  telem_gear        = 0xFF;   // 0=INVALID,1=P,2=R,3=N,4=D,7=SNA
static volatile bool     telem_brake       = false;  // brake pedal applied (0x145)
static volatile int16_t  telem_torqueFront = 0;      // DI_torqueCommand (rear drive unit) raw int13s; ×2 = Nm
static volatile int16_t  telem_torqueRear  = 0;      // DI_torqueActual  (rear drive unit) raw int13s; ×2 = Nm

// ── CAN frame parsers ─────────────────────────────────────────────────────────

// 0x257 (599) — DI_speed
// DI_vehicleSpeed : 12|12@1+ (0.08,-40) kph  →  byte1[7:4] + byte2[7:0]
// Stored as kph×10 so existing /10 displays remain correct.
inline void handleSpeed(const CanFrame& frame) {
    if (frame.dlc < 3) return;
    uint16_t raw = ((uint16_t)frame.data[1] >> 4) | ((uint16_t)frame.data[2] << 4);
    // physical kph = raw * 0.08 - 40  →  kph×10 = raw * 4/5 - 400
    int32_t kph10 = (int32_t)raw * 4 / 5 - 400;
    telem_speed_raw = (kph10 > 0) ? (uint16_t)kph10 : 0;
}

// 0x145 (325) — ESP_status
// ESP_driverBrakeApply : 29|2@1+  byte[3] bits[6:5]
// VAL: 0=NotInit_or_Off  1=Not_Applied  2=Driver_applying_brakes  3=Faulty_SNA
inline void handleBrake(const CanFrame& frame) {
    if (frame.dlc < 4) return;
    telem_brake = ((frame.data[3] >> 5) & 0x03) == 2;
}

// 0x118 (280) — DI_systemStatus
// DI_gear : 21|3@1+  →  byte[2] bits[7:5]
// Values: 0=INVALID,1=P,2=R,3=N,4=D,7=SNA
inline void handleGear(const CanFrame& frame) {
    if (frame.dlc < 3) return;
    telem_gear = (frame.data[2] >> 5) & 0x07;
}

// 0x108 (264) — DI_torque
// DI_torqueCommand : 12|13@1-  signed ×2 Nm  →  byte1[7:4] | byte2<<4 | byte3[0]<<12
// DI_torqueActual  : 27|13@1-  signed ×2 Nm  →  byte3[7:3] | byte4<<5
inline void handleTorque(const CanFrame& frame) {
    if (frame.dlc < 5) return;
    // torqueCommand (bit 12, 13-bit signed)
    uint16_t rc = ((uint16_t)(frame.data[1] >> 4))
                | ((uint16_t)frame.data[2] << 4)
                | (((uint16_t)(frame.data[3] & 0x01)) << 12);
    telem_torqueFront = (rc & 0x1000) ? (int16_t)(rc | 0xE000) : (int16_t)rc;
    // torqueActual (bit 27, 13-bit signed)
    uint16_t ra = ((uint16_t)(frame.data[3] >> 3))
                | ((uint16_t)frame.data[4] << 5);
    telem_torqueRear  = (ra & 0x1000) ? (int16_t)(ra | 0xE000) : (int16_t)ra;
}

// ── Live-state accessors (for /api/status JSON) ───────────────────────────────
inline uint16_t telemSpeedRaw()    { return telem_speed_raw; }
inline uint8_t  telemGear()        { return telem_gear; }
inline int16_t  telemTorqueFront() { return telem_torqueFront; }
inline int16_t  telemTorqueRear()  { return telem_torqueRear; }
inline bool     telemBrake()       { return telem_brake; }
