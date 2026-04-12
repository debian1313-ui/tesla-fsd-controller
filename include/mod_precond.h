#pragma once
// ── Module: Battery preconditioning frame builder ─────────────────────────
// Constructs the UI_tripPlanning frame that requests active battery heating.
// Caller is responsible for sending the frame at the correct interval (~1 s).
//
// Handled ID (outbound only):
//   0x082 (130) — UI_tripPlanning
//     byte[0] bit0 = tripPlanningActive
//     byte[0] bit2 = requestActiveBatteryHeating

#include "can_frame_types.h"

inline void buildPreconditionFrame(CanFrame& frame) {
    frame       = CanFrame{};
    frame.id    = 0x082;
    frame.dlc   = 8;
    frame.data[0] = 0x05;  // bit0=tripPlanningActive | bit2=requestActiveBatteryHeating
}
