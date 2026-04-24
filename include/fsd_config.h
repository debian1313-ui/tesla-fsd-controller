#pragma once
#include <cstdint>

// ── HW3 policy constants (shared with mod_fsd.h, defaults below, and UI labels) ──
// Auto targets (field-calibrated — metric cluster under-delivers vs request):
static constexpr int kHw3AutoTargetBelow60Kph      = 64;   // fusedLimit <60 kph  → request 64 (visible ~50)
static constexpr int kHw3AutoTargetAt60Kph         = 100;  // fusedLimit =60 kph  → request 100 (visible ~80)
static constexpr int kHw3AutoTargetForVisible80Kph = 85;   // 60<fusedLimit<80    → request 85 (visible ~70)
static constexpr int kHw3StockOffsetCutoverKph     = 80;   // ≥80 kph → passthrough stock EAP offset
// Custom mode: user-defined target-speed lookup bucketed by 10 kph starting at 30.
static constexpr int kHw3CustomBucketBaseKph = 30;
static constexpr int kHw3CustomBucketStepKph = 10;
static constexpr int kHw3CustomTargetCount   = 5;  // 30/40/50/60/70 kph limit buckets
static_assert((kHw3StockOffsetCutoverKph - kHw3CustomBucketBaseKph)
                / kHw3CustomBucketStepKph == kHw3CustomTargetCount,
              "Custom target table must cover [base, cutover) in step-sized buckets");

// High-speed (≥80 kph) table: user-defined pct offset per bucket. Limits
// above the top bucket clamp to the last entry (120 kph covers 120+).
static constexpr int kHw3HighSpeedBucketBaseKph = 80;
static constexpr int kHw3HighSpeedBucketStepKph = 10;
static constexpr int kHw3HighSpeedBucketCount   = 5;  // 80/90/100/110/120 buckets

// ── Runtime-configurable state (shared between CAN task and web server) ──
// All fields are volatile — written by Core1 (CAN), read by Core0 (WiFi/web).
// Individual 32-bit-or-smaller volatile reads are atomic on ESP32 Xtensa.
// Multi-field compound updates are NOT atomic; see handlers.h for usage notes.
struct FSDConfig {
    // FSD control
    volatile bool     fsdEnable           = false;
    volatile uint8_t  hwMode              = 2;       // 0=LEGACY, 1=HW3, 2=HW4
    volatile uint8_t  speedProfile        = 1;       // 0-4
    volatile bool     profileModeAuto     = true;    // true=auto from stalk, false=manual
    volatile bool     isaChimeSuppress    = false;
    volatile bool     emergencyDetection  = true;
    volatile bool     forceActivate       = false;   // bypass isFSDSelectedInUI (regions without TLSSC)
    volatile bool     overrideSpeedLimit  = false;   // Legacy: set UI_visionSpeedSlider=100 in frame 1080
    volatile int      hw3SpeedOffset      = 0;       // stock offset from mux-0 data[3] as pct*5 (0-100); display only
    volatile bool     hw3AutoSpeed        = true;    // HW3 auto speed targeting: <60→64, =60→100, 60-79→85, ≥80→stock passthrough
    volatile bool     hw3CustomSpeed      = false;   // HW3 custom target table (overrides hw3AutoSpeed when true); ≥80 always stock passthrough
    // Defaults sit at the physical ceiling (limit × 1.5) per slot — Tesla fw
    // caps offset at 50% of posted limit, so anything higher silently clamps.
    // Users can dial back if they want less aggressive.
    volatile uint8_t  hw3CustomTarget[kHw3CustomTargetCount] = {
        45,   // 30 kph bucket → max +15
        60,   // 40 kph bucket → max +20
        75,   // 50 kph bucket → max +25
        90,   // 60 kph bucket → max +30
        105,  // 70 kph bucket → max +35
    };
    volatile uint8_t  hw4OffsetRaw       = 0;       // HW4 mux-2 data[1][5:0]; 0=off. Hardware field=6 bits (cap 63), UI cap=15 (v1.4.28, aligned with upstream Turkish fw). Legacy presets 7/10/14 still valid; 21 now clamps to 15.
    // ISA speed-limit override (v1.4.28, HW4 only). When ON and FSD is active,
    // we re-broadcast 0x39B with DAS_fusedSpeedLimit (byte1[4:0]) and
    // DAS_visionOnlySpeedLimit (byte2[4:0]) forced to raw=31 ("NONE"), so the
    // DAS stops clamping the set/cruise speed to the nav/vision limit on 2024+
    // firmware. The chime also goes silent as a side effect — the car's chime
    // logic is gated on a valid fusedSpeedLimit, and forcing NONE disarms it
    // without needing isaChimeSuppress on top. NVS key "df".
    volatile bool     isaOverride         = false;
    volatile uint8_t  hwDetected          = 0;       // from 0x398: 0=unknown, 1=HW3, 2=HW4 (informational only)
    volatile int8_t   gatewayAutopilot    = -1;      // from 0x7FF mux-2: -1=unseen, 0=NONE,1=HIGHWAY,2=ENHANCED,3=SELF_DRIVING,4=BASIC
    volatile bool     trackModeEnable     = false;   // HW3: echo 0x313 with UI_trackModeRequest=ON (off by default)

    // Climate — read from 0x283 (BODY_R1, GTW bus)
    volatile bool     tempSeen          = false;
    volatile uint8_t  tempInsideRaw     = 0;   // AirTemp_Insd  data[5]: raw × 0.25 = °C
    volatile uint8_t  tempOutsideRaw    = 0;   // AirTemp_Outsd data[7]: raw × 0.5 − 40 = °C

    // BMS — read-only sniff, no transmission
    volatile bool     bmsSeen             = false;
    volatile uint32_t packVoltage_cV      = 0;   // centivolt  (÷100 = V)
    volatile int32_t  packCurrent_dA      = 0;   // deciampere (÷10  = A, signed)
    volatile uint32_t socPercent_d        = 0;   // deci-%     (÷10  = %)
    volatile int8_t   battTempMin         = 0;   // °C
    volatile int8_t   battTempMax         = 0;   // °C

    // Lighting — read from 0x293 (DAS_settings)
    volatile bool     adaptiveLighting    = false;  // DAS_adaptiveHeadlights bit 22
    volatile bool     highBeamForce       = false;  // user override via UI or stalk gesture

    // Legacy 0x2F8 (760) UI_gpsVehicleSpeed sniffer — read-only, no transmission.
    // Used to validate whether the frame is visible on Party CAN and sample its
    // byte 5 (UI_userSpeedOffset) / byte 6 (UI_mppSpeedLimit) for Legacy speed
    // breakout feasibility (see project_legacy_speed_breakout_pending memory).
    volatile bool     gpsSpeedSeen        = false;
    volatile uint8_t  gpsUserOffsetRaw    = 0;   // bit40|6 — actual kph = raw − 30
    volatile uint8_t  gpsMppLimitRaw      = 0;   // bit48|5 — actual kph = raw × 5
    volatile uint32_t gpsSpeedPeriodMs    = 0;   // last inter-frame delta
    volatile uint32_t gpsSpeedLastMs      = 0;   // internal — last receive ts
    volatile uint32_t gpsSpeedCount       = 0;

    // Legacy speed breakout (v1.4.27): write 0x2F8 UI_userSpeedOffset (bit40|6, +30 raw).
    // Only written when hwMode == 0 (LEGACY). Preserves bits 6-7 of byte 5 so
    // UI_userSpeedOffsetUnits (bit 47) stays at whatever the car has configured,
    // meaning the offset unit follows the car's display (kph/mph).
    volatile uint8_t  legacyOffset            = 0;     // 0=off, 1..33 → raw = value+30 written to 0x2F8[5]
    // removeVisionSpeedLimit: was hardcoded inside 0x3EE mux-1 (bit 48 clear) whenever
    // FSD activation triggered. Now an independent user toggle — default true to preserve
    // pre-v1.4.27 behavior. When false, UI_enableVisionSpeedControl is left untouched.
    volatile bool     removeVisionSpeedLimit  = true;
    // 0x3EE (1006) mux-0 byte 7 low-nibble write inside the Legacy FSD activation
    // path. Historically hardcoded to 0x0F. Now exposed:
    //   legacyByte7Enable — when true, write low-nibble; when false, leave byte 7
    //                       low-nibble untouched (preserve incoming bits).
    //   legacyByte7Value  — 0..15 (4-bit) value to OR into byte 7 low-nibble.
    // Defaults preserve pre-existing behavior (enabled, value 0x0F).
    volatile bool     legacyByte7Enable       = true;
    volatile uint8_t  legacyByte7Value        = 0x0F;  // 0..15

    // DAS status — read from 0x39B (DAS_status), 0x399 (DAS_status ISA) and 0x389 (DAS_status2)
    volatile uint8_t  fusedSpeedLimit     = 0;   // DAS_fusedSpeedLimit        0x39B byte1[4:0]  ×5=kph; 0=none (camera+map)
    volatile uint8_t  visionSpeedLimit    = 0;   // DAS_visionOnlySpeedLimit   0x39B bit16|5     ×5=kph; 0=none (camera only)

    // HW3 offset slew limiter (test feature, v1.4.27 — default OFF, opt-in):
    // when the computed offset *drops* (e.g. posted limit fell from 80→40 and
    // custom target shrinks), ramp the outgoing raw byte down over time instead
    // of snapping. Prevents the harsh deceleration users see when the car's
    // fused limit suddenly lowers. Upward steps still pass through immediately.
    volatile bool     hw3OffsetSlew         = false;  // default off until field-validated
    volatile uint8_t  hw3SlewRatePctPerSec  = 5;     // 1..25 %/s; 5 ≈ 3 kph/s at 60 limit (raw/s = ×4)
    volatile uint8_t  hw3OffsetLastRaw      = 0;     // last raw byte we actually sent
    volatile uint32_t hw3OffsetLastSentMs   = 0;     // when we last sent hw3OffsetLastRaw
    volatile uint8_t  hw3OffsetTargetRaw    = 0;     // last target (raw) requested by encoder; for diag
    volatile uint32_t hw3OffsetSlewCount    = 0;     // # frames the slew limiter capped a drop

    // HW3 high-speed (≥80 kph fused limit) custom offset — default OFF.
    // Original behavior (when disabled): passthrough stock EAP offset at ≥80.
    // Enabled: write pct×4 raw using hw3HighSpeedTargetPct[] indexed by
    // (fusedLimitKph − 80) / 10 (80/90/100/110/120 buckets; >120 uses last).
    // Clamps to Tesla's 50% firmware ceiling; UI enforces a softer 30% default cap.
    volatile bool     hw3HighSpeedEnable    = false;
    // Per-bucket pct for 80/90/100/110/120 kph fused limits.
    // Typical 10-20; clamps at 50 (Tesla fw cap). UI enforces softer default.
    volatile uint8_t  hw3HighSpeedTargetPct[kHw3HighSpeedBucketCount] = {25, 25, 25, 25, 25};

    // HW3 0x3FD mux-2 offset wire encoding (v1.4.28). Two Tesla firmware
    // revisions in the wild decode this byte differently:
    //   PCT4: raw = pct × 4  (tesla-open-can-mod reference; Issue #9 user)
    //   KPH5: raw = offsetKph × 5  (v1.4.25 behavior; 2024 HW3 user)
    // Default PCT4 preserves v1.4.27 behavior. Users whose car mis-targets
    // can flip to KPH5 via UI. NVS key "dd".
    volatile uint8_t  hw3WireEncoding       = 1;  // 0=KPH5, 1=PCT4

    // IP blocker enable gate (v1.4.29). The DNS module's IPv4 forward hook
    // scans a 256-entry blocked IP table on every outbound packet — a real
    // per-packet cost even when no IPs are blocked yet. Default OFF so the
    // hook short-circuits immediately; users who want IP-level blocking
    // can opt in via UI. NVS key "de".
    volatile bool     ipBlockerEnabled      = false;
    volatile uint8_t  nagLevel            = 0;   // DAS_autopilotHandsOnState  bit42|4  0=ok, 1-15=nag
    volatile uint8_t  fcwLevel            = 0;   // DAS_forwardCollisionWarning bit22|2  0=none
    volatile uint8_t  accState            = 0;   // DAS_ACC_report             bit26|5  0=off, >0=AP active
    volatile uint8_t  sideCollision       = 0;   // DAS_sideCollisionWarning   bit32|2  0=none,1=left,2=right,3=both
    volatile uint8_t  laneDeptWarning     = 0;   // DAS_laneDepartureWarning   bit37|3  0=none
    volatile uint8_t  laneChangeState     = 0;   // DAS_autoLaneChangeState    bit46|5  0=idle

    // DAS settings readback — from 0x293
    volatile bool     autosteerOn         = false; // DAS_autosteerEnabled  bit38
    volatile bool     aebOn               = false; // DAS_aebEnabled        bit18
    volatile bool     fcwOn               = false; // DAS_fcwEnabled        bit34

    // AP auto-restart — inject 0x293 with autosteerEnabled=1 on disengage
    volatile bool     apRestart           = false;
    volatile uint8_t  apRestartCache[8]   = {};    // last received 0x293 raw bytes
    volatile bool     apRestartValid      = false; // cache has at least one frame

    // Performance test — 0→100 acceleration and 100→0 braking
    volatile uint8_t  perfAccelState      = 0;   // 0=idle,1=armed,2=running,3=done
    volatile uint8_t  perfBrakeState      = 0;
    volatile uint32_t perfAccelMs         = 0;   // result ms
    volatile uint32_t perfBrakeMs         = 0;   // result ms
    volatile uint8_t  perfBrakeEntryKph   = 0;   // actual speed (kph) when braking started

    // Statistics
    volatile uint32_t rxCount             = 0;
    volatile uint32_t modifiedCount       = 0;
    volatile uint32_t errorCount          = 0;
    volatile bool     canOK               = false;
    volatile bool     fsdTriggered        = false;
    volatile uint32_t uptimeStart         = 0;

#ifdef DEBUG_MODE
    // Debug: last captured frame 1021 mux-0 raw bytes
    volatile bool     dbgFrameCaptured    = false;
    volatile uint8_t  dbgFrame[8]         = {};
#endif
};
