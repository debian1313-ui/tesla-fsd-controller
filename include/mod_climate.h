#pragma once
// ── Module: Climate telemetry ────────────────────────────────────────────────
// Read-only parser for BODY_R1 (0x28B / 643), GTW bus.
//
// Signal decoding (Motorola big-endian, single-byte signals):
//   AirTemp_Insd   bit47|8  data[5]  raw × 0.25        °C  [0, 63.5]
//   AirTemp_Outsd  bit63|8  data[7]  raw × 0.5 − 40    °C  [−40, 86.5]

#include "can_frame_types.h"
#include "fsd_config.h"

extern FSDConfig cfg;

inline void handleClimate(const CanFrame& frame) {
    if (frame.dlc < 8) return;
    cfg.tempInsideRaw  = frame.data[5];   // AirTemp_Insd  × 0.25 = °C
    cfg.tempOutsideRaw = frame.data[7];   // AirTemp_Outsd × 0.5 − 40 = °C
    cfg.tempSeen       = true;
}
