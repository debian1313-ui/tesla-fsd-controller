#pragma once
// ── Module: Online OTA (pull from GitHub Releases) ────────────────────────
// Complements the existing local upload OTA (/api/ota). Exposes:
//   otaStartCheck()   — queries GitHub Releases API for the latest tag +
//                       asset matching this build's FIRMWARE_ENV_TAG.
//   otaStartPull(url) — downloads .bin and writes it via esp_https_ota.
//   otaStatusJson()   — snapshot of progress (state/written/total/msg).
//   otaLatestUrl()    — last resolved .bin URL (empty before check succeeds).
//
// Implementation uses ESP-IDF esp_http_client + esp_https_ota directly so
// it compiles identically for arduino-only AND arduino+espidf envs — the
// Arduino WiFiClientSecure library doesn't link cleanly in hybrid builds.
//
// The IDF `http_parser.h` header defines an enum value `HTTP_GET` that
// clashes with AsyncWebServer's `AsyncWebRequestMethodType::HTTP_GET`.
// We isolate IDF includes inside an `ota_impl` namespace so those enum
// constants don't leak into the global namespace used by main.cpp.

#include <Arduino.h>
#include <WiFi.h>
#include "version.h"

#ifndef OTA_RELEASE_OWNER
#define OTA_RELEASE_OWNER "wjsall"
#endif
#ifndef OTA_RELEASE_REPO
#define OTA_RELEASE_REPO "tesla-fsd-controller"
#endif

// Build-variant tag — must match merge_firmware.py env_name.replace('-','_').
// Asset names look like: esp32_v1.4.15_ota.bin, esp32s3_waveshare_v1.4.15_ota.bin …
#ifndef FIRMWARE_ENV_TAG
#  if defined(WIFI_BRIDGE_ENABLED) && (defined(CONFIG_IDF_TARGET_ESP32S3) || defined(ARDUINO_USB_MODE))
#    define FIRMWARE_ENV_TAG "esp32s3_waveshare_wifi"
#  elif defined(WIFI_BRIDGE_ENABLED)
#    define FIRMWARE_ENV_TAG "esp32_wifi"
#  elif defined(CONFIG_IDF_TARGET_ESP32S3) || defined(ARDUINO_USB_MODE)
#    define FIRMWARE_ENV_TAG "esp32s3_waveshare"
#  else
#    define FIRMWARE_ENV_TAG "esp32"
#  endif
#endif

extern volatile bool otaPendingRestart;  // defined in main.cpp

namespace ota_impl {

extern "C" {
    #include "esp_http_client.h"
    #include "esp_https_ota.h"
    #include "esp_crt_bundle.h"
    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"
}

// arduino-esp32 renames the bundle attach symbol; hybrid arduino+espidf
// builds use the stock IDF name. Both take (esp_http_client_config_t*).
#if defined(WIFI_BRIDGE_ENABLED)
#define OTA_CRT_BUNDLE_ATTACH esp_crt_bundle_attach
#else
#define OTA_CRT_BUNDLE_ATTACH arduino_esp_crt_bundle_attach
#endif

enum OtaState : uint8_t {
    OTA_IDLE        = 0,
    OTA_CHECKING    = 1,
    OTA_DOWNLOADING = 2,
    OTA_WRITING     = 3,
    OTA_READY       = 4,
    OTA_SUCCESS     = 5,
    OTA_ERROR       = 6,
};

struct OtaStatus {
    volatile uint8_t  state;
    volatile uint32_t total;
    volatile uint32_t written;
    char message[128];
    char latestVersion[24];
    char latestUrl[256];
};

static OtaStatus gOta = {};
static String    gOtaBody;

static void otaSetMsg(uint8_t state, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(gOta.message, sizeof(gOta.message), fmt, ap);
    va_end(ap);
    gOta.state = state;
}

static esp_err_t otaHttpEvent(esp_http_client_event_t* evt) {
    if (evt->event_id == HTTP_EVENT_ON_DATA && evt->data && evt->data_len > 0) {
        if (gOtaBody.length() < 32768) {
            gOtaBody.concat((const char*)evt->data, evt->data_len);
        }
    }
    return ESP_OK;
}

static bool otaParseLatest(const String& body) {
    int tagIdx = body.indexOf("\"tag_name\":\"");
    if (tagIdx < 0) { otaSetMsg(OTA_ERROR, "未找到 tag_name"); return false; }
    tagIdx += 12;
    int tagEnd = body.indexOf('"', tagIdx);
    if (tagEnd < 0) { otaSetMsg(OTA_ERROR, "tag 解析失败"); return false; }
    String tag = body.substring(tagIdx, tagEnd);
    if (tag.startsWith("v")) tag = tag.substring(1);
    strlcpy(gOta.latestVersion, tag.c_str(), sizeof(gOta.latestVersion));

    String envTag = FIRMWARE_ENV_TAG;
    String suffix = String("_v") + gOta.latestVersion + "_ota.bin";
    gOta.latestUrl[0] = '\0';

    int pos = 0;
    while (true) {
        int nameIdx = body.indexOf("\"name\":\"", pos);
        if (nameIdx < 0) break;
        nameIdx += 8;
        int nameEnd = body.indexOf('"', nameIdx);
        if (nameEnd < 0) break;
        String name = body.substring(nameIdx, nameEnd);
        bool hit = name.startsWith(envTag + "_")
                && name.endsWith("_ota.bin")
                && (name == envTag + suffix
                 || name.indexOf("_v") == (int)envTag.length());
        if (hit) {
            int urlIdx = body.indexOf("\"browser_download_url\":\"", nameEnd);
            if (urlIdx >= 0) {
                urlIdx += 24;
                int urlEnd = body.indexOf('"', urlIdx);
                if (urlEnd > urlIdx) {
                    String du = body.substring(urlIdx, urlEnd);
                    strlcpy(gOta.latestUrl, du.c_str(), sizeof(gOta.latestUrl));
                    break;
                }
            }
        }
        pos = nameEnd + 1;
    }

    if (gOta.latestUrl[0] == '\0') {
        otaSetMsg(OTA_ERROR, "未找到 %s 对应资产", FIRMWARE_ENV_TAG);
        return false;
    }
    otaSetMsg(OTA_READY, "最新 %s (当前 %s)", gOta.latestVersion, FIRMWARE_VERSION);
    return true;
}

static bool otaCheckLatest() {
    if (WiFi.status() != WL_CONNECTED) {
        otaSetMsg(OTA_ERROR, "STA 未连接,无法联网检查更新");
        return false;
    }
    gOta.state = OTA_CHECKING;
    gOtaBody = String();
    gOtaBody.reserve(8192);

    String url = String("https://api.github.com/repos/")
               + OTA_RELEASE_OWNER + "/" + OTA_RELEASE_REPO + "/releases/latest";

    esp_http_client_config_t cfg = {};
    cfg.url                        = url.c_str();
    cfg.timeout_ms                 = 15000;
    cfg.transport_type             = HTTP_TRANSPORT_OVER_SSL;
    cfg.crt_bundle_attach          = OTA_CRT_BUNDLE_ATTACH;
    cfg.event_handler              = otaHttpEvent;
    cfg.disable_auto_redirect      = false;
    cfg.max_redirection_count      = 3;

    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    if (!client) { otaSetMsg(OTA_ERROR, "HTTP init 失败"); return false; }
    esp_http_client_set_header(client, "User-Agent", "tesla-can-web-2/" FIRMWARE_VERSION);
    esp_http_client_set_header(client, "Accept", "application/vnd.github+json");

    esp_err_t err = esp_http_client_perform(client);
    int status = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);

    if (err != ESP_OK) {
        otaSetMsg(OTA_ERROR, "HTTP 错误 %d", (int)err);
        gOtaBody = String();
        return false;
    }
    if (status != 200) {
        otaSetMsg(OTA_ERROR, "GitHub API %d", status);
        gOtaBody = String();
        return false;
    }
    bool ok = otaParseLatest(gOtaBody);
    gOtaBody = String();
    return ok;
}

static void otaDoPull(const char* url) {
    if (WiFi.status() != WL_CONNECTED) {
        otaSetMsg(OTA_ERROR, "STA 未连接,无法下载");
        return;
    }
    gOta.state   = OTA_DOWNLOADING;
    gOta.written = 0;
    gOta.total   = 0;
    snprintf(gOta.message, sizeof(gOta.message), "开始下载…");

    esp_http_client_config_t http_cfg = {};
    http_cfg.url                       = url;
    http_cfg.timeout_ms                = 30000;
    http_cfg.transport_type            = HTTP_TRANSPORT_OVER_SSL;
    http_cfg.crt_bundle_attach         = OTA_CRT_BUNDLE_ATTACH;
    http_cfg.keep_alive_enable         = true;
    http_cfg.disable_auto_redirect     = false;
    http_cfg.max_redirection_count     = 5;

    esp_https_ota_config_t ota_cfg = {};
    ota_cfg.http_config = &http_cfg;

    esp_https_ota_handle_t handle = nullptr;
    esp_err_t err = esp_https_ota_begin(&ota_cfg, &handle);
    if (err != ESP_OK || !handle) {
        otaSetMsg(OTA_ERROR, "OTA begin 失败 (%d)", (int)err);
        return;
    }

    int32_t total = esp_https_ota_get_image_size(handle);
    gOta.total = (total > 0) ? (uint32_t)total : 0;
    gOta.state = OTA_WRITING;

    while (true) {
        err = esp_https_ota_perform(handle);
        if (err != ESP_ERR_HTTPS_OTA_IN_PROGRESS) break;
        gOta.written = esp_https_ota_get_image_len_read(handle);
    }

    if (err != ESP_OK) {
        esp_https_ota_abort(handle);
        otaSetMsg(OTA_ERROR, "下载失败 (%d)", (int)err);
        return;
    }
    if (!esp_https_ota_is_complete_data_received(handle)) {
        esp_https_ota_abort(handle);
        otaSetMsg(OTA_ERROR, "数据不完整");
        return;
    }
    err = esp_https_ota_finish(handle);
    if (err != ESP_OK) {
        otaSetMsg(OTA_ERROR, "写入失败 (%d)", (int)err);
        return;
    }
    gOta.written = gOta.total;
    otaSetMsg(OTA_SUCCESS, "更新成功,即将重启");
    otaPendingRestart = true;
}

struct OtaTaskArgs { uint8_t op; char url[260]; };

static void otaTaskEntry(void* arg) {
    OtaTaskArgs* a = (OtaTaskArgs*)arg;
    if (a->op == 0)      otaCheckLatest();
    else if (a->op == 1) otaDoPull(a->url);
    delete a;
    vTaskDelete(nullptr);
}

static bool otaStartCheckImpl() {
    if (gOta.state == OTA_CHECKING || gOta.state == OTA_DOWNLOADING
     || gOta.state == OTA_WRITING) return false;
    gOta.latestVersion[0] = '\0';
    gOta.latestUrl[0]     = '\0';
    gOta.state            = OTA_CHECKING;
    auto* a = new OtaTaskArgs{0, {0}};
    BaseType_t r = xTaskCreatePinnedToCore(
        otaTaskEntry, "ota_chk", 10240, a, 1, nullptr, 0);
    if (r != pdPASS) { delete a; otaSetMsg(OTA_ERROR, "任务创建失败"); return false; }
    return true;
}

static bool otaStartPullImpl(const char* url) {
    if (gOta.state == OTA_CHECKING || gOta.state == OTA_DOWNLOADING
     || gOta.state == OTA_WRITING) return false;
    if (!url || !*url) return false;
    auto* a = new OtaTaskArgs{1, {0}};
    strlcpy(a->url, url, sizeof(a->url));
    gOta.state = OTA_DOWNLOADING;
    BaseType_t r = xTaskCreatePinnedToCore(
        otaTaskEntry, "ota_dl", 12288, a, 1, nullptr, 0);
    if (r != pdPASS) { delete a; otaSetMsg(OTA_ERROR, "任务创建失败"); return false; }
    return true;
}

static size_t otaStatusJsonImpl(char* buf, size_t cap) {
    uint8_t  st  = gOta.state;
    uint32_t wr  = gOta.written;
    uint32_t tot = gOta.total;
    return snprintf(buf, cap,
        "{\"state\":%u,\"written\":%u,\"total\":%u,"
        "\"message\":\"%s\",\"latest\":\"%s\",\"url\":\"%s\","
        "\"current\":\"%s\",\"envTag\":\"%s\"}",
        (unsigned)st, (unsigned)wr, (unsigned)tot,
        gOta.message, gOta.latestVersion, gOta.latestUrl,
        FIRMWARE_VERSION, FIRMWARE_ENV_TAG);
}

}  // namespace ota_impl

// ── Public API (global-scope wrappers so main.cpp lambdas can call) ──
static inline bool   otaStartCheck()                       { return ota_impl::otaStartCheckImpl(); }
static inline bool   otaStartPull(const char* url)         { return ota_impl::otaStartPullImpl(url); }
static inline size_t otaStatusJson(char* buf, size_t cap)  { return ota_impl::otaStatusJsonImpl(buf, cap); }
static inline const char* otaLatestUrl()                   { return ota_impl::gOta.latestUrl; }
