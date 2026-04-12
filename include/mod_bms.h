#pragma once
// ── Module: BMS frame parsers ──────────────────────────────────────────────
// Reads BMS telemetry from the CAN bus. No frames are transmitted.
// Depends on: FSDConfig cfg (defined in handlers.h before this file is included)
//
// Handled IDs:
//   0x132 (306) — BMS_hvBusStatus   → packVoltage_cV, packCurrent_dA
//   0x292 (658) — BMS_socStatus     → socPercent_d
//   0x312 (786) — BMS_thermalStatus → battTempMin, battTempMax

#include "can_frame_types.h"
#include "fsd_config.h"

extern FSDConfig cfg;  // defined in handlers.h

// 0x132 (306) — BMS_hvBusStatus
// byte[1:0] = pack voltage ×0.01 V (uint16, little-endian)
// byte[3:2] = pack current ×0.1  A (int16,  little-endian, signed)
inline void handleBMSHV(const CanFrame& frame) {
    if (frame.dlc < 4) return;
    uint16_t raw_v = ((uint16_t)frame.data[1] << 8) | frame.data[0];
    int16_t  raw_i = (int16_t)(((uint16_t)frame.data[3] << 8) | frame.data[2]);
    cfg.packVoltage_cV  = raw_v;
    cfg.packCurrent_dA  = (int32_t)raw_i;
    cfg.bmsSeen         = true;
}

// 0x292 (658) — BMS_socStatus
// bits[9:0] = SOC ×0.1 % (uint10, little-endian across byte[1:0])
inline void handleBMSSOC(const CanFrame& frame) {
    if (frame.dlc < 2) return;
    uint16_t raw = ((uint16_t)(frame.data[1] & 0x03) << 8) | frame.data[0];
    cfg.socPercent_d = raw;
    cfg.bmsSeen      = true;
}

// 0x312 (786) — BMS_thermalStatus
// byte[4] = min cell temp + 40 offset → °C
// byte[5] = max cell temp + 40 offset → °C
inline void handleBMSThermal(const CanFrame& frame) {
    if (frame.dlc < 6) return;
    cfg.battTempMin = (int8_t)((int)frame.data[4] - 40);
    cfg.battTempMax = (int8_t)((int)frame.data[5] - 40);
    cfg.bmsSeen     = true;
}
