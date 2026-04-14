#pragma once
// ── Module: Diagnostic log ────────────────────────────────────────────────────
// Circular text buffer, 80 entries × 96 chars = ~7.5 KB.
// Written from canTask (Core1) and web task (Core0). Each addDiagLog call is a
// single snprintf + two integer increments — no compound operation, safe on
// ESP32 Xtensa without a mutex.
//
// Usage:
//   addDiagLog(uptime_s, "FSD triggered");   // plain string
//   char m[64]; snprintf(m,64,"errors=%u",(unsigned)n); addDiagLog(up,m);
//
// HTTP handler uses diagLogCount() / diagLogAt(i) to stream entries.

#include <cstdint>
#include <cstdio>
#include <cstring>

static constexpr uint16_t DIAG_CAP      = 80;
static constexpr uint8_t  DIAG_LINE_MAX = 96;

static char     diagBuf[DIAG_CAP][DIAG_LINE_MAX];  // NOLINT
static uint16_t diagHead  = 0;                      // NOLINT — next write slot
static uint16_t diagCount = 0;                      // NOLINT — entries stored (≤ DIAG_CAP)

// SPIFFS flush cursor — total entries ever written (never wraps).
// Core 0 compares against diagTotal to find entries not yet flushed to flash.
static uint32_t diagTotal    = 0;  // NOLINT — protected by diagMux
static uint32_t diagFlushed  = 0;  // NOLINT — updated by flushLogsToSpiffs (Core 0 only)

// Cross-core spinlock — protects diagBuf/diagHead/diagCount/diagTotal.
// taskENTER_CRITICAL_SAFE disables interrupts on the calling core and acquires
// the spinlock, blocking the other core until taskEXIT_CRITICAL_SAFE releases it.
static portMUX_TYPE diagMux = portMUX_INITIALIZER_UNLOCKED;  // NOLINT

inline void addDiagLog(uint32_t uptime_s, const char* msg) {
    uint32_t h = uptime_s / 3600;
    uint32_t m = (uptime_s % 3600) / 60;
    uint32_t s = uptime_s % 60;
    portENTER_CRITICAL(&diagMux);
    snprintf(diagBuf[diagHead], DIAG_LINE_MAX,
             "[%02u:%02u:%02u] %s", (unsigned)h, (unsigned)m, (unsigned)s, msg);
    diagHead = (uint16_t)((diagHead + 1) % DIAG_CAP);
    if (diagCount < DIAG_CAP) diagCount++;
    diagTotal++;
    portEXIT_CRITICAL(&diagMux);
}

inline uint16_t diagLogCount() { return diagCount; }

inline const char* diagLogAt(uint16_t i) {
    uint16_t start = (diagCount < DIAG_CAP) ? 0 : diagHead;
    return diagBuf[(uint16_t)((start + i) % DIAG_CAP)];
}

inline void diagLogClear() {
    portENTER_CRITICAL(&diagMux);
    diagHead = 0; diagCount = 0; diagTotal = 0; diagFlushed = 0;
    portEXIT_CRITICAL(&diagMux);
}
