/*
 * Tesla Open CAN Mod — ESP32 Web Edition
 * Core 0: WiFi AP + AsyncWebServer + OTA
 * Core 1: CAN bus read/modify/write (TWAI)
 *
 * GPLv3 — Based on tesla-open-can-mod
 */

#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <Update.h>
#include <Preferences.h>

#include "can_frame_types.h"
#include "drivers/twai_driver.h"
#include "handlers.h"
#include "web_ui.h"

// ── WiFi AP config (NVS-overridable) ──
static char apSSID[33] = "FSD-Controller";
static char apPass[64] = "12345678";

// ── Globals ──
static TWAIDriver     canDriver;
static AsyncWebServer server(80);
static Preferences    prefs;
static volatile bool  otaPendingRestart = false;

#ifndef PIN_LED
#define PIN_LED 2   // ESP32 DevKit onboard LED
#endif

// ═══════════════════════════════════════════
//  Config persistence (NVS)
// ═══════════════════════════════════════════

void loadConfig() {
    prefs.begin("fsd", true);  // read-only
    cfg.fsdEnable          = prefs.getBool("fsdEn", true);
    cfg.hwMode             = prefs.getUChar("hwMode", 2);
    cfg.speedProfile       = prefs.getUChar("spPro", 1);
    cfg.profileModeAuto    = prefs.getBool("proAuto", true);
    cfg.isaChimeSuppress   = prefs.getBool("isaChm", false);
    cfg.emergencyDetection = prefs.getBool("emDet", true);
    cfg.chinaMode          = prefs.getBool("cnMode", false);
    strlcpy(apSSID, prefs.getString("apSSID", "FSD-Controller").c_str(), sizeof(apSSID));
    strlcpy(apPass, prefs.getString("apPass", "12345678").c_str(), sizeof(apPass));
    prefs.end();

    // Clamp values
    if (cfg.hwMode > 2)       cfg.hwMode = 2;
    if (cfg.speedProfile > 4) cfg.speedProfile = 1;
}

void saveConfig() {
    prefs.begin("fsd", false);  // read-write
    prefs.putBool("fsdEn",   cfg.fsdEnable);
    prefs.putUChar("hwMode", cfg.hwMode);
    prefs.putUChar("spPro",  cfg.speedProfile);
    prefs.putBool("proAuto", cfg.profileModeAuto);
    prefs.putBool("isaChm",  cfg.isaChimeSuppress);
    prefs.putBool("emDet",   cfg.emergencyDetection);
    prefs.putBool("cnMode",  cfg.chinaMode);
    prefs.end();
}

// ═══════════════════════════════════════════
//  Web Server Setup (runs on Core 0)
// ═══════════════════════════════════════════

void setupWebServer() {
    // Serve UI
    server.on("/", HTTP_GET, [](AsyncWebServerRequest* req) {
        req->send_P(200, "text/html", INDEX_HTML);
    });

    // Status JSON — use %u for uint32_t on ESP32
    server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest* req) {
        uint32_t uptime = (millis() - cfg.uptimeStart) / 1000;
        char buf[320];
        snprintf(buf, sizeof(buf),
            "{\"rx\":%u,\"modified\":%u,\"errors\":%u,\"uptime\":%u,"
            "\"canOK\":%s,\"fsdTriggered\":%s,"
            "\"fsdEnable\":%d,\"hwMode\":%d,\"speedProfile\":%d,"
            "\"profileMode\":%d,\"isaChime\":%d,\"emergencyDet\":%d,\"chinaMode\":%d,"
            "\"apSSID\":\"%s\"}",
            (unsigned)cfg.rxCount, (unsigned)cfg.modifiedCount,
            (unsigned)cfg.errorCount, (unsigned)uptime,
            cfg.canOK ? "true" : "false",
            cfg.fsdTriggered ? "true" : "false",
            (int)cfg.fsdEnable,
            (int)cfg.hwMode,
            (int)cfg.speedProfile,
            (int)cfg.profileModeAuto,
            (int)cfg.isaChimeSuppress,
            (int)cfg.emergencyDetection,
            (int)cfg.chinaMode,
            apSSID
        );
        req->send(200, "application/json", buf);
    });

    // Set config — with input validation
    server.on("/api/set", HTTP_GET, [](AsyncWebServerRequest* req) {
        bool changed = false;

        if (req->hasParam("fsdEnable")) {
            cfg.fsdEnable = req->getParam("fsdEnable")->value().toInt() != 0;
            changed = true;
        }
        if (req->hasParam("hwMode")) {
            uint8_t v = req->getParam("hwMode")->value().toInt();
            if (v <= 2) { cfg.hwMode = v; changed = true; }
        }
        if (req->hasParam("speedProfile")) {
            uint8_t v = req->getParam("speedProfile")->value().toInt();
            if (v <= 4) { cfg.speedProfile = v; changed = true; }
        }
        if (req->hasParam("profileMode")) {
            cfg.profileModeAuto = req->getParam("profileMode")->value().toInt() != 0;
            changed = true;
        }
        if (req->hasParam("isaChime")) {
            cfg.isaChimeSuppress = req->getParam("isaChime")->value().toInt() != 0;
            changed = true;
        }
        if (req->hasParam("emergencyDet")) {
            cfg.emergencyDetection = req->getParam("emergencyDet")->value().toInt() != 0;
            changed = true;
        }
        if (req->hasParam("chinaMode")) {
            cfg.chinaMode = req->getParam("chinaMode")->value().toInt() != 0;
            changed = true;
        }

        if (changed) saveConfig();
        req->send(200, "text/plain", "OK");
    });

    // WiFi AP settings — save to NVS then restart
    server.on("/api/wifi", HTTP_POST, [](AsyncWebServerRequest* req) {
        if (!req->hasParam("ssid", true)) {
            req->send(400, "text/plain", "Missing ssid");
            return;
        }
        String newSSID = req->getParam("ssid", true)->value();
        String newPass = req->hasParam("pass", true) ? req->getParam("pass", true)->value() : "";
        newSSID.trim();
        if (newSSID.length() == 0 || newSSID.length() > 32) {
            req->send(400, "text/plain", "SSID must be 1-32 chars");
            return;
        }
        if (newPass.length() > 0 && newPass.length() < 8) {
            req->send(400, "text/plain", "Password must be >= 8 chars");
            return;
        }
        Preferences wPrefs;
        wPrefs.begin("fsd", false);
        wPrefs.putString("apSSID", newSSID);
        if (newPass.length() >= 8) wPrefs.putString("apPass", newPass);
        wPrefs.end();
        req->send(200, "text/plain", "OK");
        otaPendingRestart = true;
    });

    // OTA firmware upload — flag-based restart (no delay in async context)
    server.on("/api/ota", HTTP_POST,
        [](AsyncWebServerRequest* req) {
            bool ok = !Update.hasError();
            req->send(200, "text/plain", ok ? "OK" : "FAIL");
            if (ok) otaPendingRestart = true;
        },
        [](AsyncWebServerRequest* req, const String& filename,
           size_t index, uint8_t* data, size_t len, bool final) {
            if (index == 0) {
                Serial.printf("OTA start: %s\n", filename.c_str());
                if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
                    Update.printError(Serial);
                }
            }
            if (Update.isRunning()) {
                if (Update.write(data, len) != len) {
                    Update.printError(Serial);
                }
            }
            if (final) {
                if (Update.end(true)) {
                    Serial.printf("OTA done: %u bytes\n", (unsigned)(index + len));
                } else {
                    Update.printError(Serial);
                }
            }
        }
    );

    server.begin();
    Serial.println("Web server started");
}

// ═══════════════════════════════════════════
//  CAN Task (pinned to Core 1)
// ═══════════════════════════════════════════

void canTask(void* param) {
    CanFrame frame;
    for (;;) {
        bool activity = false;
        while (canDriver.read(frame)) {
            cfg.canOK = true;
            activity = true;
            handleMessage(frame, canDriver);
        }
        // LED: on during activity, off when idle
        digitalWrite(PIN_LED, activity ? HIGH : LOW);
        // Yield to avoid starving watchdog
        vTaskDelay(1);
    }
}

// ═══════════════════════════════════════════
//  Arduino setup / loop
// ═══════════════════════════════════════════

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n=== FSD Controller ===");

    // LED
    pinMode(PIN_LED, OUTPUT);
    digitalWrite(PIN_LED, LOW);

    // Load saved config from NVS
    loadConfig();
    Serial.printf("Config loaded: HW=%d, Profile=%d\n", cfg.hwMode, cfg.speedProfile);

    cfg.uptimeStart = millis();

    // Init CAN
    if (canDriver.init()) {
        cfg.canOK = true;
        Serial.println("ESP32 TWAI ready @ 500k");
    } else {
        cfg.canOK = false;
        Serial.println("CAN init failed!");
    }

    // Start WiFi AP
    WiFi.mode(WIFI_AP);
    WiFi.softAP(apSSID, apPass[0] ? apPass : nullptr);
    Serial.printf("WiFi AP: %s  IP: %s\n", apSSID, WiFi.softAPIP().toString().c_str());

    // Start web server
    setupWebServer();

    // Pin CAN processing to Core 1 (8KB stack for safety)
    xTaskCreatePinnedToCore(canTask, "CAN", 8192, NULL, 2, NULL, 1);
}

void loop() {
    // Handle OTA restart safely outside async context
    if (otaPendingRestart) {
        delay(1000);  // let response finish sending
        ESP.restart();
    }
    vTaskDelay(1000);
}
