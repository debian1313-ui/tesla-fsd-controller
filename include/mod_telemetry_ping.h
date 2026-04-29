#pragma once
// ── Module: Anonymous usage ping ──────────────────────────────────────────
// Once per 24h, sends {id, version, env} to a Cloudflare Worker. The id is
// sha256(MAC)[0..8] hex (64 bits) — not reversible to MAC. No IP is stored
// server-side (the Worker drops it). Purpose: give the project owner a rough
// count of active installations and version/variant distribution.
//
// Opt-out: NVS key `ub_ping` (default true). User toggles via Wi-Fi bridge UI.
//
// WIFI_BRIDGE_ENABLED only — the ping needs STA connectivity that only the
// Wi-Fi bridge env has reliably.

#ifdef WIFI_BRIDGE_ENABLED

#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>
#include "version.h"
#include "fsd_config.h"
#include "mod_ota.h"  // reuses FIRMWARE_ENV_TAG + OTA_CRT_BUNDLE_ATTACH

extern FSDConfig cfg;

// Override via -D TESLA_COUNTER_ENDPOINT=\"https://...\" if deploying to a
// different subdomain. Default is a custom domain to avoid GFW-style blocks
// on *.workers.dev that prevent mainland-China users from pinging.
#ifndef TESLA_COUNTER_ENDPOINT
#  define TESLA_COUNTER_ENDPOINT "https://counter.wjsall.uk"
#endif

namespace ping_impl {

extern "C" {
    #include "mbedtls/sha256.h"
}

// mod_ota.h already pulled esp_http_client into ota_impl (include-guarded),
// so re-including here would be a no-op. Importing the whole namespace keeps
// enum values (HTTP_METHOD_POST, HTTP_TRANSPORT_OVER_SSL) and the
// esp_crt_bundle_attach symbol available for the OTA_CRT_BUNDLE_ATTACH macro.
using namespace ota_impl;

struct PingState {
    bool     enabled;
    uint32_t lastAttemptMs;
    uint32_t lastSuccessMs;
    char     lastMsg[48];
    char     deviceId[17];  // 16 hex chars + NUL
};

static PingState gPing = { true, 0, 0, "", "" };

// Derive a stable anonymous id: first 8 bytes of sha256(MAC) as hex.
static void computeDeviceId() {
    if (gPing.deviceId[0]) return;  // already computed
    uint8_t mac[6] = {0};
    WiFi.macAddress(mac);
    uint8_t digest[32];
    mbedtls_sha256_context ctx;
    mbedtls_sha256_init(&ctx);
    mbedtls_sha256_starts(&ctx, 0);
    mbedtls_sha256_update(&ctx, mac, 6);
    mbedtls_sha256_finish(&ctx, digest);
    mbedtls_sha256_free(&ctx);
    for (int i = 0; i < 8; ++i) {
        snprintf(&gPing.deviceId[i * 2], 3, "%02x", digest[i]);
    }
    gPing.deviceId[16] = '\0';
}

// Send one ping POST. Blocks up to ~10s. Returns true on HTTP 2xx.
static bool doPing() {
    if (WiFi.status() != WL_CONNECTED) {
        strlcpy(gPing.lastMsg, "Wi-Fi 未连", sizeof(gPing.lastMsg));
        return false;
    }
    if (esp_get_free_heap_size() < 60 * 1024) {
        strlcpy(gPing.lastMsg, "内存不足", sizeof(gPing.lastMsg));
        return false;
    }
    computeDeviceId();

    // carSwVer is user-entered; sanitizer in /api/carver already rejects
    // controls. Whitelist before sending to keep ping body strictly safe and
    // match worker.js carVer regex.
    char escCarV[33] = {0};
    {
        size_t out = 0;
        for (size_t i = 0; cfg.carSwVer[i] && out + 1 < sizeof(escCarV); i++) {
            char c = cfg.carSwVer[i];
            bool ok = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')
                   || (c >= '0' && c <= '9') || c == '.' || c == '_' || c == '-';
            if (ok) escCarV[out++] = c;
        }
        escCarV[out] = 0;
    }

    char body[256];
    int n = snprintf(body, sizeof(body),
        "{\"id\":\"%s\",\"version\":\"%s\",\"env\":\"%s\",\"carVer\":\"%s\"}",
        gPing.deviceId, FIRMWARE_VERSION, FIRMWARE_ENV_TAG, escCarV);
    if (n <= 0 || n >= (int)sizeof(body)) {
        strlcpy(gPing.lastMsg, "body 太大", sizeof(gPing.lastMsg));
        return false;
    }

    esp_http_client_config_t cfg = {};
    cfg.url                   = TESLA_COUNTER_ENDPOINT "/ping";
    cfg.timeout_ms            = 10000;
    cfg.transport_type        = HTTP_TRANSPORT_OVER_SSL;
    cfg.crt_bundle_attach     = OTA_CRT_BUNDLE_ATTACH;
    cfg.disable_auto_redirect = false;
    cfg.max_redirection_count = 2;
    cfg.buffer_size           = 1024;
    cfg.buffer_size_tx        = 512;

    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    if (!client) { strlcpy(gPing.lastMsg, "init 失败", sizeof(gPing.lastMsg)); return false; }
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client, "User-Agent", "tesla-can-web-2/" FIRMWARE_VERSION);
    esp_http_client_set_post_field(client, body, n);

    esp_err_t err = esp_http_client_perform(client);
    int status = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);

    if (err != ESP_OK) {
        snprintf(gPing.lastMsg, sizeof(gPing.lastMsg), "网络错误 %d", (int)err);
        return false;
    }
    if (status < 200 || status >= 300) {
        snprintf(gPing.lastMsg, sizeof(gPing.lastMsg), "HTTP %d", status);
        return false;
    }
    snprintf(gPing.lastMsg, sizeof(gPing.lastMsg), "已上报 (v%s)", FIRMWARE_VERSION);
    return true;
}

struct PingTaskArgs { uint8_t dummy; };

static void pingTaskEntry(void* arg) {
    delete (PingTaskArgs*)arg;
    bool ok = doPing();
    gPing.lastAttemptMs = millis();
    if (ok) gPing.lastSuccessMs = gPing.lastAttemptMs;
    vTaskDelete(nullptr);
}

}  // namespace ping_impl

// ── Public API ────────────────────────────────────────────────────────────

static inline void pingLoadEnabled(Preferences& p) {
    ping_impl::gPing.enabled = p.getBool("ub_ping", true);
}
static inline void pingSaveEnabled(Preferences& p) {
    p.putBool("ub_ping", ping_impl::gPing.enabled);
}
static inline bool pingIsEnabled()   { return ping_impl::gPing.enabled; }
static inline void pingSetEnabled(bool v) { ping_impl::gPing.enabled = v; }
static inline const char* pingDeviceId() {
    ping_impl::computeDeviceId();
    return ping_impl::gPing.deviceId;
}
static inline const char* pingLastMsg()      { return ping_impl::gPing.lastMsg; }
static inline uint32_t pingLastSuccessMs()   { return ping_impl::gPing.lastSuccessMs; }

// Kick off an async ping if enabled + STA connected + debounce satisfied.
// Call from main loop at low cadence (e.g., every 10s). No-ops most of the time.
static inline void pingService() {
    if (!ping_impl::gPing.enabled) return;
    if (WiFi.status() != WL_CONNECTED) return;
    if (otaIsActive()) return;  // don't compete with OTA for heap/network

    uint32_t now = millis();
    bool first = (ping_impl::gPing.lastAttemptMs == 0);
    uint32_t sinceAttempt = now - ping_impl::gPing.lastAttemptMs;
    uint32_t interval = (ping_impl::gPing.lastSuccessMs == 0)
        ? 5 * 60 * 1000        // keep retrying every 5 min until first success
        : 24 * 60 * 60 * 1000; // then once per day
    if (!first && sinceAttempt < interval) return;

    // Grace period after boot to let Wi-Fi stabilize + avoid racing boot OTAs.
    if (first && now < 60 * 1000) return;

    ping_impl::gPing.lastAttemptMs = now;  // debounce now; task will update success
    auto* a = new ping_impl::PingTaskArgs{0};
    BaseType_t r = xTaskCreatePinnedToCore(
        ping_impl::pingTaskEntry, "ping", 8192, a, 1, nullptr, 0);
    if (r != pdPASS) { delete a; }
}

#endif // WIFI_BRIDGE_ENABLED
