#pragma once
#include "can_driver.h"
#include <driver/twai.h>

#ifndef TWAI_TX_PIN
#define TWAI_TX_PIN GPIO_NUM_5
#endif
#ifndef TWAI_RX_PIN
#define TWAI_RX_PIN GPIO_NUM_4
#endif

struct TWAIDriver : public CanDriver {
    gpio_num_t txPin, rxPin;
    static constexpr bool kSupportsISR = false;

    TWAIDriver(gpio_num_t tx = (gpio_num_t)TWAI_TX_PIN,
               gpio_num_t rx = (gpio_num_t)TWAI_RX_PIN)
        : txPin(tx), rxPin(rx) {}

    bool init() override {
        twai_general_config_t g = TWAI_GENERAL_CONFIG_DEFAULT(txPin, rxPin, TWAI_MODE_NORMAL);
        g.rx_queue_len = 32;
        g.tx_queue_len = 16;
        twai_timing_config_t t = TWAI_TIMING_CONFIG_500KBITS();
        twai_filter_config_t f = TWAI_FILTER_CONFIG_ACCEPT_ALL();
        if (twai_driver_install(&g, &t, &f) != ESP_OK) return false;
        if (twai_start() != ESP_OK) return false;
        driverOK_ = true;
        return true;
    }

    bool send(const CanFrame& frame) override {
        if (!driverOK_) return false;
        twai_message_t msg = {};
        msg.identifier        = frame.id;
        msg.data_length_code  = frame.dlc;
        memcpy(msg.data, frame.data, 8);
        if (twai_transmit(&msg, pdMS_TO_TICKS(2)) != ESP_OK) {
            if (isBusOff()) recover();
            return false;
        }
        return true;
    }

    bool read(CanFrame& frame) override {
        if (!driverOK_) {
            tryRecover();
            return false;
        }
        twai_message_t msg;
        if (twai_receive(&msg, 0) != ESP_OK) {
            if (isBusOff()) recover();
            return false;
        }
        frame.id  = msg.identifier;
        frame.dlc = (msg.data_length_code <= 8) ? msg.data_length_code : 8;
        memset(frame.data, 0, 8);
        memcpy(frame.data, msg.data, frame.dlc);
        return true;
    }

    void setFilters(const uint32_t* /*ids*/, uint8_t /*count*/) override {
        // Software filtering via isFilteredId() in handlers.h
    }

private:
    static constexpr uint32_t BUSOFF_COOLDOWN_MS  = 1000;
    static constexpr uint32_t DRIVERFAIL_RETRY_MS = 10000;

    bool     driverOK_      = false;
    uint32_t lastRecovery_  = 0;

    bool isBusOff() {
        twai_status_info_t status;
        if (twai_get_status_info(&status) != ESP_OK) return false;
        return status.state == TWAI_STATE_BUS_OFF;
    }

    void recover() {
        uint32_t now = millis();
        if (now - lastRecovery_ < BUSOFF_COOLDOWN_MS) return;
        lastRecovery_ = now;
        // Use ESP-IDF native recovery API — lighter than full reinstall.
        twai_initiate_recovery();
    }

    void tryRecover() {
        uint32_t now = millis();
        if (now - lastRecovery_ < DRIVERFAIL_RETRY_MS) return;
        lastRecovery_ = now;
        twai_driver_uninstall();
        twai_general_config_t g = TWAI_GENERAL_CONFIG_DEFAULT(txPin, rxPin, TWAI_MODE_NORMAL);
        g.rx_queue_len = 32;
        g.tx_queue_len = 16;
        twai_timing_config_t t = TWAI_TIMING_CONFIG_500KBITS();
        twai_filter_config_t f = TWAI_FILTER_CONFIG_ACCEPT_ALL();
        if (twai_driver_install(&g, &t, &f) == ESP_OK && twai_start() == ESP_OK)
            driverOK_ = true;
    }
};
