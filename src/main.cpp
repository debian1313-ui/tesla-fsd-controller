/*
 * Tesla Open CAN Mod — ESP32 Web Edition
 * Core 0: WiFi AP + AsyncWebServer + OTA
 * Core 1: CAN bus read/modify/write (TWAI)
 *
 * GPLv3 — Based on tesla-open-can-mod
 */

#include <Arduino.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <ESPAsyncWebServer.h>
#include <Update.h>
#include <Preferences.h>
#include <esp_task_wdt.h>
#include <driver/twai.h>
#include <esp_netif.h>
#include <SPIFFS.h>

#include "can_frame_types.h"
#include "drivers/twai_driver.h"
#include "handlers.h"
#include "version.h"
#include "web_ui.h"
#include "web_ui_dash.h"
#include "web_perf.h"

// ── WiFi AP config (NVS-overridable) ──
static char apSSID[33] = "FSD-Controller";
static char apPass[64] = "12345678";

// ── WiFi STA config (connect to router) ──
static char staSSID[33] = "";
static char staPass[64] = "";
static bool staConnected = false;

// ── Globals ──
static DNSServer      dnsServer;
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
    cfg.apRestart          = prefs.getBool("apRestart", false);
    cfg.hw3SmartEnable     = prefs.getBool("hw3SmEn", false);
    cfg.hw3SmartT1         = prefs.getUChar("hw3SmT1", 60);
    cfg.hw3SmartT2         = prefs.getUChar("hw3SmT2", 100);
    cfg.hw3SmartO1         = prefs.getUChar("hw3SmO1", 20);
    cfg.hw3SmartO2         = prefs.getUChar("hw3SmO2", 15);
    cfg.hw3SmartO3         = prefs.getUChar("hw3SmO3", 10);
    cfg.precondition       = prefs.getBool("precond",  false);
    cfg.nagKiller          = prefs.getBool("nagKill",  false);
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

    cfg.hw3SmartLastKmh = cfg.hw3SmartO2;  // sync fallback to current tier-2 default
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
    prefs.putInt("hw3Off",    cfg.hw3OffsetManual);
    prefs.putBool("apRestart", cfg.apRestart);
    prefs.putBool("hw3SmEn",   cfg.hw3SmartEnable);
    prefs.putUChar("hw3SmT1", cfg.hw3SmartT1);
    prefs.putUChar("hw3SmT2", cfg.hw3SmartT2);
    prefs.putUChar("hw3SmO1", cfg.hw3SmartO1);
    prefs.putUChar("hw3SmO2", cfg.hw3SmartO2);
    prefs.putUChar("hw3SmO3", cfg.hw3SmartO3);
    prefs.putBool("precond",   cfg.precondition);
    prefs.putBool("nagKill",   cfg.nagKiller);
    prefs.end();
}

// ═══════════════════════════════════════════
//  SPIFFS persistent log  (Core 0 only)
// ═══════════════════════════════════════════
// File: /diag.log  max ~96 KB.
// When the file exceeds SPIFFS_LOG_MAX, the back half is kept (rotate in-place).
// All writes happen from Core 0 (loop / setupWebServer) — never from canTask.

static constexpr size_t SPIFFS_LOG_MAX   = 96 * 1024;  // 96 KB rotate threshold
static constexpr size_t SPIFFS_LOG_KEEP  = 48 * 1024;  // keep last 48 KB after rotate
static const char*      SPIFFS_LOG_PATH  = "/diag.log";
static bool             spiffsOK         = false;

// Rotate: keep only the last SPIFFS_LOG_KEEP bytes of the file.
static void spiffsRotate() {
    File f = SPIFFS.open(SPIFFS_LOG_PATH, "r");
    if (!f) return;
    size_t sz = f.size();
    if (sz <= SPIFFS_LOG_KEEP) { f.close(); return; }
    size_t skip = sz - SPIFFS_LOG_KEEP;
    f.seek(skip);
    // Read tail into a temporary buffer and rewrite the file.
    // We do this in 2 KB chunks to avoid a large stack allocation.
    static uint8_t rotBuf[2048];
    File tmp = SPIFFS.open("/diag.tmp", "w");
    if (!tmp) { f.close(); return; }
    while (f.available()) {
        size_t n = f.readBytes((char*)rotBuf, sizeof(rotBuf));
        if (n > 0) tmp.write(rotBuf, n);
    }
    f.close();
    tmp.close();
    SPIFFS.remove(SPIFFS_LOG_PATH);
    SPIFFS.rename("/diag.tmp", SPIFFS_LOG_PATH);
}

// Called once during setup — mount SPIFFS and write a BOOT marker.
static void setupSpiffs(const char* fwVersion) {
    if (!SPIFFS.begin(/*formatOnFail=*/true)) {
        Serial.println("[SPIFFS] mount failed");
        return;
    }
    spiffsOK = true;
    File f = SPIFFS.open(SPIFFS_LOG_PATH, "a");
    if (f) {
        char marker[64];
        snprintf(marker, sizeof(marker), "\n=== BOOT v%s ===\n", fwVersion);
        f.print(marker);
        f.close();
    }
    size_t total = SPIFFS.totalBytes();
    size_t used  = SPIFFS.usedBytes();
    Serial.printf("[SPIFFS] mounted  total=%u used=%u\n", (unsigned)total, (unsigned)used);
}

// Called from loop() every 3 s — appends any new RAM ring-buffer entries to flash.
static void flushLogsToSpiffs() {
    if (!spiffsOK) return;
    uint32_t total = diagTotal;  // snapshot (volatile read)
    if (total == diagFlushed) return;  // nothing new

    File f = SPIFFS.open(SPIFFS_LOG_PATH, "a");
    if (!f) return;

    // Walk the ring buffer from diagFlushed to total.
    // diagBuf holds at most DIAG_CAP entries; if more than DIAG_CAP have been written
    // since last flush we can only recover the most recent DIAG_CAP.
    uint32_t pending = total - diagFlushed;
    uint32_t skip    = (pending > DIAG_CAP) ? pending - DIAG_CAP : 0;
    uint32_t start   = diagFlushed + skip;

    for (uint32_t t = start; t < total; t++) {
        // Map monotonic index t → ring buffer slot.
        // diagHead points to the *next* write slot, so the oldest entry in the
        // ring is at (diagHead + DIAG_CAP - diagCount) % DIAG_CAP when full.
        // Simpler: entry at absolute index t is at slot t % DIAG_CAP.
        uint16_t slot = (uint16_t)(t % DIAG_CAP);
        f.print(diagBuf[slot]);
        f.print('\n');
    }
    f.close();
    diagFlushed = total;

    // Rotate if the file has grown too large.
    File fr = SPIFFS.open(SPIFFS_LOG_PATH, "r");
    if (fr) {
        size_t sz = fr.size();
        fr.close();
        if (sz >= SPIFFS_LOG_MAX) spiffsRotate();
    }
}

// ═══════════════════════════════════════════
//  Web Server Setup (runs on Core 0)
// ═══════════════════════════════════════════

void setupWebServer() {
    // ── Captive portal — iOS & Android auto-detection ─────────────────
    // iOS tries /hotspot-detect.html; redirect to main page.
    server.on("/hotspot-detect.html", HTTP_GET, [](AsyncWebServerRequest* req) {
        req->redirect("http://" + WiFi.softAPIP().toString() + "/");
    });
    // iOS 14+
    server.on("/library/test/success.html", HTTP_GET, [](AsyncWebServerRequest* req) {
        req->send(200, "text/html", "<HTML><HEAD><TITLE>Success</TITLE></HEAD><BODY>Success</BODY></HTML>");
    });
    // Android generate_204
    server.on("/generate_204", HTTP_GET, [](AsyncWebServerRequest* req) {
        req->redirect("http://" + WiFi.softAPIP().toString() + "/");
    });
    // Windows NCSI
    server.on("/connecttest.txt", HTTP_GET, [](AsyncWebServerRequest* req) {
        req->send(200, "text/plain", "Microsoft NCSI");
    });
    server.on("/ncsi.txt", HTTP_GET, [](AsyncWebServerRequest* req) {
        req->send(200, "text/plain", "Microsoft NCSI");
    });

    // Serve UI
    server.on("/", HTTP_GET, [](AsyncWebServerRequest* req) {
        req->send_P(200, "text/html", INDEX_HTML);
    });

    // Dashboard page — instrument cluster view (token checked via JS)
    server.on("/dash", HTTP_GET, [](AsyncWebServerRequest* req) {
        req->send_P(200, "text/html", DASH_HTML);
    });

    // Performance test page
    server.on("/perf", HTTP_GET, [](AsyncWebServerRequest* req) {
        req->send_P(200, "text/html", PERF_HTML);
    });

    // Performance test control API
    server.on("/api/perf", HTTP_GET, [](AsyncWebServerRequest* req) {
        if (!checkToken(req)) { req->send(403, "text/plain", "Forbidden"); return; }
        if (req->hasParam("cmd")) {
            String cmd = req->getParam("cmd")->value();
            if (cmd == "arm_accel")   { cfg.perfAccelState = 1; cfg.perfAccelMs = 0; }
            else if (cmd == "arm_brake")  { cfg.perfBrakeState = 1; cfg.perfBrakeMs = 0; }
            else if (cmd == "reset_accel"){ cfg.perfAccelState = 0; cfg.perfAccelMs = 0; }
            else if (cmd == "reset_brake"){ cfg.perfBrakeState = 0; cfg.perfBrakeMs = 0; }
            else if (cmd == "reset")  { cfg.perfAccelState = 0; cfg.perfAccelMs = 0;
                                        cfg.perfBrakeState = 0; cfg.perfBrakeMs = 0; }
        }
        req->send(200, "application/json", "{\"ok\":true}");
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

        char buf[1500];
        static_assert(sizeof(buf) >= 1500, "JSON buffer too small");
        snprintf(buf, sizeof(buf),
            "{\"rx\":%u,\"modified\":%u,\"errors\":%u,\"uptime\":%u,"
            "\"canOK\":%s,\"fsdTriggered\":%s,"
            "\"fsdEnable\":%d,\"hwMode\":%d,\"speedProfile\":%d,"
            "\"profileMode\":%d,\"isaChime\":%d,\"emergencyDet\":%d,\"forceActivate\":%d,"
            "\"hw3Offset\":%d,\"precond\":%d,\"hwDetected\":%d,"
            "\"hw3Smart\":%d,\"hw3SmT1\":%d,\"hw3SmT2\":%d,"
            "\"hw3SmO1\":%d,\"hw3SmO2\":%d,\"hw3SmO3\":%d,"
            "\"fusedLimit\":%u,\"smartTier\":%u,\"smartKmh\":%u,"
            "\"bmsSeen\":%s,\"bmsV\":%u,\"bmsA\":%d,\"bmsSoc\":%u,"
            "\"bmsMinT\":%d,\"bmsMaxT\":%d,"
            "\"freeHeap\":%u,\"safeMode\":%s,\"pinRequired\":%s,"
            "\"timeSynced\":%s,"
            "\"speedD\":%u,\"gearRaw\":%u,\"torqueF\":%d,\"torqueR\":%d,"
            "\"adaptLighting\":%s,\"hbForce\":%s,"
            "\"visionLimit\":%u,\"nagLevel\":%u,\"fcw\":%u,\"accState\":%u,\"brake\":%s,"
            "\"sideCol\":%u,\"laneWarn\":%u,\"laneChg\":%u,"
            "\"autosteer\":%s,\"aeb\":%s,\"fcwOn\":%s,"
            "\"apRestart\":%s,\"nagKiller\":%s,"
            "\"perfAccel\":%u,\"perfBrake\":%u,\"perfAccelMs\":%u,\"perfBrakeMs\":%u,\"brakeEntryKph\":%u,"
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
            (int)cfg.precondition,
            (int)cfg.hwDetected,
            (int)cfg.hw3SmartEnable,
            (int)cfg.hw3SmartT1, (int)cfg.hw3SmartT2,
            (int)cfg.hw3SmartO1, (int)cfg.hw3SmartO2, (int)cfg.hw3SmartO3,
            (unsigned)cfg.fusedSpeedLimit, (unsigned)cfg.hw3SmartActiveTier, (unsigned)cfg.hw3SmartLastKmh,
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
            cfg.nagKiller   ? "true" : "false",
            (unsigned)cfg.perfAccelState, (unsigned)cfg.perfBrakeState,
            (unsigned)cfg.perfAccelMs,    (unsigned)cfg.perfBrakeMs,
            (unsigned)cfg.perfBrakeEntryKph,
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
            if ((v == -1 || (v >= 0 && v <= 100)) && v != cfg.hw3OffsetManual) {
                cfg.hw3OffsetManual = v; changed = true;
            }
        }
        if (req->hasParam("hw3Smart")) {
            bool v = req->getParam("hw3Smart")->value().toInt() != 0;
            if (v != cfg.hw3SmartEnable) { cfg.hw3SmartEnable = v; changed = true; }
        }
        if (req->hasParam("hw3SmT1")) {
            uint8_t v = (uint8_t)constrain(req->getParam("hw3SmT1")->value().toInt(), 20, 180);
            if (v != cfg.hw3SmartT1) { cfg.hw3SmartT1 = v; changed = true; }
        }
        if (req->hasParam("hw3SmT2")) {
            uint8_t v = (uint8_t)constrain(req->getParam("hw3SmT2")->value().toInt(), 20, 200);
            if (v != cfg.hw3SmartT2) { cfg.hw3SmartT2 = v; changed = true; }
        }
        if (req->hasParam("hw3SmO1")) {
            uint8_t v = (uint8_t)constrain(req->getParam("hw3SmO1")->value().toInt(), 0, 20);
            if (v != cfg.hw3SmartO1) { cfg.hw3SmartO1 = v; changed = true; }
        }
        if (req->hasParam("hw3SmO2")) {
            uint8_t v = (uint8_t)constrain(req->getParam("hw3SmO2")->value().toInt(), 0, 20);
            if (v != cfg.hw3SmartO2) { cfg.hw3SmartO2 = v; changed = true; }
        }
        if (req->hasParam("hw3SmO3")) {
            uint8_t v = (uint8_t)constrain(req->getParam("hw3SmO3")->value().toInt(), 0, 20);
            if (v != cfg.hw3SmartO3) { cfg.hw3SmartO3 = v; changed = true; }
        }
        // precond removed from UI — requires Vehicle CAN (X179 pin 9/10)
        if (req->hasParam("nagKiller")) {
            bool v = req->getParam("nagKiller")->value().toInt() != 0;
            if (v != cfg.nagKiller) { cfg.nagKiller = v; changed = true; }
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
    // Uses async scan + polling to avoid blocking the WiFi driver in AP+STA mode.
    server.on("/api/scan", HTTP_GET, [](AsyncWebServerRequest* req) {
        if (!checkToken(req)) { req->send(403, "text/plain", "UNAUTH"); return; }
        WiFi.scanDelete();
        WiFi.scanNetworks(/*async=*/true, /*hidden=*/false);
        // Poll up to 6 s for results (channel hopping takes ~100 ms/channel)
        uint32_t t0 = millis();
        int n = WIFI_SCAN_RUNNING;
        while (n == WIFI_SCAN_RUNNING && millis() - t0 < 6000) {
            delay(100);
            n = WiFi.scanComplete();
        }
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
        if (spiffsOK) SPIFFS.remove(SPIFFS_LOG_PATH);
        req->send(200, "text/plain", "OK");
    });

    // Download full persistent log from SPIFFS as a text attachment.
    server.on("/api/log/download", HTTP_GET, [](AsyncWebServerRequest* req) {
        if (!checkToken(req)) { req->send(403, "text/plain", "UNAUTH"); return; }
        if (!spiffsOK || !SPIFFS.exists(SPIFFS_LOG_PATH)) {
            req->send(404, "text/plain", "No persistent log");
            return;
        }
        AsyncWebServerResponse* resp = req->beginResponse(
            SPIFFS, SPIFFS_LOG_PATH, "text/plain");
        resp->addHeader("Content-Disposition",
                        "attachment; filename=\"diag.log\"");
        req->send(resp);
    });

    // /api/highbeam removed — requires Vehicle CAN (X179 pin 9/10), not available on Party CAN

    // AP auto-restart toggle
    server.on("/api/aprestart", HTTP_GET, [](AsyncWebServerRequest* req) {
        if (!checkToken(req)) { req->send(403, "text/plain", "UNAUTH"); return; }
        if (!req->hasParam("en")) { req->send(400, "text/plain", "Missing en"); return; }
        cfg.apRestart = req->getParam("en")->value().toInt() != 0;
        saveConfig();
        req->send(200, "application/json", "{\"ok\":true}");
    });

    // Manual reboot — flag-based (same pattern as OTA restart)
    server.on("/api/reboot", HTTP_GET, [](AsyncWebServerRequest* req) {
        if (!checkToken(req)) { req->send(403, "text/plain", "UNAUTH"); return; }
        req->send(200, "application/json", "{\"ok\":true}");
        otaPendingRestart = true;
    });

    server.on("/api/reset", HTTP_POST, [](AsyncWebServerRequest* req) {
        if (!checkToken(req)) { req->send(403, "text/plain", "UNAUTH"); return; }
        // Clear all namespaces
        prefs.begin("fsd", false);
        prefs.clear();
        prefs.end();
        Preferences secPrefs;
        secPrefs.begin("sec", false);
        secPrefs.clear();
        secPrefs.end();
        Preferences sysPrefs;
        sysPrefs.begin("sys", false);
        sysPrefs.clear();
        sysPrefs.end();
        // Reload defaults into cfg and restart
        req->send(200, "application/json", "{\"ok\":true}");
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
    uint32_t precondLastMs = 0;  // last precondition frame sent (millis)
    uint32_t normalStartMs = millis();  // boot time; clears crash counter after 10s
    bool     crashCleared  = false;    // true once crash counter has been cleared
    uint32_t logLastMs     = 0;  // last 1 Hz log check (millis)
    uint8_t  prevAccState = 0;   // tracks AP state transitions for auto-restart

    // One-shot CAN ID frequency scan — collects for 30s then logs to diag
    struct IdEntry { uint32_t id; uint32_t cnt; };
    static IdEntry idScan[256];  // static: avoids 2KB stack usage
    memset(idScan, 0, sizeof(idScan));
    uint8_t  idScanN    = 0;
    bool     idScanDone = false;

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
                if (cfg.canOK) {
                    // Transition into bus-off: clear stale gesture state so
                    // spurious noise frames during recovery can't complete a
                    // partially-accumulated HB tap sequence.
                    resetStalkGestureState();
                    // Log TEC/REC counters to help diagnose who is causing errors.
                    // TEC high → ESP32 is transmitting bad frames (firmware/wiring issue on TX)
                    // REC high → ESP32 is receiving error frames (car-side or CAN-H/L wiring fault)
                    uint32_t up = (millis() - cfg.uptimeStart) / 1000;
                    char errmsg[64];
                    snprintf(errmsg, sizeof(errmsg),
                             "bus-off TEC=%u REC=%u tx_err=%u rx_err=%u",
                             (unsigned)twaiStatus.tx_error_counter,
                             (unsigned)twaiStatus.rx_error_counter,
                             (unsigned)twaiStatus.tx_failed_count,
                             (unsigned)twaiStatus.rx_missed_count);
                    addDiagLog(up, errmsg);
                }
                cfg.canOK = false;
                // Recovery handled internally by driver's read()/send()
                // via recoverWithCooldown() — no external trigger needed.
            } else if (!cfg.canOK && canDriver.isDriverOK()) {
                // Driver recovered from bus-off but no frame has arrived yet.
                // Sync canOK so the UI reflects the restored state immediately
                // rather than waiting for the first successful frame read.
                cfg.canOK = true;
            }
        }

        // ── Normal frame processing ────────────────────────────────
        bool activity = false;
        while (canDriver.read(frame)) {
            cfg.canOK = true;
            activity = true;
            handleMessage(frame, canDriver);
            updatePerfTest(telemSpeedRaw(), telemTorqueRear(), telemBrake());
            // ID scan: track unique IDs for first 30s
            if (!idScanDone) {
                bool found = false;
                for (uint8_t i = 0; i < idScanN; i++) {
                    if (idScan[i].id == frame.id) { idScan[i].cnt++; found = true; break; }
                }
                if (!found && idScanN < 256) { idScan[idScanN].id = frame.id; idScan[idScanN].cnt = 1; idScanN++; }
            }
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

        // ── Precondition frame every 500 ms when enabled ───────────
        if (cfg.precondition && cfg.canOK) {
            uint32_t now = millis();
            if (now - precondLastMs >= 500) {
                precondLastMs = now;
                CanFrame pcFrame;
                buildPreconditionFrame(pcFrame);
                canDriver.send(pcFrame);
            }
        } else {
            precondLastMs = 0;
        }


        // ── Clear crash counter after 10s of normal operation ──────
        if (!crashCleared && millis() - normalStartMs >= 10000) {
            crashCleared = true;
            prefs.begin("sys", false);
            prefs.putInt("crashes", 0);
            prefs.end();
        }

        // ── CAN ID scan: log all IDs at 30s sorted by ID number ──────
        if (!idScanDone && millis() - normalStartMs >= 30000) {
            idScanDone = true;
            // Sort by ID ascending (selection sort, small N)
            for (uint8_t i = 0; i < idScanN - 1; i++)
                for (uint8_t j = i + 1; j < idScanN; j++)
                    if (idScan[j].id < idScan[i].id) {
                        IdEntry t = idScan[i]; idScan[i] = idScan[j]; idScan[j] = t;
                    }
            uint32_t up = (millis() - cfg.uptimeStart) / 1000;
            char hdr[40];
            snprintf(hdr, sizeof(hdr), "=== CAN IDs (%u unique, 30s) ===", idScanN);
            addDiagLog(up, hdr);
            char buf[80];
            for (uint8_t i = 0; i < idScanN; i += 6) {
                int p = 0;
                for (uint8_t j = i; j < i + 6 && j < idScanN; j++)
                    p += snprintf(buf + p, sizeof(buf) - p, "%03X:%u ", idScan[j].id, idScan[j].cnt);
                addDiagLog(up, buf);
            }
        }

        // ── Diagnostic log — 1 Hz event transition check ──────────
        uint32_t nowMs = millis();
        if (nowMs - logLastMs >= 1000) {
            logLastMs = nowMs;
            uint32_t up = (nowMs - cfg.uptimeStart) / 1000;
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

    // Init SPIFFS — must come before boot log entry so the marker lands in the file
    setupSpiffs(FIRMWARE_VERSION);

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

    // Connect to router first (if configured) so we can read the assigned STA IP
    // and pick a non-conflicting AP subnet
    if (staSSID[0] != '\0') {
        // Bring up AP on default 192.168.4.1 while STA connects
        WiFi.softAPConfig(IPAddress(192,168,4,1), IPAddress(192,168,4,1), IPAddress(255,255,255,0));
        WiFi.softAP(apSSID, apPass[0] ? apPass : nullptr);
        WiFi.begin(staSSID, staPass[0] ? staPass : nullptr);
        Serial.printf("WiFi STA: connecting to '%s'...\n", staSSID);
        uint32_t t0 = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - t0 < 10000) {
            delay(200);
        }
        staConnected = (WiFi.status() == WL_CONNECTED);
        if (staConnected) {
            // Keep AP on 192.168.4.1; only move if router is on the same subnet.
            uint8_t routerOctet = WiFi.localIP()[2];
            uint8_t apOctet = 4;
            if (apOctet == routerOctet) apOctet = 99;
            if (apOctet == routerOctet) apOctet = 98;
            IPAddress apIP(192, 168, apOctet, 1);
            WiFi.softAPConfig(apIP, apIP, IPAddress(255,255,255,0));
            WiFi.softAP(apSSID, apPass[0] ? apPass : nullptr);
            char logMsg[64];
            snprintf(logMsg, sizeof(logMsg), "STA connected %s  AP: 192.168.%u.1",
                     WiFi.localIP().toString().c_str(), (unsigned)apOctet);
            Serial.println(logMsg);
            addDiagLog(0, logMsg);
        } else {
            Serial.printf("WiFi STA connect failed\n");
            addDiagLog(0, "STA connect FAILED");
        }
    } else {
        // No router configured — use default ESP32 AP address
        WiFi.softAPConfig(IPAddress(192,168,4,1), IPAddress(192,168,4,1), IPAddress(255,255,255,0));
        WiFi.softAP(apSSID, apPass[0] ? apPass : nullptr);
    }
    // Captive portal DNS — redirect all domains to ESP32 AP IP.
    // Causes iOS/Android to detect a captive portal and pop up the browser automatically.
    dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
    dnsServer.start(53, "*", WiFi.softAPIP());
    Serial.printf("WiFi AP: %s  IP: %s\n", apSSID, WiFi.softAPIP().toString().c_str());

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

    // Captive portal — must be called frequently to answer DNS queries promptly
    dnsServer.processNextRequest();

    // Flush new RAM log entries to SPIFFS every 3 s (Core 0 only).
    static uint32_t lastFlushMs = 0;
    uint32_t nowMs = millis();
    if (nowMs - lastFlushMs >= 3000) {
        lastFlushMs = nowMs;
        flushLogsToSpiffs();
    }

    // Heap guard: AsyncWebServer heap fragmentation causes silent request
    // failures after long uptime. Auto-restart when free heap < 20 KB.
    static uint32_t lastHeapMs  = 0;
    static bool     heapWarnSent = false;
    if (nowMs - lastHeapMs >= 5000) {
        lastHeapMs = nowMs;
        uint32_t freeHeap = ESP.getFreeHeap();
        if (freeHeap < 20000 && !heapWarnSent) {
            uint32_t up = (nowMs - cfg.uptimeStart) / 1000;
            char msg[48];
            snprintf(msg, sizeof(msg), "low heap %u B", (unsigned)freeHeap);
            addDiagLog(up, msg);
            heapWarnSent = true;
        } else if (freeHeap >= 20000) {
            heapWarnSent = false;  // reset so it can warn again if heap drops again
        }
    }

    vTaskDelay(10);
}
