/*
 * Tesla Open CAN Mod — ESP32 Web Edition
 * Core 0: WiFi AP + AsyncWebServer + OTA
 * Core 1: CAN bus read/modify/write (TWAI)
 *
 * GPLv3 — Based on tesla-open-can-mod
 */

#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
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
#include "web_ui_dash.h"

// ── WiFi AP config (NVS-overridable) ──
static char apSSID[33] = "FSD-Controller";
static char apPass[64] = "12345678";

// ── WiFi STA config (connect to router) ──
static char staSSID[33] = "";
static char staPass[64] = "";
static bool staConnected = false;

// ── Globals ──
static TWAIDriver     canDriver;
static AsyncWebServer server(80);
static Preferences    prefs;
static volatile bool  otaPendingRestart = false;
static bool           safeModeActive    = false;

// ── Time sync (browser-pushed Unix timestamp) ──────────────────────
static volatile uint32_t timeUnixBase   = 0;   // Unix time at sync point
static volatile uint32_t timeMillisBase = 0;   // millis() at sync point
static volatile bool     timeSynced     = false;

// Returns Unix timestamp (when synced) or seconds-since-boot (fallback).
static inline uint32_t getUnixTime() {
    if (timeSynced) return timeUnixBase + (millis() - timeMillisBase) / 1000;
    return (millis() - cfg.uptimeStart) / 1000;
}

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
    cfg.hw3SpeedCapEnable  = prefs.getBool("hw3Cap", false);
    cfg.precondition       = prefs.getBool("precond", false);
    strlcpy(apSSID, prefs.getString("apSSID", "FSD-Controller").c_str(), sizeof(apSSID));
    strlcpy(apPass, prefs.getString("apPass", "12345678").c_str(), sizeof(apPass));
    strlcpy(staSSID, prefs.getString("staSSID", "").c_str(), sizeof(staSSID));
    strlcpy(staPass, prefs.getString("staPass", "").c_str(), sizeof(staPass));
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
    prefs.putBool("hw3Cap",  cfg.hw3SpeedCapEnable);
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

    // Dashboard page — instrument cluster view (token checked via JS)
    server.on("/dash", HTTP_GET, [](AsyncWebServerRequest* req) {
        req->send_P(200, "text/html", DASH_HTML);
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

        char buf[1280];
        static_assert(sizeof(buf) >= 1280, "JSON buffer too small");
        snprintf(buf, sizeof(buf),
            "{\"rx\":%u,\"modified\":%u,\"errors\":%u,\"uptime\":%u,"
            "\"canOK\":%s,\"fsdTriggered\":%s,"
            "\"fsdEnable\":%d,\"hwMode\":%d,\"speedProfile\":%d,"
            "\"profileMode\":%d,\"isaChime\":%d,\"emergencyDet\":%d,\"forceActivate\":%d,"
            "\"hw3Offset\":%d,\"hw3Cap\":%s,\"precond\":%d,\"hwDetected\":%d,"
            "\"bmsSeen\":%s,\"bmsV\":%u,\"bmsA\":%d,\"bmsSoc\":%u,"
            "\"bmsMinT\":%d,\"bmsMaxT\":%d,"
            "\"freeHeap\":%u,\"safeMode\":%s,\"pinRequired\":%s,"
            "\"timeSynced\":%s,"
            "\"speedD\":%u,\"gearRaw\":%u,\"torqueF\":%d,\"torqueR\":%d,"
            "\"adaptLighting\":%s,\"hbForce\":%s,"
            "\"visionLimit\":%u,\"nagLevel\":%u,\"fcw\":%u,\"accState\":%u,\"brake\":%s,"
            "\"sideCol\":%u,\"laneWarn\":%u,\"laneChg\":%u,"
            "\"autosteer\":%s,\"aeb\":%s,\"fcwOn\":%s,"
            "\"apRestart\":%s,"
            "\"apSSID\":\"%s\",\"staSSID\":\"%s\",\"staIP\":\"%s\",\"staOK\":%s,"
            "\"version\":\"%s\"}",
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
            cfg.hw3SpeedCapEnable ? "true" : "false",
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
            timeSynced ? "true" : "false",
            (unsigned)telemSpeedRaw(), (unsigned)telemGear(),
            (int)telemTorqueFront(), (int)telemTorqueRear(),
            cfg.adaptiveLighting ? "true" : "false",
            cfg.highBeamForce    ? "true" : "false",
            (unsigned)cfg.visionSpeedLimit,
            (unsigned)cfg.nagLevel,
            (unsigned)cfg.fcwLevel,
            (unsigned)cfg.accState,
            telemBrake() ? "true" : "false",
            (unsigned)cfg.sideCollision,
            (unsigned)cfg.laneDeptWarning,
            (unsigned)cfg.laneChangeState,
            cfg.autosteerOn ? "true" : "false",
            cfg.aebOn       ? "true" : "false",
            cfg.fcwOn       ? "true" : "false",
            cfg.apRestart   ? "true" : "false",
            escapedSSID,
            staSSID,
            staConnected ? WiFi.localIP().toString().c_str() : "",
            staConnected ? "true" : "false",
            FIRMWARE_VERSION
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
        if (newPin.length() > 0 && newPin.length() < 4) { req->send(400, "text/plain", "TOO_SHORT"); return; }
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
        if (req->hasParam("hw3Cap")) {
            bool v = req->getParam("hw3Cap")->value().toInt() != 0;
            if (v != cfg.hw3SpeedCapEnable) { cfg.hw3SpeedCapEnable = v; changed = true; }
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

    // WiFi scan — returns nearby SSIDs as JSON array
    server.on("/api/scan", HTTP_GET, [](AsyncWebServerRequest* req) {
        if (!checkToken(req)) { req->send(403, "text/plain", "UNAUTH"); return; }
        int n = WiFi.scanNetworks(/*async=*/false, /*hidden=*/false);
        uint32_t up = (millis() - cfg.uptimeStart) / 1000;
        if (n <= 0) {
            char logMsg[48];
            snprintf(logMsg, sizeof(logMsg), "WiFi scan: %d networks (err=%d)", n, n);
            addDiagLog(up, logMsg);
            WiFi.scanDelete();
            req->send(200, "application/json", "[]");
            return;
        }
        char logMsg[48];
        snprintf(logMsg, sizeof(logMsg), "WiFi scan: %d networks found", n);
        addDiagLog(up, logMsg);
        String json = "[";
        for (int i = 0; i < n; i++) {
            if (i > 0) json += ",";
            String ssid = WiFi.SSID(i);
            ssid.replace("\\", "\\\\");
            ssid.replace("\"", "\\\"");
            json += "{\"ssid\":\"" + ssid + "\",\"rssi\":" + String(WiFi.RSSI(i)) + "}";
        }
        json += "]";
        WiFi.scanDelete();
        req->send(200, "application/json", json);
    });

    // WiFi STA settings — connect module to router
    server.on("/api/sta", HTTP_POST, [](AsyncWebServerRequest* req) {
        if (!checkToken(req)) { req->send(403, "text/plain", "UNAUTH"); return; }
        String newSSID = req->hasParam("ssid", true) ? req->getParam("ssid", true)->value() : "";
        String newPass = req->hasParam("pass", true) ? req->getParam("pass", true)->value() : "";
        newSSID.trim();
        Preferences wPrefs;
        wPrefs.begin("fsd", false);
        if (newSSID.length() == 0) {
            // Clear STA config
            wPrefs.putString("staSSID", "");
            wPrefs.putString("staPass", "");
        } else {
            if (newSSID.length() > 32) { req->send(400, "text/plain", "SSID too long"); wPrefs.end(); return; }
            wPrefs.putString("staSSID", newSSID);
            wPrefs.putString("staPass", newPass);
        }
        wPrefs.end();
        req->send(200, "text/plain", "OK");
        otaPendingRestart = true;
    });

    // Time sync — browser pushes its Unix timestamp so CSV rows carry real time
    server.on("/api/time", HTTP_POST, [](AsyncWebServerRequest* req) {
        if (!checkToken(req)) { req->send(403, "text/plain", "UNAUTH"); return; }
        if (!req->hasParam("ts", true)) { req->send(400, "text/plain", "Missing ts"); return; }
        uint32_t ts = (uint32_t)req->getParam("ts", true)->value().toInt();
        if (ts < 1700000000UL) { req->send(400, "text/plain", "Invalid"); return; }
        timeUnixBase   = ts;
        timeMillisBase = millis();
        timeSynced     = true;
        req->send(200, "text/plain", "OK");
    });

    // Diagnostic log — stream all entries as plain text (GET) or clear (POST /api/log/clear)
    server.on("/api/log", HTTP_GET, [](AsyncWebServerRequest* req) {
        if (!checkToken(req)) { req->send(403, "text/plain", "UNAUTH"); return; }
        AsyncResponseStream* resp = req->beginResponseStream("text/plain; charset=utf-8");
        uint16_t cnt = diagLogCount();
        if (cnt == 0) {
            resp->print("(no log entries)\n");
        } else {
            for (uint16_t i = 0; i < cnt; i++) {
                resp->print(diagLogAt(i));
                resp->print('\n');
            }
        }
        req->send(resp);
    });

    server.on("/api/log/clear", HTTP_POST, [](AsyncWebServerRequest* req) {
        if (!checkToken(req)) { req->send(403, "text/plain", "UNAUTH"); return; }
        diagLogClear();
        req->send(200, "text/plain", "OK");
    });

    // High beam force — only activatable when adaptive lighting is detected on bus
    server.on("/api/highbeam", HTTP_GET, [](AsyncWebServerRequest* req) {
        if (!checkToken(req)) { req->send(403, "text/plain", "UNAUTH"); return; }
        if (!req->hasParam("en")) { req->send(400, "text/plain", "Missing en"); return; }
        bool en = req->getParam("en")->value().toInt() != 0;
        if (en && !cfg.adaptiveLighting) {
            req->send(200, "application/json", "{\"ok\":false,\"reason\":\"not_adaptive\"}");
            return;
        }
        cfg.highBeamForce = en;
        req->send(200, "application/json", "{\"ok\":true}");
    });

    // AP auto-restart toggle
    server.on("/api/aprestart", HTTP_GET, [](AsyncWebServerRequest* req) {
        if (!checkToken(req)) { req->send(403, "text/plain", "UNAUTH"); return; }
        if (!req->hasParam("en")) { req->send(400, "text/plain", "Missing en"); return; }
        cfg.apRestart = req->getParam("en")->value().toInt() != 0;
        req->send(200, "application/json", "{\"ok\":true}");
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
    uint32_t logTick      = 0;   // counts up to 1000ms for 1 Hz event log check
    uint8_t  prevAccState = 0;   // tracks AP state transitions for auto-restart

    // Diagnostic log — state tracking for transition detection
    bool     log_prevCanOK     = false;
    bool     log_prevFsdEnable = cfg.fsdEnable;
    bool     log_prevFsdTrig   = false;
    bool     log_prevHB        = false;
    uint32_t log_prevErrCount  = 0;
    uint32_t log_fsdModAt      = 0;   // modifiedCount when FSD last triggered

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
                    addDiagLog((millis() - cfg.uptimeStart) / 1000, "CAN BUS-OFF recovery");
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

        // ── AP auto-restart on disengage ──────────────────────────────
        if (prevAccState > 0 && cfg.accState == 0 && cfg.canOK) {
            tryAPRestart(canDriver);
            if (cfg.apRestart) {
                uint32_t up = (millis() - cfg.uptimeStart) / 1000;
                addDiagLog(up, cfg.apRestartValid ? "AP disengaged -> restart injected"
                                                  : "AP disengaged -> no cache");
            }
        }
        prevAccState = cfg.accState;

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
        // Cap at 10001 so this triggers exactly once per boot, not every 49 days
        if (normalTick < 10001 && ++normalTick == 10000) {
            prefs.begin("sys", false);
            prefs.putInt("crashes", 0);
            prefs.end();
        }

        // ── Diagnostic log — 1 Hz event transition check ──────────
        if (++logTick >= 1000) {
            logTick = 0;
            uint32_t up = (millis() - cfg.uptimeStart) / 1000;
            char msg[64];

            if (cfg.canOK != log_prevCanOK) {
                addDiagLog(up, cfg.canOK ? "CAN OK" : "CAN ERROR");
                log_prevCanOK = cfg.canOK;
            }
            if (cfg.fsdEnable != log_prevFsdEnable) {
                addDiagLog(up, cfg.fsdEnable ? "FSD enable=ON" : "FSD enable=OFF");
                log_prevFsdEnable = cfg.fsdEnable;
            }
            if (cfg.fsdTriggered != log_prevFsdTrig) {
                if (cfg.fsdTriggered) {
                    addDiagLog(up, "FSD triggered");
                    log_fsdModAt = cfg.modifiedCount;
                } else {
                    snprintf(msg, sizeof(msg), "FSD released (injected %u)",
                             (unsigned)(cfg.modifiedCount - log_fsdModAt));
                    addDiagLog(up, msg);
                }
                log_prevFsdTrig = cfg.fsdTriggered;
            }
            if (cfg.highBeamForce != log_prevHB) {
                if (cfg.highBeamForce) {
                    snprintf(msg, sizeof(msg), "HB force ON adaptive=%s",
                             cfg.adaptiveLighting ? "Y" : "N");
                    addDiagLog(up, msg);
                } else {
                    addDiagLog(up, "HB force OFF");
                }
                log_prevHB = cfg.highBeamForce;
            }
            if (cfg.errorCount > log_prevErrCount) {
                snprintf(msg, sizeof(msg), "errors +%u total=%u",
                         (unsigned)(cfg.errorCount - log_prevErrCount),
                         (unsigned)cfg.errorCount);
                addDiagLog(up, msg);
                log_prevErrCount = cfg.errorCount;
            }
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
        if (crashes >= 3) {
            safeModeActive = true;
            // Reset counter now so next boot (after reflash) can escape safe mode.
            // canTask does not run in safe mode, so the counter would never be
            // cleared by the normal 10 s path — device would be permanently stuck.
            sysPrefs.putInt("crashes", 0);
        } else {
            sysPrefs.putInt("crashes", crashes);
        }
        sysPrefs.end();
        Serial.printf(safeModeActive ? "[SAFE MODE] crash count=%d, counter reset, CAN disabled\n"
                                     : "[Boot] crash count=%d\n", crashes);
    }

    // ── Watchdog: 5-second timeout ─────────────────────────────────
    esp_task_wdt_init(5, true);  // 5s timeout, panic on trigger

    // Load saved config from NVS
    loadConfig();
    Serial.printf("Config loaded: HW=%d, Profile=%d\n", cfg.hwMode, cfg.speedProfile);

    cfg.uptimeStart = millis();

    // Boot log entry
    {
        char bootMsg[64];
        snprintf(bootMsg, sizeof(bootMsg), "BOOT v%s HW%u fsd=%s",
                 FIRMWARE_VERSION, (unsigned)cfg.hwMode,
                 cfg.fsdEnable ? "ON" : "OFF");
        addDiagLog(0, bootMsg);
    }

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

    // Always use AP+STA mode so WiFi scanning is available even without a router configured
    WiFi.mode(WIFI_AP_STA);
    // Use 192.168.99.1 for AP to avoid subnet conflict with common routers (192.168.0/1/4.x)
    WiFi.softAPConfig(IPAddress(192,168,99,1), IPAddress(192,168,99,1), IPAddress(255,255,255,0));
    WiFi.softAP(apSSID, apPass[0] ? apPass : nullptr);
    Serial.printf("WiFi AP: %s  IP: %s\n", apSSID, WiFi.softAPIP().toString().c_str());

    // Connect to router if configured
    if (staSSID[0] != '\0') {
        WiFi.begin(staSSID, staPass[0] ? staPass : nullptr);
        Serial.printf("WiFi STA: connecting to '%s'...\n", staSSID);
        uint32_t t0 = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - t0 < 10000) {
            delay(200);
        }
        staConnected = (WiFi.status() == WL_CONNECTED);
        if (staConnected) {
            Serial.printf("WiFi STA connected  IP: %s\n", WiFi.localIP().toString().c_str());
            addDiagLog(0, ("STA connected " + WiFi.localIP().toString()).c_str());
        } else {
            Serial.printf("WiFi STA connect failed\n");
            addDiagLog(0, "STA connect FAILED");
        }
    }

    // Start web server
    setupWebServer();

    // mDNS — accessible via fsd.local on both AP and LAN
    if (MDNS.begin("fsd")) {
        MDNS.addService("http", "tcp", 80);
        Serial.println("mDNS: fsd.local");
        addDiagLog(0, "mDNS: fsd.local");
    } else {
        Serial.println("mDNS: start failed");
        addDiagLog(0, "mDNS start FAILED");
    }

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
