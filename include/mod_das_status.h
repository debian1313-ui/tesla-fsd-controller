#pragma once
// ── Module: DAS status telemetry ─────────────────────────────────────────────
// Read-only parsers for DAS_status (0x39B/923) and DAS_status2 (0x389/905).
//
// Bit extraction: byte = bit/8, shift = bit%8, value = (data[byte]>>shift)&mask
//
// 0x39B (923) DAS_status — DLC >= 7:
//   DAS_fusedSpeedLimit         bit8|5   data[1] & 0x1F            ×5 kph  (0=SNA, 31=NONE)
//   DAS_visionOnlySpeedLimit    bit16|5  data[2] & 0x1F            ×5 kph  (0=SNA, 31=NONE)
//   DAS_forwardCollisionWarning bit22|2  (data[2]>>6) & 0x03
//   DAS_autoparkReady           bit24|1  data[3] & 0x01
//   DAS_sideCollisionAvoid      bit30|2  (data[3]>>6) & 0x03
//   DAS_sideCollisionWarning    bit32|2  data[4] & 0x03            0=none,1=left,2=right,3=both
//   DAS_laneDepartureWarning    bit37|3  (data[4]>>5) & 0x07
//   DAS_autopilotHandsOnState   bit42|4  (data[5]>>2) & 0x0F       nag level
//   DAS_autoLaneChangeState     bit46|5  ((data[5]>>6)&0x03)|((data[6]&0x07)<<2)
//
// 0x389 (905) DAS_status2 — DLC >= 5:
//   DAS_ACC_report              bit26|5  (data[3]>>2) & 0x1F       0=AP off

#include "can_frame_types.h"
#include "fsd_config.h"

extern FSDConfig cfg;

// ── DAS_status (0x39B / 923) ─────────────────────────────────────────────────
inline void handleDASStatus(const CanFrame& frame) {
    if (frame.dlc < 7) return;
    cfg.fusedSpeedLimit  = frame.data[1] & 0x1F;                                      // bit8|5  DAS_fusedSpeedLimit ×5=kph
    cfg.visionSpeedLimit = frame.data[2] & 0x1F;                                      // bit16|5 DAS_visionOnlySpeedLimit ×5=kph
    cfg.fcwLevel         = (frame.data[2] >> 6) & 0x03;                               // bit22|2
    cfg.sideCollision    = frame.data[4] & 0x03;                                       // bit32|2
    cfg.laneDeptWarning  = (frame.data[4] >> 5) & 0x07;                               // bit37|3
    cfg.nagLevel         = (frame.data[5] >> 2) & 0x0F;                               // bit42|4
    cfg.laneChangeState  = ((frame.data[5] >> 6) & 0x03) | ((frame.data[6] & 0x07) << 2); // bit46|5
}

// ── DAS_status2 (0x389 / 905) ────────────────────────────────────────────────
inline void handleDASStatus2(const CanFrame& frame) {
    if (frame.dlc < 5) return;
    cfg.accState = (frame.data[3] >> 2) & 0x1F;   // bit26|5
}
