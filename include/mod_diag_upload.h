#pragma once
// ── Module: Diagnostic bundle upload ─────────────────────────────────────────
// User clicks "上传诊断包" → ESP32 builds a JSON snapshot of cfg + recent
// CAN fingerprint (0x398 GTW_carConfig + seen-IDs bitmap) + in-memory log,
// POSTs to the project's Cloudflare Worker, receives a 6-char short ID.
//
// User pastes the ID in a GitHub issue. The Worker only returns payload
// content to a request bearing the maintainer's secret — paste-the-ID-publicly
// is safe.
//
// WIFI_BRIDGE_ENABLED only — needs STA connectivity. Reuses ping_impl
// device-id derivation and OTA's TLS bundle.

#ifdef WIFI_BRIDGE_ENABLED

#include <Arduino.h>
#include <WiFi.h>
#include <SPIFFS.h>
#include "version.h"
#include "fsd_config.h"
#include "mod_ota.h"             // FIRMWARE_ENV_TAG, TLS bundle
#include "mod_telemetry_ping.h"  // ping_impl::computeDeviceId / deviceId
#include "mod_log.h"
#include "mod_diag_collect.h"
#include "mod_telemetry.h"  // telemSpeedRaw / telemGear accessors

// Path must match SPIFFS_LOG_PATH in main.cpp. Hard-coded here to avoid an
// extern dance — the path is stable and only used in two places.
#ifndef DIAG_SPIFFS_LOG_PATH
#  define DIAG_SPIFFS_LOG_PATH "/diag.log"
#endif

#ifndef TESLA_DIAG_ENDPOINT
#  define TESLA_DIAG_ENDPOINT TESLA_COUNTER_ENDPOINT  // same Worker, /diag path
#endif

extern FSDConfig cfg;        // defined in handlers.h

namespace diag_impl {

using namespace ota_impl;  // esp_http_client_*, HTTP_METHOD_POST, etc.

// ── Sizing / threshold constants ───────────────────────────────────────────
// Server caps `DIAG_MAX_BYTES = 65536` in worker.js — payload sizing here
// stays well below that. Adjust the trio together if log volume changes.
constexpr size_t kDiagPayloadCap   = 24 * 1024;  // heap buf for serialized JSON
constexpr size_t kDiagLogTailBytes = 12 * 1024;  // /diag.log tail embedded into payload
constexpr uint8_t kDiagTaskPrio    = 1;
constexpr size_t kDiagTaskStack    = 8 * 1024;
// Refuse upload when the largest contiguous free block can't hold the payload
// buffer plus a small slack for IDF + TLS allocations. We check largest block
// (not total free heap) because heap fragmentation, not total bytes, is what
// actually fails malloc on long-running ESP32 firmware.
constexpr size_t kDiagMinLargestBlock = kDiagPayloadCap + 4 * 1024;

enum DiagState : uint8_t {
    DIAG_IDLE      = 0,
    DIAG_UPLOADING = 1,
    DIAG_DONE      = 2,
    DIAG_ERROR     = 3,
};

struct DiagUploadState {
    volatile uint8_t state;
    char     shortId[8];   // 6-char ID + NUL
    char     msg[64];
};

// Project is single-TU (handlers.h included only by main.cpp). If that ever
// changes, switch this to inline (C++17) or define in a .cpp.
static DiagUploadState gDiagUp = { DIAG_IDLE, "", "" };

// Append helper — bumps `*pos` by snprintf return, never overruns `cap`.
// Returns true while buffer still has room.
static inline bool dappend(char* buf, size_t cap, size_t* pos, const char* fmt, ...) {
    if (*pos >= cap) return false;
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf + *pos, cap - *pos, fmt, ap);
    va_end(ap);
    if (n < 0) return false;
    *pos += (size_t)n;
    return *pos < cap;
}

// Build the diagnostic JSON payload into a heap buffer. Caller frees.
// Returns nullptr on alloc / format failure.
static char* buildPayload(size_t* outLen) {
    constexpr size_t CAP = kDiagPayloadCap;
    char* buf = (char*)malloc(CAP);
    if (!buf) return nullptr;
    size_t pos = 0;

    ping_impl::computeDeviceId();
    const char* devId = ping_impl::gPing.deviceId;

    uint32_t uptime = (millis() - cfg.uptimeStart) / 1000;

    dappend(buf, CAP, &pos,
        "{\"id\":\"%s\",\"version\":\"%s\",\"env\":\"%s\","
        "\"uptime\":%u,\"freeHeap\":%u,",
        devId, FIRMWARE_VERSION, FIRMWARE_ENV_TAG,
        (unsigned)uptime, (unsigned)esp_get_free_heap_size());

    dappend(buf, CAP, &pos,
        "\"rx\":%u,\"modified\":%u,\"errors\":%u,\"canOK\":%s,\"fsdTriggered\":%s,",
        (unsigned)cfg.rxCount, (unsigned)cfg.modifiedCount, (unsigned)cfg.errorCount,
        cfg.canOK ? "true" : "false",
        cfg.fsdTriggered ? "true" : "false");

    dappend(buf, CAP, &pos,
        "\"hwMode\":%d,\"hwDetected\":%d,\"gwAutopilot\":%d,\"speedProfile\":%d,"
        "\"profileModeAuto\":%d,",
        (int)cfg.hwMode, (int)cfg.hwDetected, (int)cfg.gatewayAutopilot, (int)cfg.speedProfile,
        (int)cfg.profileModeAuto);

    dappend(buf, CAP, &pos,
        "\"fsdEnable\":%d,\"forceActivate\":%d,\"tlsscBypass\":%d,\"isaChime\":%d,"
        "\"emergencyDet\":%d,\"overrideSL\":%d,\"removeVSL\":%d,\"isaOverride\":%d,"
        "\"ipBlockerEnabled\":%d,",
        (int)cfg.fsdEnable, (int)cfg.forceActivate, (int)cfg.tlsscBypass,
        (int)cfg.isaChimeSuppress, (int)cfg.emergencyDetection,
        (int)cfg.overrideSpeedLimit, (int)cfg.removeVisionSpeedLimit,
        (int)cfg.isaOverride, (int)cfg.ipBlockerEnabled);

    dappend(buf, CAP, &pos,
        "\"hw3AutoSpeed\":%d,\"hw3CustomSpeed\":%d,\"hw3WireEncoding\":%u,"
        "\"hw3OffsetSlew\":%d,\"hw3SlewRate\":%u,\"hw3HighSpeedEnable\":%d,"
        "\"hw4Offset\":%u,\"legacyOffset\":%u,",
        (int)cfg.hw3AutoSpeed, (int)cfg.hw3CustomSpeed,
        (unsigned)cfg.hw3WireEncoding, (int)cfg.hw3OffsetSlew,
        (unsigned)cfg.hw3SlewRatePctPerSec, (int)cfg.hw3HighSpeedEnable,
        (unsigned)cfg.hw4OffsetRaw, (unsigned)cfg.legacyOffset);

    // Live HW3 offset state — what the encoder *targeted* vs what we *sent*
    // last, plus how often slew clamped a drop. Critical for diagnosing
    // "offset feels wrong" reports (slew capping vs encoding mismatch).
    dappend(buf, CAP, &pos,
        "\"hw3StockOff\":%d,\"hw3LastRaw\":%u,\"hw3TargetRaw\":%u,"
        "\"hw3SlewCount\":%u,\"hw3LastSentMs\":%u,",
        (int)cfg.hw3SpeedOffset,
        (unsigned)cfg.hw3OffsetLastRaw, (unsigned)cfg.hw3OffsetTargetRaw,
        (unsigned)cfg.hw3OffsetSlewCount, (unsigned)cfg.hw3OffsetLastSentMs);

    // HW3 custom-target & high-speed tables. Without these we can't reproduce
    // a user's configured curve from a diag bundle. Compact JSON arrays.
    dappend(buf, CAP, &pos, "\"hw3CustomTarget\":[");
    for (size_t i = 0; i < sizeof(cfg.hw3CustomTarget) / sizeof(cfg.hw3CustomTarget[0]); i++) {
        dappend(buf, CAP, &pos, "%s%u", i ? "," : "", (unsigned)cfg.hw3CustomTarget[i]);
    }
    dappend(buf, CAP, &pos, "],\"hw3HighSpeedTargetPct\":[");
    for (int i = 0; i < kHw3HighSpeedBucketCount; i++) {
        dappend(buf, CAP, &pos, "%s%u", i ? "," : "", (unsigned)cfg.hw3HighSpeedTargetPct[i]);
    }
    dappend(buf, CAP, &pos, "],");

    // Legacy speed-limit sniff (0x2F8 UI_gpsVehicleSpeed). gpsSpeedSeen=false
    // → Legacy isn't the active path; kept anyway for cross-variant triage.
    dappend(buf, CAP, &pos,
        "\"gpsSeen\":%s,\"gpsUserOff\":%u,\"gpsMppLim\":%u,\"gpsCount\":%u,\"gpsPeriodMs\":%u,",
        cfg.gpsSpeedSeen ? "true" : "false",
        (unsigned)cfg.gpsUserOffsetRaw, (unsigned)cfg.gpsMppLimitRaw,
        (unsigned)cfg.gpsSpeedCount, (unsigned)cfg.gpsSpeedPeriodMs);

    // DAS state bits parsed from 0x293 / 0x39B — whether FSD is engaged at
    // upload time. Speed-limit injection only fires when FSD is active.
    dappend(buf, CAP, &pos,
        "\"autosteerOn\":%d,\"aebOn\":%d,\"fcwOn\":%d,\"fcwLevel\":%u,"
        "\"sideColl\":%u,\"laneDept\":%u,\"laneChg\":%u,\"brake\":%d,",
        (int)cfg.autosteerOn, (int)cfg.aebOn, (int)cfg.fcwOn, (unsigned)cfg.fcwLevel,
        (unsigned)cfg.sideCollision, (unsigned)cfg.laneDeptWarning,
        (unsigned)cfg.laneChangeState, (int)telemBrake());

    dappend(buf, CAP, &pos,
        "\"fusedLimit\":%u,\"visionLimit\":%u,\"nagLevel\":%u,\"accState\":%u,"
        "\"speedKph10\":%u,\"gearRaw\":%u,",
        (unsigned)cfg.fusedSpeedLimit, (unsigned)cfg.visionSpeedLimit,
        (unsigned)cfg.nagLevel, (unsigned)cfg.accState,
        (unsigned)telemSpeedRaw(), (unsigned)telemGear());

    dappend(buf, CAP, &pos,
        "\"bmsSeen\":%s,\"bmsV\":%u,\"bmsA\":%d,\"bmsSoc\":%u,\"bmsMinT\":%d,\"bmsMaxT\":%d,"
        "\"tempInRaw\":%u,\"tempOutRaw\":%u,",
        cfg.bmsSeen ? "true" : "false",
        (unsigned)cfg.packVoltage_cV, (int)cfg.packCurrent_dA, (unsigned)cfg.socPercent_d,
        (int)cfg.battTempMin, (int)cfg.battTempMax,
        (unsigned)cfg.tempInsideRaw, (unsigned)cfg.tempOutsideRaw);

    // User-entered Tesla MCU version — sanitizer already rejects control bytes,
    // but JSON-escape " and \ defensively. Same helper as /api/status.
    char escCarSw[sizeof(cfg.carSwVer) * 2 + 2];
    jsonEscapeInto(cfg.carSwVer, escCarSw, sizeof(escCarSw));
    dappend(buf, CAP, &pos, "\"carSwVer\":\"%s\",", escCarSw);

    // GTW_carConfig 0x398 raw bytes — server decodes signals.
    dappend(buf, CAP, &pos, "\"carConfig\":{\"seen\":%s,\"bytes\":[",
            gDiagCarConfig.seen ? "true" : "false");
    for (int i = 0; i < 8; i++) {
        dappend(buf, CAP, &pos, "%s%u", i ? "," : "", (unsigned)gDiagCarConfig.bytes[i]);
    }
    dappend(buf, CAP, &pos, "]},");

    // Raw last-frame cache for speed-critical IDs. Lets us decode any
    // not-yet-parsed bit (e.g. new ISA mux) post-hoc on the server.
    dappend(buf, CAP, &pos, "\"speedFrames\":{");
    for (uint8_t i = 0; i < DIAG_SPEED_ID_COUNT; i++) {
        const DiagFrameSnap& s = gDiagSpeedFrames[i];
        dappend(buf, CAP, &pos, "%s\"%u\":{\"seen\":%s,\"dlc\":%u,\"b\":[",
            i ? "," : "", (unsigned)DIAG_SPEED_IDS[i],
            s.seen ? "true" : "false", (unsigned)s.dlc);
        for (uint8_t j = 0; j < 8; j++) {
            dappend(buf, CAP, &pos, "%s%u", j ? "," : "", (unsigned)s.bytes[j]);
        }
        dappend(buf, CAP, &pos, "]}");
    }
    dappend(buf, CAP, &pos, "},");

    // seen-IDs bitmap as hex string (256 bytes → 512 hex chars).
    dappend(buf, CAP, &pos, "\"seenIds\":\"");
    for (uint16_t i = 0; i < DIAG_SEEN_BYTES && pos + 3 < CAP; i++) {
        dappend(buf, CAP, &pos, "%02x", (unsigned)gDiagSeenIds[i]);
    }
    dappend(buf, CAP, &pos, "\",");

    // ── Persistent log tail ──────────────────────────────────────────────
    // /diag.log is the only signal that survives "drove at noon, uploaded at
    // 10pm". Read the tail (last ~12 KB, ~120 events) and embed as a single
    // JSON string so the server can decode any speed-limit transition in time.
    // Falls back to the in-RAM ring if SPIFFS is unavailable.
    bool wroteLog = false;
    if (SPIFFS.exists(DIAG_SPIFFS_LOG_PATH)) {
        File f = SPIFFS.open(DIAG_SPIFFS_LOG_PATH, "r");
        if (f) {
            size_t sz = f.size();
            size_t skip = sz > kDiagLogTailBytes ? sz - kDiagLogTailBytes : 0;
            f.seek(skip);
            // Skip leading partial line so the embedded log starts cleanly.
            if (skip > 0) {
                while (f.available()) { int c = f.read(); if (c < 0 || c == '\n') break; }
            }
            dappend(buf, CAP, &pos, "\"log\":\"");
            // 256-byte read buffer caps stack pressure on the upload task.
            uint8_t chunk[256];
            while (f.available() && pos + 8 < CAP) {
                int n = f.read(chunk, sizeof(chunk));
                if (n <= 0) break;
                for (int j = 0; j < n && pos + 6 < CAP; j++) {
                    char c = (char)chunk[j];
                    if (c == '"' || c == '\\') { buf[pos++] = '\\'; buf[pos++] = c; }
                    else if (c == '\n')        { buf[pos++] = '\\'; buf[pos++] = 'n'; }
                    else if (c == '\r')        { /* drop */ }
                    else if ((uint8_t)c < 0x20) { buf[pos++] = ' '; }
                    else                       { buf[pos++] = c; }
                }
            }
            f.close();
            dappend(buf, CAP, &pos, "\"}");
            wroteLog = true;
        }
    }
    if (!wroteLog) {
        dappend(buf, CAP, &pos, "\"logRam\":[");
        uint16_t cnt = diagLogCount();
        for (uint16_t i = 0; i < cnt; i++) {
            if (pos + 8 >= CAP) break;
            if (i > 0) buf[pos++] = ',';
            buf[pos++] = '"';
            const char* line = diagLogAt(i);
            for (size_t j = 0; line[j] && pos + 2 < CAP; j++) {
                unsigned char c = (unsigned char)line[j];
                if (c == '"' || c == '\\') buf[pos++] = '\\';
                if (c < 0x20) { buf[pos++] = ' '; continue; }
                buf[pos++] = (char)c;
            }
            if (pos + 1 < CAP) buf[pos++] = '"';
        }
        dappend(buf, CAP, &pos, "]}");
    }

    if (pos >= CAP - 1) {
        // Hit cap mid-write — produce no payload rather than malformed JSON.
        free(buf);
        return nullptr;
    }
    buf[pos] = 0;
    *outLen = pos;
    return buf;
}

// Parse {"id":"AB12CD"} from response body — extract value of first "id" key.
// Tolerant to whitespace; rejects values that don't match the worker alphabet.
static bool extractShortId(const char* body, char* out, size_t cap) {
    if (!body || cap < 8) return false;
    const char* p = strstr(body, "\"id\"");
    if (!p) return false;
    p += 4;
    while (*p && *p != ':') p++;
    if (*p != ':') return false;
    p++;
    while (*p == ' ' || *p == '\t') p++;
    if (*p != '"') return false;
    p++;
    size_t n = 0;
    while (*p && *p != '"' && n + 1 < cap) {
        char c = *p++;
        // Match worker.js DIAG_ID_ALPHABET — excludes I and O to avoid the
        // user confusing them with 1/0 when typing the id from a paper note.
        bool ok = ((c >= 'A' && c <= 'H') || (c >= 'J' && c <= 'N')
                || (c >= 'P' && c <= 'Z') || (c >= '2' && c <= '9'));
        if (!ok) return false;
        out[n++] = c;
    }
    out[n] = 0;
    return n >= 4 && n <= 8 && *p == '"';
}

static void uploadTask(void* /*arg*/) {
    char* payload = nullptr;
    size_t payloadLen = 0;

    if (WiFi.status() != WL_CONNECTED) {
        strlcpy(gDiagUp.msg, "Wi-Fi 未连接", sizeof(gDiagUp.msg));
        gDiagUp.state = DIAG_ERROR;
        vTaskDelete(nullptr);
        return;
    }
    // Largest contiguous block (not total free heap) is what malloc actually
    // needs — under fragmentation a 100 KB total can still fail a 24 KB alloc.
    if (heap_caps_get_largest_free_block(MALLOC_CAP_8BIT) < kDiagMinLargestBlock) {
        strlcpy(gDiagUp.msg, "内存不足", sizeof(gDiagUp.msg));
        gDiagUp.state = DIAG_ERROR;
        vTaskDelete(nullptr);
        return;
    }

    payload = buildPayload(&payloadLen);
    if (!payload) {
        strlcpy(gDiagUp.msg, "payload 构建失败", sizeof(gDiagUp.msg));
        gDiagUp.state = DIAG_ERROR;
        vTaskDelete(nullptr);
        return;
    }

    esp_http_client_config_t hcfg = {};
    hcfg.url                   = TESLA_DIAG_ENDPOINT "/diag";
    hcfg.timeout_ms            = 15000;
    hcfg.transport_type        = HTTP_TRANSPORT_OVER_SSL;
    hcfg.crt_bundle_attach     = OTA_CRT_BUNDLE_ATTACH;
    hcfg.disable_auto_redirect = false;
    hcfg.max_redirection_count = 2;
    hcfg.buffer_size           = 4096;  // S3-style redirect headers can be long
    hcfg.buffer_size_tx        = 1024;

    esp_http_client_handle_t client = esp_http_client_init(&hcfg);
    if (!client) {
        free(payload);
        strlcpy(gDiagUp.msg, "HTTP init 失败", sizeof(gDiagUp.msg));
        gDiagUp.state = DIAG_ERROR;
        vTaskDelete(nullptr);
        return;
    }
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client, "User-Agent", "tesla-can-web-2/" FIRMWARE_VERSION);
    esp_http_client_set_post_field(client, payload, (int)payloadLen);

    char respBuf[128] = {0};
    esp_err_t err = esp_http_client_open(client, (int)payloadLen);
    bool ok = false;
    int status = 0;
    if (err == ESP_OK) {
        int wn = esp_http_client_write(client, payload, (int)payloadLen);
        if (wn == (int)payloadLen) {
            esp_http_client_fetch_headers(client);
            status = esp_http_client_get_status_code(client);
            int rn = esp_http_client_read_response(client, respBuf, sizeof(respBuf) - 1);
            if (rn > 0) respBuf[rn] = 0;
            ok = (status >= 200 && status < 300);
        }
    }
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
    free(payload);

    if (!ok) {
        // Map known worker.js status codes to friendly messages. 403 isn't
        // emitted by worker.js itself — it comes from Cloudflare's edge
        // (Bot Fight Mode / managed WAF challenge), typically because the
        // 15 KB diag POST trips a different rule than the 200 B ping POST.
        const char* canned = nullptr;
        switch (status) {
            case 400: canned = "请求格式错误";              break;
            case 403: canned = "服务拒绝（CF 防火墙？）";   break;
            case 413: canned = "数据过大，请重试";          break;
            case 429: canned = "限频中，请稍后再试";        break;
            case 503: canned = "服务繁忙，请重试";          break;
        }
        if (canned)         strlcpy(gDiagUp.msg, canned, sizeof(gDiagUp.msg));
        else if (status > 0) snprintf(gDiagUp.msg, sizeof(gDiagUp.msg), "HTTP %d", status);
        else                 snprintf(gDiagUp.msg, sizeof(gDiagUp.msg), "网络错误 %d", (int)err);
        gDiagUp.state = DIAG_ERROR;
        vTaskDelete(nullptr);
        return;
    }

    if (!extractShortId(respBuf, gDiagUp.shortId, sizeof(gDiagUp.shortId))) {
        strlcpy(gDiagUp.msg, "响应格式错误", sizeof(gDiagUp.msg));
        gDiagUp.state = DIAG_ERROR;
        vTaskDelete(nullptr);
        return;
    }

    snprintf(gDiagUp.msg, sizeof(gDiagUp.msg), "已生成 ID: %s", gDiagUp.shortId);
    gDiagUp.state = DIAG_DONE;
    vTaskDelete(nullptr);
}

}  // namespace diag_impl

// ── Public API ────────────────────────────────────────────────────────────

// Trigger upload. No-op if already uploading. Returns false if currently busy.
static inline bool diagUploadTrigger() {
    using namespace diag_impl;
    if (gDiagUp.state == DIAG_UPLOADING) return false;
    if (WiFi.status() != WL_CONNECTED) {
        strlcpy(gDiagUp.msg, "Wi-Fi 未连接", sizeof(gDiagUp.msg));
        gDiagUp.state = DIAG_ERROR;
        return true;  // state updated, but task not started
    }
    gDiagUp.shortId[0] = 0;
    gDiagUp.msg[0] = 0;
    gDiagUp.state = DIAG_UPLOADING;
    BaseType_t r = xTaskCreatePinnedToCore(
        uploadTask, "diagUp", kDiagTaskStack, nullptr, kDiagTaskPrio, nullptr, 0);
    if (r != pdPASS) {
        strlcpy(gDiagUp.msg, "任务创建失败", sizeof(gDiagUp.msg));
        gDiagUp.state = DIAG_ERROR;
    }
    return true;
}

static inline uint8_t      diagUploadState()   { return diag_impl::gDiagUp.state; }
static inline const char*  diagUploadShortId() { return diag_impl::gDiagUp.shortId; }
static inline const char*  diagUploadMsg()     { return diag_impl::gDiagUp.msg; }

#endif  // WIFI_BRIDGE_ENABLED
