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
        return twai_start() == ESP_OK;
    }

    bool send(const CanFrame& frame) override {
        twai_message_t msg = {};
        msg.identifier        = frame.id;
        msg.data_length_code  = frame.dlc;
        memcpy(msg.data, frame.data, 8);
        if (twai_transmit(&msg, pdMS_TO_TICKS(10)) != ESP_OK) {
            if (isBusOff()) recover();
            return false;
        }
        return true;
    }

    bool read(CanFrame& frame) override {
        twai_message_t msg;
        if (twai_receive(&msg, 0) != ESP_OK) {
            if (isBusOff()) recover();
            return false;
        }
        frame.id  = msg.identifier;
        frame.dlc = msg.data_length_code;
        memcpy(frame.data, msg.data, 8);
        return true;
    }

    void setFilters(const uint32_t* /*ids*/, uint8_t /*count*/) override {
        // Software filtering via isFilteredId() in handlers.h
    }

private:
    bool isBusOff() {
        twai_status_info_t status;
        if (twai_get_status_info(&status) != ESP_OK) return false;
        return status.state == TWAI_STATE_BUS_OFF;
    }

    void recover() {
        // Use ESP-IDF native recovery API — lighter than full reinstall.
        // Driver handles the 128-bit recovery sequence automatically.
        twai_initiate_recovery();
    }
};
