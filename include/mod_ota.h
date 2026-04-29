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
void saveRestartReason(const char* tag);  // defined in main.cpp

namespace ota_impl {

extern "C" {
    #include "esp_http_client.h"
    #include "esp_https_ota.h"
    #include "esp_ota_ops.h"
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

// Release notes buffer sized for v1.4.32-style releases (~2 KB) with
// headroom. UTF-8 boundary alignment on truncation keeps Chinese characters
// from being split mid-byte.
static constexpr size_t kOtaNotesCap = 4096;

struct OtaStatus {
    volatile uint8_t  state;
    volatile uint32_t total;
    volatile uint32_t written;
    char message[128];
    char latestVersion[24];
    char latestUrl[256];
    char releaseNotes[kOtaNotesCap];
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

// Decode a JSON string starting at `body[start]` (which is the char *after*
// the opening quote) into `out`, capped at `cap-1` bytes plus null terminator.
// Stops at the unescaped closing '"'. Honors \n \r \t \" \\ \/ \b \f and
// \uXXXX (BMP only, BMP-truncated for surrogate pairs). On overflow, walks
// back to a UTF-8 leading byte boundary so multi-byte Chinese characters
// aren't split. Returns the index of the closing quote (or end of body).
static int otaJsonUnescapeInto(const String& body, int start, char* out, size_t cap) {
    if (cap == 0) return start;
    size_t o = 0;
    int i = start;
    int n = (int)body.length();
    auto hexNibble = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
        if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
        return -1;
    };
    auto putc = [&](uint8_t b) {
        if (o + 1 < cap) out[o++] = (char)b;
    };
    while (i < n) {
        char c = body[i];
        if (c == '"') break;  // unescaped closing quote
        if (c == '\\' && i + 1 < n) {
            char e = body[i + 1];
            switch (e) {
                case 'n': putc('\n'); i += 2; break;
                case 'r': putc('\r'); i += 2; break;
                case 't': putc('\t'); i += 2; break;
                case 'b': putc('\b'); i += 2; break;
                case 'f': putc('\f'); i += 2; break;
                case '"': putc('"');  i += 2; break;
                case '\\': putc('\\'); i += 2; break;
                case '/':  putc('/');  i += 2; break;
                case 'u': {
                    if (i + 5 >= n) { i = n; break; }
                    int h0 = hexNibble(body[i + 2]);
                    int h1 = hexNibble(body[i + 3]);
                    int h2 = hexNibble(body[i + 4]);
                    int h3 = hexNibble(body[i + 5]);
                    if ((h0 | h1 | h2 | h3) < 0) { i += 2; break; }
                    uint32_t cp = (h0 << 12) | (h1 << 8) | (h2 << 4) | h3;
                    if (cp < 0x80) {
                        putc((uint8_t)cp);
                    } else if (cp < 0x800) {
                        putc(0xC0 | (cp >> 6));
                        putc(0x80 | (cp & 0x3F));
                    } else {
                        putc(0xE0 | (cp >> 12));
                        putc(0x80 | ((cp >> 6) & 0x3F));
                        putc(0x80 | (cp & 0x3F));
                    }
                    i += 6;
                    break;
                }
                default:  putc(e); i += 2; break;
            }
        } else {
            putc((uint8_t)c);
            i += 1;
        }
    }
    // UTF-8 boundary trim: if we filled the buffer, walk back so we don't
    // leave a partial multi-byte sequence at the tail. Strip continuation
    // bytes (0b10xxxxxx) — anything left with the high bit set must be a
    // multi-byte lead whose follow-up bytes were dropped, so drop it too.
    if (o + 1 >= cap && o > 0) {
        while (o > 0 && (((uint8_t)out[o - 1]) & 0xC0) == 0x80) o--;
        if (o > 0 && (((uint8_t)out[o - 1]) & 0x80) != 0) o--;
    }
    out[o] = '\0';
    // Advance i to past the closing quote if present
    if (i < n && body[i] == '"') i++;
    return i;
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

    // Parse body field (release notes). Optional — absence is not fatal.
    gOta.releaseNotes[0] = '\0';
    int bodyIdx = body.indexOf("\"body\":\"");
    if (bodyIdx >= 0) {
        otaJsonUnescapeInto(body, bodyIdx + 8, gOta.releaseNotes, sizeof(gOta.releaseNotes));
    }

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

// Mirror list tried in order for both API-check and asset download.
// All of these accept the "<mirror>/<origin-url>" convention (prefix style),
// so the same helper serves both api.github.com and github.com URLs.
// Direct origin is always tried last as a final fallback.
static constexpr const char* kOtaMirrors[] = {
    "https://gh-proxy.com",
    "https://ghfast.top",
    "https://mirror.ghproxy.com",
};
static constexpr size_t kOtaMirrorCount = sizeof(kOtaMirrors) / sizeof(kOtaMirrors[0]);

// Return a short host-only label for UI messages (e.g. "gh-proxy.com").
static const char* otaMirrorLabel(const char* mirror) {
    const char* p = strstr(mirror, "://");
    return p ? p + 3 : mirror;
}

// Build "<mirror>/<origin>" with exactly one slash between.
static String otaWithMirror(const char* mirror, const String& origin) {
    String m(mirror);
    while (m.endsWith("/")) m.remove(m.length() - 1);
    if (m.isEmpty()) return origin;
    return m + "/" + origin;
}

// Perform one GitHub API GET against `url`. Populates gOtaBody on success.
// Returns true iff err == ESP_OK && status == 200.
static bool otaApiGet(const String& url) {
    gOtaBody = String();
    gOtaBody.reserve(8192);

    esp_http_client_config_t cfg = {};
    cfg.url                        = url.c_str();
    cfg.timeout_ms                 = 8000;  // short per-attempt so slow nodes fail fast
    cfg.transport_type             = HTTP_TRANSPORT_OVER_SSL;
    cfg.crt_bundle_attach          = OTA_CRT_BUNDLE_ATTACH;
    cfg.event_handler              = otaHttpEvent;
    cfg.disable_auto_redirect      = false;
    cfg.max_redirection_count      = 5;
    cfg.buffer_size                = 4096;

    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    if (!client) return false;
    esp_http_client_set_header(client, "User-Agent", "tesla-can-web-2/" FIRMWARE_VERSION);
    esp_http_client_set_header(client, "Accept", "application/vnd.github+json");

    esp_err_t err = esp_http_client_perform(client);
    int status = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);
    return err == ESP_OK && status == 200;
}

static bool otaCheckLatest() {
    if (WiFi.status() != WL_CONNECTED) {
        otaSetMsg(OTA_ERROR, "STA 未连接,无法联网检查更新");
        return false;
    }
    gOta.state = OTA_CHECKING;

    String origin = String("https://api.github.com/repos/")
                  + OTA_RELEASE_OWNER + "/" + OTA_RELEASE_REPO + "/releases/latest";

    // Try each mirror in order, then direct as last resort.
    for (size_t i = 0; i <= kOtaMirrorCount; ++i) {
        String url;
        if (i < kOtaMirrorCount) {
            url = otaWithMirror(kOtaMirrors[i], origin);
            snprintf(gOta.message, sizeof(gOta.message), "检查更新 (%s)…", otaMirrorLabel(kOtaMirrors[i]));
        } else {
            url = origin;
            snprintf(gOta.message, sizeof(gOta.message), "检查更新 (直连 github)…");
        }
        if (otaApiGet(url)) {
            bool ok = otaParseLatest(gOtaBody);
            gOtaBody = String();
            return ok;  // parse success or terminal parse error — don't re-try mirrors
        }
        gOtaBody = String();
    }

    otaSetMsg(OTA_ERROR, "所有节点均失败");
    return false;
}

// Single download attempt. Returns ESP_OK on full-image success, else error.
// Does NOT call otaSetMsg on success (caller decides) but sets progress state.
// Does NOT call saveRestartReason / set otaPendingRestart — caller handles.
static esp_err_t otaTryDownload(const char* url) {
    gOta.state   = OTA_DOWNLOADING;
    gOta.written = 0;
    gOta.total   = 0;

    esp_http_client_config_t http_cfg = {};
    http_cfg.url                       = url;
    http_cfg.timeout_ms                = 30000;
    http_cfg.transport_type            = HTTP_TRANSPORT_OVER_SSL;
    http_cfg.crt_bundle_attach         = OTA_CRT_BUNDLE_ATTACH;
    http_cfg.keep_alive_enable         = true;
    http_cfg.disable_auto_redirect     = false;
    http_cfg.max_redirection_count     = 5;
    // GitHub Releases 302 to objects.githubusercontent.com with a 500+ byte
    // S3 signed Location header. Default 512-byte recv buffer truncates it,
    // esp_http_client_set_redirection parses a broken URL, and the retry
    // lands on a garbage host that returns a bogus status → esp_https_ota's
    // _http_handle_response_code maps to ESP_FAIL(-1).
    http_cfg.buffer_size               = 4096;
    http_cfg.buffer_size_tx            = 1024;

    esp_https_ota_config_t ota_cfg = {};
    ota_cfg.http_config = &http_cfg;

    esp_https_ota_handle_t handle = nullptr;
    esp_err_t err = esp_https_ota_begin(&ota_cfg, &handle);
    if (err != ESP_OK || !handle) return (err != ESP_OK ? err : ESP_FAIL);

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
        return err;
    }
    if (!esp_https_ota_is_complete_data_received(handle)) {
        esp_https_ota_abort(handle);
        return ESP_FAIL;
    }
    err = esp_https_ota_finish(handle);
    return err;
}

static void otaDoPull(const char* url) {
    if (WiFi.status() != WL_CONNECTED) {
        otaSetMsg(OTA_ERROR, "STA 未连接,无法下载");
        return;
    }

    String origin(url);
    // Only github.com asset URLs can be mirrored; non-github URLs go direct only.
    bool mirrorable = origin.startsWith("https://github.com/");
    size_t tries = mirrorable ? (kOtaMirrorCount + 1) : 1;

    esp_err_t err = ESP_FAIL;
    for (size_t i = 0; i < tries; ++i) {
        String attempt;
        if (mirrorable && i < kOtaMirrorCount) {
            attempt = otaWithMirror(kOtaMirrors[i], origin);
            snprintf(gOta.message, sizeof(gOta.message), "下载中 (%s)…", otaMirrorLabel(kOtaMirrors[i]));
        } else {
            attempt = origin;
            snprintf(gOta.message, sizeof(gOta.message), "下载中 (直连 github)…");
        }
        err = otaTryDownload(attempt.c_str());
        if (err == ESP_OK) {
            gOta.written = gOta.total;
            otaSetMsg(OTA_SUCCESS, "更新成功,即将重启");
            saveRestartReason("ota pull success");
            otaPendingRestart = true;
            return;
        }
    }

    otaSetMsg(OTA_ERROR, "下载失败 (%d)", (int)err);
}

struct OtaTaskArgs { uint8_t op; char url[260]; };

static void otaTaskEntry(void* arg) {
    OtaTaskArgs* a = (OtaTaskArgs*)arg;
    if (a->op == 0)      otaCheckLatest();
    else if (a->op == 1) otaDoPull(a->url);
    delete a;
    vTaskDelete(nullptr);
}

// SSL + esp_crt_bundle needs ~50 KB free heap at handshake time. Below this
// threshold, mbedTLS allocations fail deep in the stack and the device panics.
// Field-reported on v1.4.18 Waveshare S3 Wi-Fi bridge: click "check update"
// → LED reboot → uptime resets to 0.
static constexpr size_t kOtaMinFreeHeap = 60 * 1024;

static bool otaStartCheckImpl() {
    if (gOta.state == OTA_CHECKING || gOta.state == OTA_DOWNLOADING
     || gOta.state == OTA_WRITING) return false;
    size_t freeHeap = esp_get_free_heap_size();
    if (freeHeap < kOtaMinFreeHeap) {
        otaSetMsg(OTA_ERROR, "内存不足 (%u KB),请重启后再试", (unsigned)(freeHeap / 1024));
        return false;
    }
    gOta.latestVersion[0] = '\0';
    gOta.latestUrl[0]     = '\0';
    gOta.state            = OTA_CHECKING;
    auto* a = new OtaTaskArgs{0, {0}};
    BaseType_t r = xTaskCreatePinnedToCore(
        otaTaskEntry, "ota_chk", 16384, a, 1, nullptr, 0);
    if (r != pdPASS) { delete a; otaSetMsg(OTA_ERROR, "任务创建失败"); return false; }
    return true;
}

static bool otaStartPullImpl(const char* url) {
    if (gOta.state == OTA_CHECKING || gOta.state == OTA_DOWNLOADING
     || gOta.state == OTA_WRITING) return false;
    if (!url || !*url) return false;
    size_t freeHeap = esp_get_free_heap_size();
    if (freeHeap < kOtaMinFreeHeap) {
        otaSetMsg(OTA_ERROR, "内存不足 (%u KB),请重启后再试", (unsigned)(freeHeap / 1024));
        return false;
    }
    auto* a = new OtaTaskArgs{1, {0}};
    strlcpy(a->url, url, sizeof(a->url));
    gOta.state = OTA_DOWNLOADING;
    BaseType_t r = xTaskCreatePinnedToCore(
        otaTaskEntry, "ota_dl", 16384, a, 1, nullptr, 0);
    if (r != pdPASS) { delete a; otaSetMsg(OTA_ERROR, "任务创建失败"); return false; }
    return true;
}

// Encode `in` as a JSON string body (without surrounding quotes) into `out`.
// Escapes \\ \" and control chars; passes UTF-8 through verbatim. Truncates
// safely at multi-byte boundaries so the JSON parser on the client side can
// still decode it. Returns bytes written (not counting null).
static size_t otaJsonEscapeInto(const char* in, char* out, size_t cap) {
    if (cap == 0) return 0;
    size_t o = 0;
    auto put = [&](char c) -> bool {
        if (o + 1 >= cap) return false;
        out[o++] = c;
        return true;
    };
    for (size_t i = 0; in[i]; ++i) {
        uint8_t c = (uint8_t)in[i];
        bool ok = true;
        switch (c) {
            case '\\': ok = put('\\') && put('\\'); break;
            case '"':  ok = put('\\') && put('"');  break;
            case '\n': ok = put('\\') && put('n');  break;
            case '\r': ok = put('\\') && put('r');  break;
            case '\t': ok = put('\\') && put('t');  break;
            case '\b': ok = put('\\') && put('b');  break;
            case '\f': ok = put('\\') && put('f');  break;
            default:
                if (c < 0x20) {
                    char tmp[8];
                    int  m = snprintf(tmp, sizeof(tmp), "\\u%04x", c);
                    for (int k = 0; k < m && ok; ++k) ok = put(tmp[k]);
                } else {
                    ok = put((char)c);
                }
                break;
        }
        if (!ok) {
            // Walk back to a UTF-8 lead-byte boundary so we don't leave a
            // partial sequence (which would break JSON.parse on the client).
            while (o > 0 && (((uint8_t)out[o - 1]) & 0xC0) == 0x80) o--;
            if (o > 0 && (((uint8_t)out[o - 1]) & 0x80) != 0) o--;
            break;
        }
    }
    out[o] = '\0';
    return o;
}

static size_t otaStatusJsonImpl(char* buf, size_t cap) {
    uint8_t  st  = gOta.state;
    uint32_t wr  = gOta.written;
    uint32_t tot = gOta.total;
    return snprintf(buf, cap,
        "{\"state\":%u,\"written\":%u,\"total\":%u,"
        "\"message\":\"%s\",\"latest\":\"%s\",\"url\":\"%s\","
        "\"current\":\"%s\",\"envTag\":\"%s\","
        "\"hasNotes\":%s}",
        (unsigned)st, (unsigned)wr, (unsigned)tot,
        gOta.message, gOta.latestVersion, gOta.latestUrl,
        FIRMWARE_VERSION, FIRMWARE_ENV_TAG,
        gOta.releaseNotes[0] ? "true" : "false");
}

// Separate endpoint for release notes — fetched once per check, not on each
// /api/ota/status poll (which the UI hits every couple seconds during OTA).
// Caller should size buf >= 6 KB to hold the worst-case JSON-escaped 4 KB
// notes plus the small version/envelope. Truncates safely if smaller.
static constexpr size_t kOtaNotesJsonSuffix = 3;  // sizeof("\"}\0")

static size_t otaNotesJsonImpl(char* buf, size_t cap) {
    if (cap < 64) { if (cap) buf[0] = '\0'; return 0; }
    int prefix = snprintf(buf, cap,
        "{\"version\":\"%s\",\"notes\":\"", gOta.latestVersion);
    if (prefix < 0 || (size_t)prefix >= cap) { buf[0] = '\0'; return 0; }
    size_t o = (size_t)prefix;
    if (cap - o < kOtaNotesJsonSuffix + 1) { buf[o] = '\0'; return o; }
    size_t notesCap = cap - o - kOtaNotesJsonSuffix;
    size_t wrote = otaJsonEscapeInto(gOta.releaseNotes, buf + o, notesCap);
    o += wrote;
    buf[o++] = '"';
    buf[o++] = '}';
    buf[o]   = '\0';
    return o;
}

// Report both OTA partitions so the UI can tell the user what a rollback
// would land on. `prev` is empty/null when the other slot was never written
// (fresh flash — nothing to roll back to).
static size_t otaPartInfoJsonImpl(char* buf, size_t cap) {
    const esp_partition_t* running = esp_ota_get_running_partition();
    const esp_partition_t* next    = esp_ota_get_next_update_partition(nullptr);

    char curVer[32] = "", curDate[16] = "", curLabel[8] = "";
    char prvVer[32] = "", prvDate[16] = "", prvLabel[8] = "";
    bool canRollback = false;

    if (running) {
        strlcpy(curLabel, running->label, sizeof(curLabel));
        esp_app_desc_t d;
        if (esp_ota_get_partition_description(running, &d) == ESP_OK) {
            strlcpy(curVer,  d.version, sizeof(curVer));
            strlcpy(curDate, d.date,    sizeof(curDate));
        }
    }
    if (next && next != running) {
        esp_app_desc_t d;
        if (esp_ota_get_partition_description(next, &d) == ESP_OK) {
            strlcpy(prvLabel, next->label, sizeof(prvLabel));
            strlcpy(prvVer,   d.version,   sizeof(prvVer));
            strlcpy(prvDate,  d.date,      sizeof(prvDate));
            canRollback = true;
        }
    }

    return snprintf(buf, cap,
        "{\"canRollback\":%s,"
        "\"current\":{\"label\":\"%s\",\"version\":\"%s\",\"date\":\"%s\"},"
        "\"previous\":{\"label\":\"%s\",\"version\":\"%s\",\"date\":\"%s\"}}",
        canRollback ? "true" : "false",
        curLabel, curVer, curDate,
        prvLabel, prvVer, prvDate);
}

// Flip the boot partition to the other OTA slot. Caller is responsible for
// scheduling the restart (we don't reboot in-handler so the HTTP response can
// flush first). On error, writes a short reason into `err` and returns false.
static bool otaDoRollbackImpl(char* err, size_t errCap) {
    const esp_partition_t* running = esp_ota_get_running_partition();
    const esp_partition_t* next    = esp_ota_get_next_update_partition(nullptr);
    if (!next || next == running) {
        strlcpy(err, "no_previous", errCap);
        return false;
    }
    esp_app_desc_t d;
    if (esp_ota_get_partition_description(next, &d) != ESP_OK) {
        strlcpy(err, "previous_empty", errCap);
        return false;
    }
    esp_err_t r = esp_ota_set_boot_partition(next);
    if (r != ESP_OK) {
        snprintf(err, errCap, "esp_err_0x%x", (unsigned)r);
        return false;
    }
    return true;
}

}  // namespace ota_impl

// ── Public API (global-scope wrappers so main.cpp lambdas can call) ──
static inline bool   otaStartCheck()                       { return ota_impl::otaStartCheckImpl(); }
static inline bool   otaStartPull(const char* url)         { return ota_impl::otaStartPullImpl(url); }
static inline size_t otaStatusJson(char* buf, size_t cap)  { return ota_impl::otaStatusJsonImpl(buf, cap); }
static inline size_t otaNotesJson(char* buf, size_t cap)   { return ota_impl::otaNotesJsonImpl(buf, cap); }
static inline size_t otaPartInfoJson(char* buf, size_t cap){ return ota_impl::otaPartInfoJsonImpl(buf, cap); }
static inline bool   otaDoRollback(char* err, size_t errCap){ return ota_impl::otaDoRollbackImpl(err, errCap); }
static inline const char* otaLatestUrl()                   { return ota_impl::gOta.latestUrl; }
// True while a network OTA task is running. Main loop must skip blocking
// Wi-Fi service calls during this window — WiFi.begin() inside
// serviceUpstreamWiFi() can stall for seconds while mbedTLS on core 0 holds
// the Wi-Fi stack, and a stall > 5 s trips esp_task_wdt_init(5, true) panic.
static inline bool   otaIsActive() {
    uint8_t s = ota_impl::gOta.state;
    return s == ota_impl::OTA_CHECKING
        || s == ota_impl::OTA_DOWNLOADING
        || s == ota_impl::OTA_WRITING;
}
