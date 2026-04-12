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
#include <esp_task_wdt.h>
#include <driver/twai.h>

#include "can_frame_types.h"
#include "drivers/twai_driver.h"
#include "handlers.h"
#include "version.h"
#include "web_ui.h"

// ── WiFi AP config (NVS-overridable) ──
static char apSSID[33] = "FSD-Controller";
static char apPass[64] = "12345678";

// ── Globals ──
static TWAIDriver     canDriver;
static AsyncWebServer server(80);
static Preferences    prefs;
static volatile bool  otaPendingRestart = false;
static bool           safeModeActive    = false;

// ── Auth ──────────────────────────────────────────────────────────
static char sessionToken[17] = {0};  // 16 hex chars, reset on reboot
static char storedPin[17]    = {0};  // loaded from NVS; empty = no auth

static void generateToken() {
    uint32_t r1 = esp_random(), r2 = esp_random();
    snprintf(sessionToken, sizeof(sessionToken), "%08X%08X", r1, r2);
}

// Returns true if request is authorised.
// If no PIN is configured, always returns true (open access).
static bool checkToken(AsyncWebServerRequest* req) {
    if (storedPin[0] == '\0') return true;           // no PIN set → open
    if (sessionToken[0] == '\0') return false;        // PIN set but no token yet
    if (req->hasParam("token") &&
        req->getParam("token")->value().equals(sessionToken)) return true;
    return false;
}

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
    cfg.forceActivate      = prefs.getBool("cnMode", false);
    cfg.hw3OffsetManual    = prefs.getInt("hw3Off", -1);
    cfg.precondition       = prefs.getBool("precond", false);
    strlcpy(apSSID, prefs.getString("apSSID", "FSD-Controller").c_str(), sizeof(apSSID));
    strlcpy(apPass, prefs.getString("apPass", "12345678").c_str(), sizeof(apPass));
    prefs.end();

    // Load PIN from separate namespace
    Preferences secPrefs;
    secPrefs.begin("sec", true);
    strlcpy(storedPin, secPrefs.getString("pin", "").c_str(), sizeof(storedPin));
    secPrefs.end();

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
    prefs.putBool("cnMode",  cfg.forceActivate);
    prefs.putInt("hw3Off",   cfg.hw3OffsetManual);
    prefs.putBool("precond", cfg.precondition);
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

        // JSON-escape apSSID (guard against " and \ in SSID)
        char escapedSSID[128] = {};
        const char* src = apSSID;
        char* dst = escapedSSID;
        char* end = escapedSSID + sizeof(escapedSSID) - 3;
        while (*src && dst < end) {
            if (*src == '"' || *src == '\\') *dst++ = '\\';
            *dst++ = *src++;
        }

        char buf[1200];
        static_assert(sizeof(buf) >= 1200, "JSON buffer too small");
        snprintf(buf, sizeof(buf),
            "{\"rx\":%u,\"modified\":%u,\"errors\":%u,\"uptime\":%u,"
            "\"canOK\":%s,\"fsdTriggered\":%s,"
            "\"fsdEnable\":%d,\"hwMode\":%d,\"speedProfile\":%d,"
            "\"profileMode\":%d,\"isaChime\":%d,\"emergencyDet\":%d,\"forceActivate\":%d,"
            "\"hw3Offset\":%d,\"precond\":%d,\"hwDetected\":%d,"
            "\"bmsSeen\":%s,\"bmsV\":%u,\"bmsA\":%d,\"bmsSoc\":%u,"
            "\"bmsMinT\":%d,\"bmsMaxT\":%d,"
            "\"freeHeap\":%u,\"safeMode\":%s,\"pinRequired\":%s,"
            "\"apSSID\":\"%s\",\"version\":\"%s\"}",
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
            (int)cfg.forceActivate,
            (int)cfg.hw3OffsetManual,
            (int)cfg.precondition,
            (int)cfg.hwDetected,
            cfg.bmsSeen ? "true" : "false",
            (unsigned)cfg.packVoltage_cV,   // ÷100 = V  (done in JS)
            (int)cfg.packCurrent_dA,        // ÷10  = A  (done in JS)
            (unsigned)cfg.socPercent_d,     // ÷10  = %  (done in JS)
            (int)cfg.battTempMin,
            (int)cfg.battTempMax,
            (unsigned)esp_get_free_heap_size(),
            safeModeActive ? "true" : "false",
            (storedPin[0] != '\0') ? "true" : "false",
            escapedSSID, FIRMWARE_VERSION
        );

#ifdef DEBUG_MODE
        if (cfg.dbgFrameCaptured) {
            char dbg[120];
            snprintf(dbg, sizeof(dbg),
                ",\"dbg\":{\"captured\":true,\"bytes\":\"%02X %02X %02X %02X %02X %02X %02X %02X\",\"bit30\":%d}",
                cfg.dbgFrame[0], cfg.dbgFrame[1], cfg.dbgFrame[2], cfg.dbgFrame[3],
                cfg.dbgFrame[4], cfg.dbgFrame[5], cfg.dbgFrame[6], cfg.dbgFrame[7],
                (cfg.dbgFrame[3] >> 6) & 0x01
            );
            size_t len = strlen(buf);
            buf[len - 1] = '\0';
            strlcat(buf, dbg, sizeof(buf));
            strlcat(buf, "}", sizeof(buf));
        }
#endif
        req->send(200, "application/json", buf);
    });

    // Auth — validate PIN, return session token
    server.on("/api/auth", HTTP_POST, [](AsyncWebServerRequest* req) {
        String pin = req->hasParam("pin", true) ? req->getParam("pin", true)->value() : "";
        if (strcmp(pin.c_str(), storedPin) == 0) {
            generateToken();
            req->send(200, "text/plain", sessionToken);
        } else {
            req->send(403, "text/plain", "WRONG");
        }
    });

    // Change PIN (requires current token)
    server.on("/api/pin", HTTP_POST, [](AsyncWebServerRequest* req) {
        if (!checkToken(req)) { req->send(403, "text/plain", "UNAUTH"); return; }
        String newPin = req->hasParam("pin", true) ? req->getParam("pin", true)->value() : "";
        if (newPin.length() > 16) { req->send(400, "text/plain", "TOO_LONG"); return; }
        strlcpy(storedPin, newPin.c_str(), sizeof(storedPin));
        Preferences secPrefs;
        secPrefs.begin("sec", false);
        secPrefs.putString("pin", newPin);
        secPrefs.end();
        sessionToken[0] = '\0';  // invalidate existing tokens
        req->send(200, "text/plain", "OK");
    });

    // Set config — with input validation and NVS write-only-on-change
    server.on("/api/set", HTTP_GET, [](AsyncWebServerRequest* req) {
        if (!checkToken(req)) { req->send(403, "text/plain", "UNAUTH"); return; }
        bool changed = false;

        if (req->hasParam("fsdEnable")) {
            bool v = req->getParam("fsdEnable")->value().toInt() != 0;
            if (v != cfg.fsdEnable) { cfg.fsdEnable = v; changed = true; }
        }
        if (req->hasParam("hwMode")) {
            uint8_t v = req->getParam("hwMode")->value().toInt();
            if (v <= 2 && v != cfg.hwMode) { cfg.hwMode = v; changed = true; }
        }
        if (req->hasParam("speedProfile")) {
            uint8_t v = req->getParam("speedProfile")->value().toInt();
            if (v <= 4 && v != cfg.speedProfile) { cfg.speedProfile = v; changed = true; }
        }
        if (req->hasParam("profileMode")) {
            bool v = req->getParam("profileMode")->value().toInt() != 0;
            if (v != cfg.profileModeAuto) { cfg.profileModeAuto = v; changed = true; }
        }
        if (req->hasParam("isaChime")) {
            bool v = req->getParam("isaChime")->value().toInt() != 0;
            if (v != cfg.isaChimeSuppress) { cfg.isaChimeSuppress = v; changed = true; }
        }
        if (req->hasParam("emergencyDet")) {
            bool v = req->getParam("emergencyDet")->value().toInt() != 0;
            if (v != cfg.emergencyDetection) { cfg.emergencyDetection = v; changed = true; }
        }
        if (req->hasParam("forceActivate")) {
            bool v = req->getParam("forceActivate")->value().toInt() != 0;
            if (v != cfg.forceActivate) { cfg.forceActivate = v; changed = true; }
        }
        if (req->hasParam("hw3Offset")) {
            int v = req->getParam("hw3Offset")->value().toInt();
            if ((v == -1 || (v >= 0 && v <= 50)) && v != cfg.hw3OffsetManual) {
                cfg.hw3OffsetManual = v; changed = true;
            }
        }
        if (req->hasParam("precond")) {
            bool v = req->getParam("precond")->value().toInt() != 0;
            if (v != cfg.precondition) { cfg.precondition = v; changed = true; }
        }

        if (changed) saveConfig();
        req->send(200, "text/plain", "OK");
    });

    // WiFi AP settings — save to NVS then restart
    server.on("/api/wifi", HTTP_POST, [](AsyncWebServerRequest* req) {
        if (!checkToken(req)) { req->send(403, "text/plain", "UNAUTH"); return; }
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
            if (!checkToken(req)) { req->send(403, "text/plain", "UNAUTH"); return; }
            bool ok = !Update.hasError();
            req->send(200, "text/plain", ok ? "OK" : "FAIL");
            if (ok) otaPendingRestart = true;
        },
        [](AsyncWebServerRequest* req, const String& filename,
           size_t index, uint8_t* data, size_t len, bool final) {
            if (!checkToken(req)) return;  // block unauthorized uploads
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
    // Register this task with the watchdog (5 second timeout)
    esp_task_wdt_add(NULL);

    CanFrame frame;
    uint32_t precondTick  = 0;
    uint32_t busOffTick   = 0;
    uint32_t normalTick   = 0;   // counts up after init; clears crash counter at 10s

    for (;;) {
        // Feed watchdog — if this task hangs, ESP32 resets after 5s
        esp_task_wdt_reset();

        // ── Bus-Off auto-recovery ──────────────────────────────────
        twai_status_info_t twaiStatus;
        if (twai_get_status_info(&twaiStatus) == ESP_OK) {
            if (twaiStatus.state == TWAI_STATE_BUS_OFF) {
                cfg.canOK = false;
                if (++busOffTick >= 500) {   // ~500ms in bus-off → trigger recovery
                    busOffTick = 0;
                    twai_initiate_recovery();
                    Serial.println("[CAN] Bus-Off recovery initiated");
                }
            } else if (twaiStatus.state == TWAI_STATE_RUNNING) {
                busOffTick = 0;
            }
        }

        // ── Normal frame processing ────────────────────────────────
        bool activity = false;
        while (canDriver.read(frame)) {
            cfg.canOK = true;
            activity = true;
            handleMessage(frame, canDriver);
        }

        // ── Precondition frame every ~1 s when enabled ─────────────
        if (cfg.precondition && cfg.canOK) {
            if (++precondTick >= 1000) {
                precondTick = 0;
                CanFrame pcFrame;
                buildPreconditionFrame(pcFrame);
                canDriver.send(pcFrame);
            }
        } else {
            precondTick = 0;
        }

        // ── Clear crash counter after 10s of normal operation ──────
        if (++normalTick == 10000) {
            prefs.begin("sys", false);
            prefs.putInt("crashes", 0);
            prefs.end();
        }

        // LED: on during activity, off when idle
        digitalWrite(PIN_LED, activity ? HIGH : LOW);
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

    // ── Crash counter / safe mode ──────────────────────────────────
    // If the device crashes 3+ times in a row within 10s, enter safe mode
    // (WiFi only, no CAN) so the user can re-flash firmware.
    {
        Preferences sysPrefs;
        sysPrefs.begin("sys", false);
        int crashes = sysPrefs.getInt("crashes", 0) + 1;
        sysPrefs.putInt("crashes", crashes);
        sysPrefs.end();
        if (crashes >= 3) {
            safeModeActive = true;
            Serial.printf("[SAFE MODE] crash count=%d, CAN disabled\n", crashes);
        } else {
            Serial.printf("[Boot] crash count=%d\n", crashes);
        }
    }

    // ── Watchdog: 5-second timeout ─────────────────────────────────
    esp_task_wdt_init(5, true);  // 5s timeout, panic on trigger

    // Load saved config from NVS
    loadConfig();
    Serial.printf("Config loaded: HW=%d, Profile=%d\n", cfg.hwMode, cfg.speedProfile);

    cfg.uptimeStart = millis();

    // Init CAN (skip in safe mode)
    if (!safeModeActive) {
        if (canDriver.init()) {
            cfg.canOK = true;
            Serial.println("ESP32 TWAI ready @ 500k");
        } else {
            cfg.canOK = false;
            Serial.println("CAN init failed!");
        }
    } else {
        cfg.canOK = false;
    }

    // Start WiFi AP
    WiFi.mode(WIFI_AP);
    WiFi.softAP(apSSID, apPass[0] ? apPass : nullptr);
    Serial.printf("WiFi AP: %s  IP: %s\n", apSSID, WiFi.softAPIP().toString().c_str());

    // Start web server
    setupWebServer();

    // Pin CAN processing to Core 1 — skip in safe mode
    if (!safeModeActive) {
        xTaskCreatePinnedToCore(canTask, "CAN", 8192, NULL, 2, NULL, 1);
    }
}

void loop() {
    // Handle OTA restart safely outside async context
    if (otaPendingRestart) {
        delay(1000);  // let response finish sending
        ESP.restart();
    }
    vTaskDelay(1000);
}
