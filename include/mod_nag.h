#pragma once
// ── Module: Nag killer ────────────────────────────────────────────────────────
// Intercepts 0x370 EPAS3S_sysStatus and echoes it with counter+1.
//
// Background: AP monitors the EPAS rolling counter to detect fresh steering
// data. By injecting a frame with counter+1 immediately after the real frame,
// AP sees continuous "fresh" EPAS updates which can suppress or delay the
// hands-on nag sequence. Torsion bar torque and handsOnLevel are NOT modified
// — only the counter and checksum are changed.
//
// 0x370 (880) EPAS3S_sysStatus — DLC 8:
//   EPAS3S_sysStatusCounter  : 48|4@1+   byte6[3:0]  rolling 0-15
//   EPAS3S_sysStatusChecksum : 56|8@1+   byte7       sum(id_lo+id_hi+data[0..6])
//
// ⚠️  WARNING: this bypasses the hands-on detection safety system.
//     Only enable in controlled conditions. Off by default.

#include "can_frame_types.h"
#include "drivers/can_driver.h"
#include "fsd_config.h"

extern FSDConfig cfg;

inline void handleEPASNag(const CanFrame& frame, CanDriver& driver) {
    if (!cfg.nagKiller || !cfg.accState) return;
    if (frame.dlc < 8) return;

    CanFrame f = frame;  // copy

    // Increment rolling counter (byte6 bits[3:0])
    uint8_t cnt = (f.data[6] & 0x0F);
    cnt = (cnt + 1) & 0x0F;
    f.data[6] = (f.data[6] & 0xF0) | cnt;

    // Recalculate checksum: sum(id_lo + id_hi + data[0..6]) & 0xFF
    uint16_t sum = (880 & 0xFF) + (880 >> 8);
    for (uint8_t i = 0; i < 7; i++) sum += f.data[i];
    f.data[7] = (uint8_t)(sum & 0xFF);

    if (!driver.send(f)) cfg.errorCount++;
}
