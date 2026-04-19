#pragma once
// ── Thermal protection module ─────────────────────────────────────────────────
// Samples ESP32 internal temperature sensor every 5s, maintains an EMA, and
// produces a throttle level used to slow down WiFi retry / pause uplink when
// the chip overheats. Ported from pudge9527/tesla-fsd-wifi-controller.

#ifdef WIFI_BRIDGE_ENABLED

#include <Arduino.h>
#include <cmath>
#include "esp_log.h"

// Forward declaration of ESP-IDF weak symbol provided by arduino-esp32.
extern "C" float temperatureRead();

enum class ThermalLevel : uint8_t {
    Normal = 0,
    Warning,
    Throttled,
    Protect,
};

struct ThermalStatus {
    float currentC = NAN;
    float averageC = NAN;
    uint32_t lastSampleMillis = 0;
    ThermalLevel level = ThermalLevel::Normal;
};

static ThermalStatus gThermalStatus;

static constexpr uint32_t THERMAL_SAMPLE_MS = 5000;
// Thresholds tuned for Tesla cabin summer (ambient up to 60°C).
// ESP32-S3 spec: operating ambient -40~85°C, junction limit 125°C.
// 10°C hysteresis between enter/clear prevents oscillation in hot cars.
static constexpr float CHIP_TEMP_WARN_C = 70.0f;
static constexpr float CHIP_TEMP_THROTTLE_C = 82.0f;
static constexpr float CHIP_TEMP_PROTECT_C = 88.0f;
static constexpr float CHIP_TEMP_WARN_CLEAR_C = 65.0f;
static constexpr float CHIP_TEMP_THROTTLE_CLEAR_C = 78.0f;
static constexpr float CHIP_TEMP_PROTECT_CLEAR_C = 78.0f;
static constexpr float CHIP_TEMP_EMA_ALPHA = 0.25f;

static inline const char* thermalStatusText() {
    switch (gThermalStatus.level) {
        case ThermalLevel::Warning:   return "温度偏高";
        case ThermalLevel::Throttled: return "高温降频";
        case ThermalLevel::Protect:   return "过热保护中";
        case ThermalLevel::Normal:
        default:                      return "正常";
    }
}

static inline bool thermalProtectActive() {
    return gThermalStatus.level == ThermalLevel::Protect;
}

static inline bool thermalThrottleActive() {
    return gThermalStatus.level == ThermalLevel::Throttled ||
           gThermalStatus.level == ThermalLevel::Protect;
}

static inline void thermalUpdateLevel() {
    if (!std::isfinite(gThermalStatus.averageC)) {
        gThermalStatus.level = ThermalLevel::Normal;
        return;
    }

    ThermalLevel prev = gThermalStatus.level;
    ThermalLevel next = prev;
    float avg = gThermalStatus.averageC;

    switch (prev) {
        case ThermalLevel::Protect:
            if (avg <= CHIP_TEMP_PROTECT_CLEAR_C) {
                if (avg >= CHIP_TEMP_THROTTLE_C)      next = ThermalLevel::Throttled;
                else if (avg >= CHIP_TEMP_WARN_C)     next = ThermalLevel::Warning;
                else                                  next = ThermalLevel::Normal;
            }
            break;
        case ThermalLevel::Throttled:
            if (avg >= CHIP_TEMP_PROTECT_C)           next = ThermalLevel::Protect;
            else if (avg <= CHIP_TEMP_THROTTLE_CLEAR_C)
                next = avg >= CHIP_TEMP_WARN_C ? ThermalLevel::Warning : ThermalLevel::Normal;
            break;
        case ThermalLevel::Warning:
            if (avg >= CHIP_TEMP_PROTECT_C)           next = ThermalLevel::Protect;
            else if (avg >= CHIP_TEMP_THROTTLE_C)     next = ThermalLevel::Throttled;
            else if (avg <= CHIP_TEMP_WARN_CLEAR_C)   next = ThermalLevel::Normal;
            break;
        case ThermalLevel::Normal:
        default:
            if (avg >= CHIP_TEMP_PROTECT_C)           next = ThermalLevel::Protect;
            else if (avg >= CHIP_TEMP_THROTTLE_C)     next = ThermalLevel::Throttled;
            else if (avg >= CHIP_TEMP_WARN_C)         next = ThermalLevel::Warning;
            break;
    }

    gThermalStatus.level = next;
}

static inline void serviceThermalStatus() {
    // arduino-esp32's temperatureRead() reconfigures the tsens driver on every
    // call, which emits an INFO-level "Config temperature range ..." log line.
    // Suppress once so the serial console is not flooded every THERMAL_SAMPLE_MS.
    static bool tsensLogSuppressed = false;
    if (!tsensLogSuppressed) {
        esp_log_level_set("tsens", ESP_LOG_WARN);
        tsensLogSuppressed = true;
    }

    uint32_t now = millis();
    if (gThermalStatus.lastSampleMillis != 0 && now - gThermalStatus.lastSampleMillis < THERMAL_SAMPLE_MS) {
        return;
    }
    gThermalStatus.lastSampleMillis = now;

    float currentC = temperatureRead();
    if (!std::isfinite(currentC) || currentC < -40.0f || currentC > 125.0f) {
        return;
    }

    gThermalStatus.currentC = currentC;
    if (std::isfinite(gThermalStatus.averageC)) {
        gThermalStatus.averageC =
            (gThermalStatus.averageC * (1.0f - CHIP_TEMP_EMA_ALPHA)) +
            (currentC * CHIP_TEMP_EMA_ALPHA);
    } else {
        gThermalStatus.averageC = currentC;
    }

    thermalUpdateLevel();
}

#endif // WIFI_BRIDGE_ENABLED
