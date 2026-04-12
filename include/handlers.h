#pragma once
#include <cstdint>
#include <algorithm>
#include "can_frame_types.h"
#include "drivers/can_driver.h"
#include "can_helpers.h"

// ── Runtime-configurable state (shared with web server) ──
struct FSDConfig {
    volatile bool     fsdEnable           = true;
    volatile uint8_t  hwMode              = 2;       // 0=LEGACY, 1=HW3, 2=HW4
    volatile uint8_t  speedProfile        = 1;       // 0-4
    volatile bool     profileModeAuto     = true;    // true=auto from stalk, false=manual
    volatile bool     isaChimeSuppress    = false;
    volatile bool     emergencyDetection  = true;
    volatile bool     forceActivate      = false;  // bypass isFSDSelectedInUI check (regions without TLSSC)
    volatile int      hw3SpeedOffset     = 0;      // cached from mux-0 frame, used in mux-2
    volatile int      hw3OffsetManual    = -1;     // -1=auto(from CAN), 0-50=user override (%)
    volatile bool     precondition       = false;  // trigger battery preheating via 0x082
    volatile uint8_t  hwDetected        = 0;      // HW detected from 0x398: 0=unknown,1=HW3,2=HW4 (informational only, does NOT override hwMode)

    // BMS (read-only sniff)
    volatile bool     bmsSeen           = false;
    volatile uint32_t packVoltage_cV    = 0;   // centivolt  (÷100 = V)
    volatile int32_t  packCurrent_dA    = 0;   // deciampere (÷10  = A, signed)
    volatile uint32_t socPercent_d      = 0;   // deci-%     (÷10  = %)
    volatile int8_t   battTempMin       = 0;   // °C
    volatile int8_t   battTempMax       = 0;   // °C

    // Stats
    volatile uint32_t rxCount       = 0;
    volatile uint32_t modifiedCount = 0;
    volatile uint32_t errorCount    = 0;
    volatile bool     canOK         = false;
    volatile bool     fsdTriggered  = false;
    volatile uint32_t uptimeStart   = 0;

#ifdef DEBUG_MODE
    // Debug: last captured frame 1021 mux-0 raw bytes
    volatile bool    dbgFrameCaptured = false;
    volatile uint8_t dbgFrame[8]      = {};
#endif
};

static FSDConfig cfg;

// ── Precondition frame builder ──
inline void buildPreconditionFrame(CanFrame& frame) {
    frame = CanFrame{};
    frame.id  = 0x082;  // 130 — UI_tripPlanning
    frame.dlc = 8;
    frame.data[0] = 0x05;  // bit0=tripPlanningActive, bit2=requestActiveBatteryHeating
}

// ── BMS frame parsers (read-only sniff) ──
// 0x132 (306) — BMS_hvBusStatus
inline void handleBMSHV(const CanFrame& frame) {
    if (frame.dlc < 4) return;
    uint16_t raw_v = ((uint16_t)frame.data[1] << 8) | frame.data[0];
    int16_t  raw_i = (int16_t)(((uint16_t)frame.data[3] << 8) | frame.data[2]);
    cfg.packVoltage_cV = raw_v;                 // ×0.01 V
    cfg.packCurrent_dA = (int32_t)raw_i;        // ×0.1  A
    cfg.bmsSeen = true;
}
// 0x292 (658) — BMS_socStatus
inline void handleBMSSOC(const CanFrame& frame) {
    if (frame.dlc < 2) return;
    uint16_t raw = ((uint16_t)(frame.data[1] & 0x03) << 8) | frame.data[0];
    cfg.socPercent_d = raw;                     // ×0.1 %
    cfg.bmsSeen = true;
}
// 0x312 (786) — BMS_thermalStatus
inline void handleBMSThermal(const CanFrame& frame) {
    if (frame.dlc < 6) return;
    cfg.battTempMin = (int8_t)((int)frame.data[4] - 40);
    cfg.battTempMax = (int8_t)((int)frame.data[5] - 40);
    cfg.bmsSeen = true;
}

// ── Filter IDs per HW mode ──
static constexpr uint32_t LEGACY_IDS[] = {69, 1006};
static constexpr uint32_t HW3_IDS[]    = {1016, 1021};
static constexpr uint32_t HW4_IDS[]    = {921, 1016, 1021};

inline const uint32_t* getFilterIds() {
    switch (cfg.hwMode) {
        case 0: return LEGACY_IDS;
        case 1: return HW3_IDS;
        default: return HW4_IDS;
    }
}
inline uint8_t getFilterIdCount() {
    switch (cfg.hwMode) {
        case 0: return 2;
        case 1: return 2;
        default: return 3;
    }
}

inline bool isFilteredId(uint32_t id) {
    auto* ids = getFilterIds();
    auto  cnt = getFilterIdCount();
    for (uint8_t i = 0; i < cnt; i++) {
        if (ids[i] == id) return true;
    }
    return false;
}

// ── Handler: Legacy ──
static void handleLegacy(CanFrame& frame, CanDriver& driver) {
    if (frame.id == 69 && cfg.profileModeAuto) {
        if (frame.dlc < 2) return;
        uint8_t pos = frame.data[1] >> 5;
        if      (pos <= 1) cfg.speedProfile = 2;
        else if (pos == 2) cfg.speedProfile = 1;
        else               cfg.speedProfile = 0;
        return;
    }
    if (frame.id == 1006) {
        if (frame.dlc < 8) return;
        auto index = readMuxID(frame);
        if (index == 0) cfg.fsdTriggered = cfg.forceActivate || isFSDSelectedInUI(frame);
        if (index == 0 && cfg.fsdTriggered && cfg.fsdEnable) {
            setBit(frame, 46, true);
            setSpeedProfileV12V13(frame, cfg.speedProfile);
            if (driver.send(frame)) cfg.modifiedCount++;
            else cfg.errorCount++;
        }
        if (index == 1) {
            setBit(frame, 19, false);
            driver.send(frame);  // nag suppression only, not counted as FSD modification
        }
    }
}

// ── Handler: HW3 ──
static void handleHW3(CanFrame& frame, CanDriver& driver) {
    if (frame.id == 1016 && cfg.profileModeAuto) {
        if (frame.dlc < 6) return;
        uint8_t fd = (frame.data[5] & 0b11100000) >> 5;
        switch (fd) {
            case 1: cfg.speedProfile = 2; break;
            case 2: cfg.speedProfile = 1; break;
            case 3: cfg.speedProfile = 0; break;
        }
        return;
    }
    if (frame.id == 1021) {
        if (frame.dlc < 8) return;
        auto index = readMuxID(frame);
        if (index == 0) cfg.fsdTriggered = cfg.forceActivate || isFSDSelectedInUI(frame);
        if (index == 0 && cfg.fsdTriggered && cfg.fsdEnable) {
            cfg.hw3SpeedOffset = std::max(std::min(((int)((frame.data[3] >> 1) & 0x3F) - 30) * 5, 50), 0);
            setBit(frame, 46, true);
            setSpeedProfileV12V13(frame, cfg.speedProfile);
            if (driver.send(frame)) cfg.modifiedCount++;
            else cfg.errorCount++;
        }
        if (index == 1) {
            setBit(frame, 19, false);
            if (driver.send(frame)) cfg.modifiedCount++;
            else cfg.errorCount++;
        }
        if (index == 2 && cfg.fsdTriggered && cfg.fsdEnable) {
            int offset = (cfg.hw3OffsetManual >= 0) ? cfg.hw3OffsetManual : cfg.hw3SpeedOffset;
            frame.data[0] &= ~(0b11000000);
            frame.data[1] &= ~(0b00111111);
            frame.data[0] |= (offset & 0x03) << 6;
            frame.data[1] |= (offset >> 2);
            if (driver.send(frame)) cfg.modifiedCount++;
            else cfg.errorCount++;
        }
    }
}

// ── Handler: HW4 ──
static void handleHW4(CanFrame& frame, CanDriver& driver) {
    if (cfg.isaChimeSuppress && frame.id == 921) {
        if (frame.dlc < 8) return;
        frame.data[1] |= 0x20;
        uint8_t sum = 0;
        for (int i = 0; i < 7; i++) sum += frame.data[i];
        sum += (921 & 0xFF) + (921 >> 8);
        frame.data[7] = sum & 0xFF;
        if (driver.send(frame)) cfg.modifiedCount++;
        else cfg.errorCount++;
        return;
    }
    if (frame.id == 1016 && cfg.profileModeAuto) {
        auto fd = (frame.data[5] & 0b11100000) >> 5;
        switch (fd) {
            case 1: cfg.speedProfile = 3; break;
            case 2: cfg.speedProfile = 2; break;
            case 3: cfg.speedProfile = 1; break;
            case 4: cfg.speedProfile = 0; break;
            case 5: cfg.speedProfile = 4; break;
        }
    }
    if (frame.id == 1021) {
        if (frame.dlc < 8) return;
        auto index = readMuxID(frame);
        if (index == 0) {
            cfg.fsdTriggered = cfg.forceActivate || isFSDSelectedInUI(frame);
#ifdef DEBUG_MODE
            for (int i = 0; i < 8; i++) cfg.dbgFrame[i] = frame.data[i];
            cfg.dbgFrameCaptured = true;
#endif
        }
        if (index == 0 && cfg.fsdTriggered && cfg.fsdEnable) {
            setBit(frame, 46, true);
            setBit(frame, 60, true);
            if (cfg.emergencyDetection) setBit(frame, 59, true);
            if (driver.send(frame)) cfg.modifiedCount++;
            else cfg.errorCount++;
        }
        if (index == 1) {
            setBit(frame, 19, false);
            setBit(frame, 47, true);
            if (driver.send(frame)) cfg.modifiedCount++;
            else cfg.errorCount++;
        }
        if (index == 2) {
            frame.data[7] &= ~(0x07 << 4);
            frame.data[7] |= (cfg.speedProfile & 0x07) << 4;
            if (driver.send(frame)) cfg.modifiedCount++;
            else cfg.errorCount++;
        }
    }
}

// ── Unified dispatch ──
static void handleMessage(CanFrame& frame, CanDriver& driver) {
    cfg.rxCount++;

    // HW detection: GTW_carConfig 0x398 (920) — informational only, does NOT change hwMode
    // User must manually select hwMode; this only shows what hardware is detected.
    if (frame.id == 920) {
        if (frame.dlc < 1) return;
        uint8_t das_hw = (frame.data[0] >> 6) & 0x03;
        if (das_hw == 2)      cfg.hwDetected = 1;  // HW3 detected
        else if (das_hw == 3) cfg.hwDetected = 2;  // HW4 detected
        return;
    }

    // BMS read-only sniff (no transmission)
    if (frame.id == 306) { handleBMSHV(frame);      return; }  // 0x132 BMS_hvBusStatus
    if (frame.id == 658) { handleBMSSOC(frame);     return; }  // 0x292 BMS_socStatus
    if (frame.id == 786) { handleBMSThermal(frame); return; }  // 0x312 BMS_thermalStatus

    if (!isFilteredId(frame.id)) return;
    switch (cfg.hwMode) {
        case 0: handleLegacy(frame, driver); break;
        case 1: handleHW3(frame, driver); break;
        default: handleHW4(frame, driver); break;
    }
}
