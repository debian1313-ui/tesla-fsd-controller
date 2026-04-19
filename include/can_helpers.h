#pragma once
#include "can_frame_types.h"

/*
 * Verified against upstream:
 * https://gitlab.com/Tesla-OPEN-CAN-MOD/tesla-open-can-mod
 */

inline uint8_t readMuxID(const CanFrame& frame) {
    return frame.data[0] & 0x07;
}

inline bool isFSDSelectedInUI(const CanFrame& frame) {
    return (frame.data[4] >> 6) & 0x01;
}

// UI_trackModeSettings — 0x313 (787), data[0] bits[1:0]
inline void setTrackModeRequest(CanFrame& frame, uint8_t request) {
    frame.data[0] &= static_cast<uint8_t>(~0x03);
    frame.data[0] |= static_cast<uint8_t>(request & 0x03);
}

// Checksum: id_lo + id_hi + data[0..dlc-1] (excluding checksumByteIndex)
inline uint8_t computeVehicleChecksum(const CanFrame& frame, uint8_t checksumByteIndex = 7) {
    uint16_t sum = static_cast<uint16_t>(frame.id & 0xFF)
                 + static_cast<uint16_t>((frame.id >> 8) & 0xFF);
    for (uint8_t i = 0; i < frame.dlc; ++i) {
        if (i == checksumByteIndex) continue;
        sum += frame.data[i];
    }
    return static_cast<uint8_t>(sum & 0xFF);
}

// GTW_autopilot — 0x7FF (2047) mux-2, data[5] bits [4:2]
// Values: 0=NONE, 1=HIGHWAY, 2=ENHANCED, 3=SELF_DRIVING, 4=BASIC
inline uint8_t readGTWAutopilot(const CanFrame& frame) {
    return static_cast<uint8_t>((frame.data[5] >> 2) & 0x07);
}

inline void setSpeedProfileV12V13(CanFrame& frame, int profile) {
    // V12/V13 speed field is bits 1-2 of byte 6 (2 bits, max value 3).
    // Clamp to [0,2] — profiles 3/4 are HW4-only; writing them here
    // would shift bits outside the cleared mask and corrupt the frame.
    if (profile > 2) profile = 2;
    frame.data[6] &= ~0x06;
    frame.data[6] |= (uint8_t)((profile & 0x03) << 1);
}

inline void setBit(CanFrame& frame, int bit, bool value) {
    if (bit < 0 || bit >= 64) return;
    int byteIndex = bit / 8;
    int bitIndex = bit % 8;
    uint8_t mask = static_cast<uint8_t>(1U << bitIndex);
    if (value) {
        frame.data[byteIndex] |= mask;
    } else {
        frame.data[byteIndex] &= static_cast<uint8_t>(~mask);
    }
}
