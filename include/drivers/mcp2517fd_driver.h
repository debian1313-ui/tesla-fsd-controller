#pragma once
// ── MCP2517FDDriver ─────────────────────────────────────────────────────────
// Wraps Pierre Molinaro's ACAN2517 library in the CanDriver interface.
// Operates in CAN 2.0B mode at 500 kbit/s (same as TWAI / Party CAN bus).
//
// Multiple instances share one SPIClass but each needs its own CS + INT pin.
//
// Oscillator: most MCP2517FD breakout modules ship with a 40 MHz crystal.
// Override with -DMCP2517FD_OSC_HZ=20000000UL if yours uses 20 MHz.
//
#include "can_driver.h"
#include <SPI.h>
#include <ACAN2517.h>

#ifndef MCP2517FD_OSC_HZ
#define MCP2517FD_OSC_HZ 40000000UL
#endif

struct MCP2517FDDriver : public CanDriver {
    MCP2517FDDriver(uint8_t csPin, SPIClass& spi, uint8_t intPin)
        : mcp_(csPin, spi, intPin) {}

    bool init() override {
        ACAN2517Settings settings(MCP2517FD_OSC_HZ,
                                  ACAN2517Settings::bitrate500kBPS);
        settings.mRequestedMode = ACAN2517Settings::NormalMode;
        const uint32_t err = mcp_.begin(settings, nullptr);
        ok_ = (err == 0);
        if (!ok_) {
            Serial.printf("[MCP2517FD] init error 0x%08X\n", (unsigned)err);
        }
        return ok_;
    }

    bool send(const CanFrame& frame) override {
        if (!ok_) return false;
        CANMessage msg;
        msg.id  = frame.id;
        msg.len = frame.dlc;
        msg.ext = false;
        msg.rtr = false;
        memcpy(msg.data, frame.data, frame.dlc);
        return mcp_.tryToSend(msg);
    }

    bool read(CanFrame& frame) override {
        if (!ok_) return false;
        CANMessage msg;
        if (!mcp_.receive(msg)) return false;
        frame.id  = msg.id;
        frame.dlc = msg.len > 8 ? 8 : msg.len;
        memset(frame.data, 0, 8);
        memcpy(frame.data, msg.data, frame.dlc);
        return true;
    }

    void setFilters(const uint32_t* /*ids*/, uint8_t /*count*/) override {}

    bool isOK() const { return ok_; }

private:
    ACAN2517 mcp_;
    bool     ok_ = false;
};
