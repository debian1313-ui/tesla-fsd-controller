#pragma once
// ── Dashboard page — /dash ────────────────────────────────────────────────────
// Instrument cluster UI served from PROGMEM.
// Polls /api/status every 1 s; animates gauges at 60 fps with lerp.
// Auth: reads sessionStorage 'fsd_tok'; redirects to / on 403.
//
// Layout (landscape-first, flex row):
//   [POWER 24%] | [SPEED 52%] | [RIGHT STACK 24%]
//
// Right stack (always visible, show/hide per data availability):
//   ① Battery card  (bmsSeen)  — compact SOC arc + V/kW/A
//   ② Speed limits              — fused + vision (always shown)
//
// SVG arc geometry (verified):
//   Speed   ViewBox 500×500, r=190, 270° arc:  M 115.6 384.4 A 190 190 0 1 1 384.4 384.4  len≈895
//   Side    ViewBox 260×260, r=108, 270° arc:  M 53.6  206.4 A 108 108 0 1 1 206.4 206.4  len≈509
//   Both arcs span 135°→45° SVG-clockwise (reflecting bottom-left→top→bottom-right).

#ifdef ARDUINO
#include <pgmspace.h>
#else
#define PROGMEM
#endif

const char DASH_HTML[] PROGMEM = R"dash(<!DOCTYPE html>
<html lang="zh">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1,user-scalable=no,viewport-fit=cover">
<meta name="apple-mobile-web-app-capable" content="yes">
<meta name="apple-mobile-web-app-status-bar-style" content="black-translucent">
<meta name="apple-mobile-web-app-title" content="FSD Dash">
<meta name="mobile-web-app-capable" content="yes">
<meta name="theme-color" content="#05080f">
<link rel="manifest" href="/manifest.json">
<title>FSD · Dash</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
html,body{
  width:100%;height:100%;background:#05080f;
  overflow:hidden;
  font-family:-apple-system,'Helvetica Neue',Arial,sans-serif;
  color:#fff;
  -webkit-tap-highlight-color:transparent;
}

/* ── Top bar ───────────────────────────────────────────────── */
#topbar{
  display:flex;align-items:center;justify-content:space-between;
  height:58px;
  padding-top:env(safe-area-inset-top);
  padding-left:max(14px, env(safe-area-inset-left));
  padding-right:max(14px, env(safe-area-inset-right));
  background:rgba(255,255,255,0.02);
  border-bottom:1px solid rgba(255,255,255,0.04);
}
#tb-time{
  flex:0 0 auto;
  font-size:20px;font-weight:300;letter-spacing:2px;
  color:#4a5a7a;min-width:56px;
}
#tb-center{
  display:flex;align-items:center;gap:8px;
}
#tb-gear{
  font-size:52px;font-weight:800;letter-spacing:4px;
  transition:color 0.3s,text-shadow 0.3s;
}
#tb-gear.P{color:#8899cc;text-shadow:0 0 14px rgba(136,153,204,0.4)}
#tb-gear.R{color:#ff3355;text-shadow:0 0 18px rgba(255,51,85,0.5)}
#tb-gear.N{color:#4d6070;text-shadow:none}
#tb-gear.D{color:#00d4aa;text-shadow:0 0 18px rgba(0,212,170,0.5)}
#tb-gear.unknown{color:#334455;text-shadow:none}
#brake-dot{
  width:14px;height:14px;border-radius:50%;flex-shrink:0;
  background:#1a2530;border:1px solid rgba(255,255,255,0.06);
  transition:background 0.15s,box-shadow 0.15s;
}
#brake-dot.on{background:#ff3344;box-shadow:0 0 6px rgba(255,51,68,0.7)}
/* Charging bolt icon */
#chg-icon{
  display:none;flex-shrink:0;
  animation:chg-pulse 1.8s ease-in-out infinite;
}
@keyframes chg-pulse{0%,100%{opacity:.7}50%{opacity:1}}
#tb-right{
  flex:0 0 auto;
  display:flex;align-items:center;gap:8px;
  flex-wrap:nowrap;
}
/* ── Warning badges ─────────────────────────────────────────── */
#can-warn,#net-warn{
  display:none;
  font-size:13px;font-weight:700;letter-spacing:1px;
  padding:2px 6px;border-radius:3px;white-space:nowrap;
  background:rgba(255,40,40,0.15);color:#ff4444;
  border:1px solid rgba(255,40,40,0.4);
  animation:blink 1s step-start infinite;
}
#fcw-warn{
  display:none;
  font-size:13px;font-weight:700;letter-spacing:.5px;
  padding:2px 7px;border-radius:3px;white-space:nowrap;
  background:rgba(255,170,0,0.15);color:#ffaa00;
  border:1px solid rgba(255,170,0,0.45);
  animation:blink 0.6s step-start infinite;
}
.bsd{
  display:none;
  font-size:13px;font-weight:700;
  padding:2px 6px;border-radius:3px;white-space:nowrap;
  background:rgba(255,170,0,0.12);color:#ffaa00;
  border:1px solid rgba(255,170,0,0.4);
}
.bsd.on{display:inline-block}
#ldw-warn{
  display:none;
  font-size:13px;font-weight:700;letter-spacing:.5px;
  padding:2px 6px;border-radius:3px;white-space:nowrap;
  background:rgba(255,100,0,0.12);color:#ff7700;
  border:1px solid rgba(255,100,0,0.35);
}
@keyframes blink{50%{opacity:0.3}}

/* ── Temperature widget ─────────────────────────────────────── */
#tb-temp{display:flex;align-items:center;gap:6px;white-space:nowrap}
.temp-cell{display:flex;align-items:center;gap:3px}
.temp-num{
  font-size:16px;font-weight:600;
  font-variant-numeric:tabular-nums;min-width:32px;text-align:right;
}
.temp-unit{font-size:10px;font-weight:400;letter-spacing:.5px;opacity:.5}
.temp-sep{width:1px;height:12px;background:rgba(255,255,255,0.06);flex-shrink:0}
#temp-out .temp-num{color:#5a9fd4}
#temp-in  .temp-num{color:#d47a3a}
/* AP / FSD badges */
.fsd-badge{
  font-size:13px;font-weight:700;letter-spacing:1px;
  padding:4px 9px;border-radius:3px;
  background:rgba(255,255,255,0.04);color:#445566;
  border:1px solid rgba(255,255,255,0.06);
  transition:all 0.3s;white-space:nowrap;
}
.fsd-badge.on{
  background:rgba(0,212,170,0.12);color:#00d4aa;
  border-color:rgba(0,212,170,0.35);
  box-shadow:0 0 8px rgba(0,212,170,0.2);
}
#fs-btn{
  background:none;border:1px solid rgba(255,255,255,0.08);
  border-radius:5px;color:#3a4a5a;cursor:pointer;
  font-size:13px;line-height:1;padding:4px 6px;
  transition:color 0.2s,border-color 0.2s;flex-shrink:0;
}
#fs-btn:hover{color:#00d4aa;border-color:#00d4aa55}

/* ── Main layout ───────────────────────────────────────────── */
#panels{
  display:flex;align-items:stretch;
  width:100%;height:calc(100vh - 58px);
  padding:6px max(5px, env(safe-area-inset-right)) max(8px, env(safe-area-inset-bottom)) max(5px, env(safe-area-inset-left));
}
.panel{
  display:flex;flex-direction:column;align-items:center;
  justify-content:center;height:100%;
}
#left-panel{
  flex:0 0 17%;
  display:flex;flex-direction:column;
  align-items:center;justify-content:center;
  gap:20px;
}
/* ── Single-letter gear display ──────────────────────────────── */
.gear-big{
  font-size:160px;font-weight:800;line-height:0.9;
  display:flex;align-items:center;justify-content:center;
  width:auto;padding:0 18px;letter-spacing:-6px;
  transition:color .3s, text-shadow .3s, opacity .3s;
}
.gear-big.sel-P{color:#8899cc;text-shadow:0 0 28px rgba(136,153,204,0.5)}
.gear-big.sel-R{color:#ff3355;text-shadow:0 0 32px rgba(255,51,85,0.6)}
.gear-big.sel-N{color:#607080;text-shadow:0 0 24px rgba(96,112,128,0.4)}
.gear-big.sel-D{color:#00d4aa;text-shadow:0 0 32px rgba(0,212,170,0.5)}
.gear-big.sel-none{color:#506070;opacity:0.3;font-size:48px;letter-spacing:-2px}
@keyframes gear-pop{
  0%{transform:scale(1)}
  40%{transform:scale(1.18)}
  70%{transform:scale(0.96)}
  100%{transform:scale(1)}
}
.gear-anim{animation:gear-pop 0.32s ease-out}
/* ── Left panel power kW ──────────────────────────────────────── */
#lp-power{display:none;flex-direction:column;align-items:center;gap:2px}
#lp-pow-num{font-size:22px;font-weight:700;font-variant-numeric:tabular-nums;color:#3a5a6a;transition:color .3s}
#lp-pow-num.drive{color:#00d4aa}
#lp-pow-num.regen{color:#3ad4ff}
.lp-pow-lbl{display:flex;align-items:center;gap:3px}
#center-panel{flex:1;padding:4px 8px;padding-top:3vh}
#right-panel{
  flex:0 0 22%;
  display:flex;flex-direction:column;
  gap:6px;padding:4px 0;
  justify-content:flex-start;
  overflow-y:auto; scrollbar-width:none;
}
#right-panel::-webkit-scrollbar{display:none}
.vdiv{width:1px;align-self:stretch;margin:12px 0;background:rgba(255,255,255,0.035);flex-shrink:0}

/* ── Common gauge elements ─────────────────────────────────── */
.glabel{
  font-size:11px;letter-spacing:3px;text-transform:uppercase;
  color:#1e2e3e;margin-bottom:3px;font-weight:500;text-align:center;
}
.side-wrap{width:min(100%,200px)}
.spd-wrap{
  /* 正方形SVG：受限于高度(vh-topbar-padding)和中间面板宽度(约61vw) */
  width:min(58vw, calc(100vh - 78px));
}

/* ── Nag dots */
#nag-row{display:none;justify-content:center;align-items:center;gap:3px;margin-top:4px}
.nag-dot{width:5px;height:5px;border-radius:50%;background:#1a2530;border:1px solid rgba(255,255,255,0.07);transition:background .3s}

/* ── RIGHT PANEL CARDS ─────────────────────────────────────── */
.rcard{
  background:rgba(255,255,255,0.04);
  border:1px solid rgba(255,255,255,0.09);
  border-radius:10px;
  padding:8px 10px;
  width:100%;
  flex-shrink:0;
}
@keyframes card-in{from{opacity:0;transform:translateY(-5px)}to{opacity:1;transform:translateY(0)}}
.rcard.visible{animation:card-in 0.28s ease-out}

/* Battery card */
#rc-bat{display:none;flex-direction:column;align-items:center}
.bat-arc-wrap{width:min(100%,140px);margin:-4px 0 -6px}
#bat-soc-pct{
  font-size:32px;font-weight:700;color:#00d4aa;
  letter-spacing:-1px;line-height:1;
}
#bat-soc-pct.warn{color:#ff8800}
#bat-soc-pct.crit{color:#ff3344}
.bat-row{
  display:flex;justify-content:space-around;width:100%;
  margin-top:4px;gap:4px;
}
.bat-cell{display:flex;flex-direction:column;align-items:center;gap:1px}
.bat-cell-val{font-size:15px;font-weight:600;color:#4a7090;font-variant-numeric:tabular-nums}
.bat-cell-val.chg{color:#3ad4ff}
.bat-cell-lbl{font-size:9px;color:#1e2e3e;letter-spacing:1.5px;text-transform:uppercase}
/* charging glow on battery card */
#rc-bat.charging{border-color:rgba(58,212,255,0.2);background:rgba(58,212,255,0.025)}

/* Speed limits card */
#rc-lim{display:flex;align-items:center;gap:8px;padding:6px 10px}
.lim-sign{
  display:flex;flex-direction:column;align-items:center;
  flex:1;gap:3px;
}
.lim-circle{
  width:46px;height:46px;border-radius:50%;
  display:flex;align-items:center;justify-content:center;
  font-size:18px;font-weight:900;
  transition:all .3s;
}
.lim-circle.fused{
  background:white;border:4px solid #e01020;color:#111;
}
.lim-circle.fused.sna{background:#0f141e;border-color:#1a2a3a;color:#2e3f50}
.lim-circle.vision{
  background:white;border:4px solid #e01020;color:#111;
  font-size:15px;
}
.lim-circle.vision.sna{background:#0f141e;border-color:#1a2a3a;color:#2e3f50}
.lim-icon-lbl{font-size:8px;color:#1e2e3e;letter-spacing:1.5px;text-transform:uppercase}
.lim-divider{width:1px;height:40px;background:rgba(255,255,255,0.04)}

/* ── Performance timer ────────────────────────────────────── */
#perf-row{
  display:none;margin-top:4px;text-align:center;
  font-size:14px;font-weight:700;letter-spacing:.5px;
  font-variant-numeric:tabular-nums;
}
#perf-row.armed  {color:#3a5a7a;animation:blink .5s step-start infinite}
#perf-row.running{color:#ff8800}
#perf-row.done   {color:#00d4aa}
/* ── Portrait overlay ─────────────────────────────────────── */
#rotate-hint{
  display:none;position:fixed;inset:0;z-index:200;
  background:#05080f;
  flex-direction:column;align-items:center;justify-content:center;gap:12px;
}
#rotate-hint svg{opacity:.35}
#rotate-hint p{font-size:14px;color:#445566;letter-spacing:2px;text-transform:uppercase}
#rotate-hint small{font-size:11px;color:#2e3f50;letter-spacing:1px}
@media (orientation:portrait){#rotate-hint{display:flex}}

/* ── Sentry mode overlay ──────────────────────────────────── */
#sentry-overlay{
  display:none;position:fixed;top:0;left:0;right:0;z-index:150;
  padding:5px 0;text-align:center;
  font-size:13px;font-weight:700;letter-spacing:3px;color:#ff4444;
  background:rgba(255,30,30,0.10);
  border-bottom:1px solid rgba(255,50,50,0.45);
  animation:blink 2s step-start infinite;
  pointer-events:none;
}
#sentry-overlay.on{display:block}

/* ── Day / Light theme ─────────────────────────────────────── */
body.day{background:#eef2f8;color:#1a2030}
body.day #topbar{background:rgba(0,0,0,0.03);border-bottom-color:rgba(0,0,0,0.07)}
body.day #tb-time{color:#7080a0}
body.day #tb-gear.unknown{color:#aabbcc}
body.day #tb-gear.N{color:#6070a0}
body.day #tb-gear.P{color:#7080b0}
body.day .vdiv{background:rgba(0,0,0,0.07)}
body.day .rcard{background:rgba(255,255,255,0.9);border-color:rgba(0,0,0,0.08)}
body.day .glabel{color:#9090a0}
body.day .bat-cell-lbl{color:#9090a0}
body.day #nag-row .nag-dot{background:#d0d8e8;border-color:rgba(0,0,0,0.08)}
body.day .lim-circle.fused.sna,body.day .lim-circle.vision.sna{background:#e0e6f0;border-color:#b0bcd0;color:#7080a0}
body.day .fsd-badge{background:rgba(0,0,0,0.04);color:#7080a0;border-color:rgba(0,0,0,0.1)}
body.day #can-warn,body.day #net-warn{background:rgba(255,40,40,0.1)}
body.day #fs-btn{border-color:rgba(0,0,0,0.12);color:#7080a0}
body.day #rotate-hint{background:#eef2f8}
body.day #rotate-hint p{color:#6070a0}
body.day #rotate-hint small{color:#9090a0}
/* Day: fix hardcoded SVG text colors in speed gauge */
body.day .spd-wrap text{fill:#8090b0}
body.day .spd-wrap #spd-txt{fill:#1a2030}
/* Day: fix battery arc SVG text */
body.day .bat-arc-wrap text{fill:#8090b0}
/* Day: speed + battery arc background tracks */
body.day #spd-bg-arc{stroke:#ccd6e0}
body.day #bat-bg-arc{stroke:#ccd6e0}
/* Day: PRND letters */
body.day .prnd-l{color:#7080a0}
body.day #bat-soc-pct{color:#00b894}
body.day #lp-pow-num{color:#5070a0}
</style>
</head>
<body>

<!-- Sentry mode overlay -->
<div id="sentry-overlay">⚙ SENTRY MODE ACTIVE</div>

<!-- Portrait overlay -->
<div id="rotate-hint" onclick="enterImmersive()">
  <svg width="48" height="48" viewBox="0 0 48 48" fill="none">
    <rect x="10" y="4" width="28" height="40" rx="4" stroke="#445566" stroke-width="2"/>
    <path d="M40 22 C40 14 34 8 24 8" stroke="#00d4aa" stroke-width="2" stroke-linecap="round"/>
    <polyline points="20,4 24,8 28,4" fill="none" stroke="#00d4aa" stroke-width="2" stroke-linejoin="round"/>
  </svg>
  <p>请横向握持设备</p>
  <small>点击进入全屏 · TAP TO FULLSCREEN</small>
</div>

<!-- Top bar -->
<div id="topbar">
  <div id="tb-time">--:--</div>

  <div id="tb-center">
    <div id="brake-dot" title="Brake"></div>
    <!-- Charging bolt: shown when bmsA < 0 -->
    <svg id="chg-icon" width="10" height="18" viewBox="0 0 10 18" fill="none" title="Charging">
      <path d="M6 1L1 10h4l-1 7 5-9H5L6 1z" fill="#3ad4ff" stroke="none"/>
    </svg>
  </div>

  <div id="tb-right">
    <span id="net-warn">
      <svg width="14" height="12" viewBox="0 0 14 12" fill="none" style="vertical-align:middle;margin-right:2px">
        <path d="M1 3.5C3.5 1.2 6 0 7 0s3.5 1.2 6 3.5" stroke="currentColor" stroke-width="1.4" stroke-linecap="round"/>
        <path d="M3 6C4.6 4.5 5.8 4 7 4s2.4.5 4 2" stroke="currentColor" stroke-width="1.4" stroke-linecap="round"/>
        <path d="M5.5 8.5C6.1 8 6.6 7.5 7 7.5s.9.5 1.5 1" stroke="currentColor" stroke-width="1.4" stroke-linecap="round"/>
        <line x1="1" y1="1" x2="13" y2="11" stroke="currentColor" stroke-width="1.3" stroke-linecap="round"/>
      </svg>
    </span>
    <span id="can-warn">
      <svg width="14" height="14" viewBox="0 0 14 14" fill="none" style="vertical-align:middle;margin-right:2px">
        <path d="M3 7h8M3 4h3M8 4h3M3 10h3M8 10h3" stroke="currentColor" stroke-width="1.3" stroke-linecap="round"/>
        <rect x="1" y="1" width="12" height="12" rx="2" stroke="currentColor" stroke-width="1.3"/>
        <line x1="1" y1="1" x2="13" y2="13" stroke="currentColor" stroke-width="1.3" stroke-linecap="round"/>
      </svg>
    </span>
    <!-- Safety warnings — highest priority, always leftmost -->
    <span id="fcw-warn" title="Forward Collision Warning">
      <svg width="18" height="16" viewBox="0 0 18 16" fill="none">
        <path d="M9 1L1 15h16L9 1z" stroke="currentColor" stroke-width="1.5" stroke-linejoin="round"/>
        <line x1="9" y1="6" x2="9" y2="10" stroke="currentColor" stroke-width="1.6" stroke-linecap="round"/>
        <circle cx="9" cy="12.5" r=".9" fill="currentColor"/>
      </svg>
    </span>
    <!-- Blind spot detection arrows -->
    <span class="bsd" id="bsd-left" title="Blind spot left">
      <svg width="14" height="12" viewBox="0 0 14 12" fill="none">
        <path d="M8 1L2 6l6 5" stroke="currentColor" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round"/>
        <path d="M13 1L7 6l6 5" stroke="currentColor" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round" opacity=".5"/>
      </svg>
    </span>
    <span class="bsd" id="bsd-right" title="Blind spot right">
      <svg width="14" height="12" viewBox="0 0 14 12" fill="none">
        <path d="M6 1l6 5-6 5" stroke="currentColor" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round"/>
        <path d="M1 1l6 5-6 5" stroke="currentColor" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round" opacity=".5"/>
      </svg>
    </span>
    <!-- Lane departure warning icon -->
    <span id="ldw-warn" title="Lane departure">
      <svg width="16" height="14" viewBox="0 0 16 14" fill="none">
        <path d="M2 13L8 2l6 11" stroke="currentColor" stroke-width="1.4" stroke-linecap="round" stroke-linejoin="round" fill="none"/>
        <path d="M8 5v4" stroke="currentColor" stroke-width="1.5" stroke-linecap="round"/>
        <path d="M5 13v-5M11 13v-5" stroke="currentColor" stroke-width="1.3" stroke-linecap="round" stroke-dasharray="2 2"/>
      </svg>
    </span>
    <!-- Temperature -->
    <div id="tb-temp">
      <div class="temp-cell" id="temp-out">
        <svg width="10" height="18" viewBox="0 0 10 18" fill="none">
          <path d="M3 11 Q3 2 5 2 Q7 2 7 11" stroke="#5a9fd4" stroke-width="1.6" stroke-linecap="round"/>
          <circle cx="5" cy="14.5" r="2.8" fill="#5a9fd4"/>
          <rect x="4.1" y="7" width="1.8" height="6" rx=".9" fill="#5a9fd4"/>
        </svg>
        <span class="temp-num" id="tb-tout">--</span><span class="temp-unit">°C</span>
      </div>
      <div class="temp-sep"></div>
      <div class="temp-cell" id="temp-in">
        <svg width="10" height="18" viewBox="0 0 10 18" fill="none">
          <path d="M3 11 Q3 2 5 2 Q7 2 7 11" stroke="#d47a3a" stroke-width="1.6" stroke-linecap="round"/>
          <circle cx="5" cy="14.5" r="2.8" fill="#d47a3a"/>
          <rect x="4.1" y="7" width="1.8" height="6" rx=".9" fill="#d47a3a"/>
        </svg>
        <span class="temp-num" id="tb-tin">--</span><span class="temp-unit">°C</span>
      </div>
    </div>
    <!-- AP / FSD -->
    <div class="fsd-badge" id="ap-badge" title="Autopilot">
      <svg width="16" height="16" viewBox="0 0 16 16" fill="none">
        <circle cx="8" cy="8" r="7" stroke="currentColor" stroke-width="1.4"/>
        <circle cx="8" cy="8" r="2.2" stroke="currentColor" stroke-width="1.3"/>
        <line x1="8" y1="1" x2="8" y2="5.8" stroke="currentColor" stroke-width="1.3"/>
        <line x1="8" y1="10.2" x2="8" y2="15" stroke="currentColor" stroke-width="1.3"/>
        <line x1="1" y1="8" x2="5.8" y2="8" stroke="currentColor" stroke-width="1.3"/>
        <line x1="10.2" y1="8" x2="15" y2="8" stroke="currentColor" stroke-width="1.3"/>
      </svg>
    </div>
    <div class="fsd-badge" id="fsd-badge" title="FSD">
      <svg width="16" height="16" viewBox="0 0 16 16" fill="none">
        <circle cx="8" cy="8" r="7" stroke="currentColor" stroke-width="1.4"/>
        <circle cx="8" cy="8" r="2.2" stroke="currentColor" stroke-width="1.3"/>
        <line x1="8" y1="1" x2="8" y2="5.8" stroke="currentColor" stroke-width="1.3"/>
        <line x1="8" y1="10.2" x2="8" y2="15" stroke="currentColor" stroke-width="1.3"/>
        <line x1="1" y1="8" x2="5.8" y2="8" stroke="currentColor" stroke-width="1.3"/>
        <line x1="10.2" y1="8" x2="15" y2="8" stroke="currentColor" stroke-width="1.3"/>
        <circle cx="8" cy="8" r=".9" fill="currentColor"/>
      </svg>
    </div>
    <button id="fs-btn" onclick="toggleFS()" title="Fullscreen">⛶</button>
  </div>
</div>

<!-- Panels -->
<div id="panels">

  <!-- LEFT: single large gear letter + power -->
  <div class="panel" id="left-panel">
    <!-- Single gear letter — only the active gear is shown, massive size -->
    <div id="gear-big" class="gear-big sel-none">--</div>
    <!-- Instantaneous power (from CAN BMS) -->
    <div id="lp-power">
      <svg width="14" height="18" viewBox="0 0 14 18" fill="none" style="flex-shrink:0">
        <path d="M8.5 0L1 11h5l-1.5 7 8-12H7.5L8.5 0z" fill="#00d4aa"/>
      </svg>
      <span id="lp-pow-num">0</span>
      <span style="font-size:9px;color:#2e3f50;letter-spacing:1px">kW</span>
    </div>
  </div>

  <div class="vdiv"></div>

  <!-- CENTER: Speed gauge -->
  <div class="panel" id="center-panel">
    <!-- Performance timer -->
    <div id="perf-row"></div>
    <!-- Nag level dots -->
    <div id="nag-row">
      <div class="nag-dot" id="nd0"></div>
      <div class="nag-dot" id="nd1"></div>
      <div class="nag-dot" id="nd2"></div>
      <div class="nag-dot" id="nd3"></div>
      <div class="nag-dot" id="nd4"></div>
    </div>
    <div class="spd-wrap">
      <svg viewBox="0 0 500 500" style="width:100%">
        <defs>
          <linearGradient id="spdGrad" x1="115.6" y1="0" x2="384.4" y2="0" gradientUnits="userSpaceOnUse">
            <stop offset="0%"   stop-color="#00d4aa"/>
            <stop offset="38%"  stop-color="#44dd88"/>
            <stop offset="72%"  stop-color="#ff8800"/>
            <stop offset="100%" stop-color="#ff2244"/>
          </linearGradient>
          <filter id="glow5">
            <feGaussianBlur in="SourceGraphic" stdDeviation="5" result="b"/>
            <feMerge><feMergeNode in="b"/><feMergeNode in="SourceGraphic"/></feMerge>
          </filter>
        </defs>
        <circle cx="250" cy="250" r="200" fill="none" stroke="rgba(0,212,170,0.03)" stroke-width="1.5"/>
        <circle cx="250" cy="250" r="178" fill="none" stroke="rgba(255,255,255,0.01)" stroke-width="1"/>
        <!-- Background track -->
        <path id="spd-bg-arc" d="M 115.6 384.4 A 190 190 0 1 1 384.4 384.4"
              fill="none" stroke="#0a1018" stroke-width="20" stroke-linecap="round"/>
        <!-- Tick marks -->
        <g id="spd-ticks"></g>
        <!-- Speed fill arc -->
        <path id="spd-arc" d="M 115.6 384.4 A 190 190 0 1 1 384.4 384.4"
              fill="none" stroke="url(#spdGrad)" stroke-width="16" stroke-linecap="round"
              stroke-dasharray="0 895" filter="url(#glow5)"/>
        <!-- Speed number -->
        <text id="spd-txt" x="250" y="290" text-anchor="middle"
              font-family="-apple-system,'Helvetica Neue',Arial,sans-serif"
              font-size="140" font-weight="800" fill="#ffffff" letter-spacing="-6">0</text>
        <text x="250" y="326" text-anchor="middle"
              font-family="-apple-system,'Helvetica Neue',Arial,sans-serif"
              font-size="17" fill="#2e4050" letter-spacing="3">km/h</text>
        <!-- Scale endpoints -->
        <text x="100" y="400" text-anchor="middle" font-family="Arial" font-size="15" fill="#1a2a3a">0</text>
        <text x="400" y="400" text-anchor="middle" font-family="Arial" font-size="15" fill="#1a2a3a">200</text>
        <text x="250" y="54"  text-anchor="middle" font-family="Arial" font-size="15" fill="#1a2a3a">100</text>
        <!-- Speed limit sign in arc gap -->
        <g id="spd-limit-g" visibility="hidden">
          <circle cx="250" cy="445" r="40" fill="white" stroke="#e01020" stroke-width="9"/>
          <text id="spd-limit-txt" x="250" y="457"
                text-anchor="middle"
                font-family="-apple-system,'Helvetica Neue',Arial,sans-serif"
                font-size="28" font-weight="900" fill="#111111">--</text>
        </g>
      </svg>
    </div>
  </div>

  <div class="vdiv"></div>

  <!-- RIGHT: Stacked cards -->
  <div id="right-panel">

    <!-- ① Battery card — shown when bmsSeen -->
    <div class="rcard" id="rc-bat" style="display:none;flex-direction:column;align-items:center">
      <!-- Compact battery arc -->
      <div class="bat-arc-wrap">
        <svg viewBox="0 0 180 160" style="width:100%">
          <defs>
            <linearGradient id="batGrad2" x1="26" y1="0" x2="154" y2="0" gradientUnits="userSpaceOnUse">
              <stop offset="0%"   stop-color="#ff2244"/>
              <stop offset="22%"  stop-color="#ff8800"/>
              <stop offset="60%"  stop-color="#44dd88"/>
              <stop offset="100%" stop-color="#00d4aa"/>
            </linearGradient>
            <filter id="glow3b">
              <feGaussianBlur in="SourceGraphic" stdDeviation="3" result="b"/>
              <feMerge><feMergeNode in="b"/><feMergeNode in="SourceGraphic"/></feMerge>
            </filter>
          </defs>
          <!-- Battery arc: cx=90,cy=90,r=72, 270° -->
          <!-- Arc endpoints: x=90+72*cos(135°)=90-50.9=39.1, y=90+72*sin(135°)=90+50.9=140.9 -->
          <!--                x=90+72*cos(45°) =90+50.9=140.9, y=90+72*sin(45°) =90+50.9=140.9 -->
          <path id="bat-bg-arc" d="M 39.1 140.9 A 72 72 0 1 1 140.9 140.9"
                fill="none" stroke="#0b111c" stroke-width="10" stroke-linecap="round"/>
          <path id="bat-arc2" d="M 39.1 140.9 A 72 72 0 1 1 140.9 140.9"
                fill="none" stroke="url(#batGrad2)" stroke-width="8" stroke-linecap="round"
                stroke-dasharray="0 339" filter="url(#glow3b)"/>
          <!-- SOC text in center -->
          <text id="bat-soc-txt" x="90" y="105"
                text-anchor="middle"
                font-family="-apple-system,'Helvetica Neue',Arial,sans-serif"
                font-size="46" font-weight="800" fill="#00d4aa" letter-spacing="-2">--%</text>
          <!-- V below SOC -->
          <text id="bat-v-txt" x="90" y="128"
                text-anchor="middle"
                font-family="Arial" font-size="13" fill="#2e4050">--- V</text>
          <!-- Scale labels -->
          <text x="26" y="154" font-family="Arial" font-size="11" fill="#1a2a3a" text-anchor="middle">0</text>
          <text x="154" y="154" font-family="Arial" font-size="11" fill="#1a2a3a" text-anchor="middle">100</text>
        </svg>
      </div>
      <!-- Power / current row -->
      <div class="bat-row">
        <div class="bat-cell">
          <span class="bat-cell-val" id="bat-kw">--</span>
          <!-- bolt = kW -->
          <svg width="7" height="11" viewBox="0 0 7 11" fill="none" class="bat-cell-lbl" style="display:block;margin:1px auto 0;opacity:.45">
            <path d="M4.5 0L0 6.5h2.5L1.5 11 7 4.5H4.5L4.5 0z" fill="currentColor"/>
          </svg>
        </div>
        <div class="bat-cell">
          <span class="bat-cell-val" id="bat-a2">--</span>
          <!-- current = A -->
          <svg width="11" height="11" viewBox="0 0 11 11" fill="none" class="bat-cell-lbl" style="display:block;margin:1px auto 0;opacity:.45">
            <circle cx="5.5" cy="5.5" r="4.5" stroke="currentColor" stroke-width="1.3"/>
            <path d="M3.5 5.5h4M5.5 3.5v4" stroke="currentColor" stroke-width="1.2" stroke-linecap="round"/>
          </svg>
        </div>
        <div class="bat-cell">
          <span class="bat-cell-val" id="bat-t2">--</span>
          <!-- thermometer = °C -->
          <svg width="8" height="12" viewBox="0 0 8 12" fill="none" class="bat-cell-lbl" style="display:block;margin:1px auto 0;opacity:.45">
            <path d="M2.5 7.5V2.5Q2.5 1 4 1Q5.5 1 5.5 2.5V7.5" stroke="currentColor" stroke-width="1.2" stroke-linecap="round"/>
            <circle cx="4" cy="9.2" r="2.3" stroke="currentColor" stroke-width="1.2"/>
          </svg>
        </div>
      </div>
    </div>

    <!-- ② Speed limits card — always visible -->
    <div class="rcard" id="rc-lim">
      <!-- Fused limit (camera+map) -->
      <div class="lim-sign">
        <div class="lim-circle fused sna" id="lim-fused">--</div>
        <!-- Camera + map-pin icon = fused (camera AND map) -->
        <svg width="20" height="13" viewBox="0 0 20 13" fill="none" class="lim-icon-lbl" style="display:block;opacity:.4;margin-top:2px">
          <rect x="1" y="3" width="10" height="8" rx="1.5" stroke="currentColor" stroke-width="1.3"/>
          <circle cx="6" cy="7" r="2.2" stroke="currentColor" stroke-width="1.2"/>
          <path d="M11 5.5l5-2.5v7l-5-2.5V5.5z" stroke="currentColor" stroke-width="1.1" stroke-linejoin="round"/>
          <circle cx="17.5" cy="1.5" r="1.3" fill="currentColor"/>
          <line x1="17.5" y1="2.8" x2="17.5" y2="4.5" stroke="currentColor" stroke-width="1.1" stroke-linecap="round"/>
        </svg>
      </div>
      <div class="lim-divider"></div>
      <!-- Vision limit (camera only) -->
      <div class="lim-sign">
        <div class="lim-circle vision sna" id="lim-vision">--</div>
        <!-- Camera-only icon = vision only -->
        <svg width="18" height="13" viewBox="0 0 18 13" fill="none" class="lim-icon-lbl" style="display:block;opacity:.4;margin-top:2px">
          <rect x="1" y="3" width="10" height="8" rx="1.5" stroke="currentColor" stroke-width="1.3"/>
          <circle cx="6" cy="7" r="2.2" stroke="currentColor" stroke-width="1.2"/>
          <path d="M11 5.5l5-2.5v7l-5-2.5V5.5z" stroke="currentColor" stroke-width="1.1" stroke-linejoin="round"/>
        </svg>
      </div>
    </div>

  </div><!-- /right-panel -->

</div><!-- /panels -->

<script>
var tok = sessionStorage.getItem('fsd_tok') || '';

// Arc constants
var SA = 895;   // speed arc total len
var GA = 339;   // battery arc total len (r=72, 270°: 2π×72×0.75 ≈ 339)
var MAX_SPD = 200;


// Lerp state
var tSpd=0, cSpd=0;
var tBat=0, cBat=0;

var K = 0.18;
var pollFails = 0;

function lerp(a,b,k){return a+(b-a)*k}
function clamp(v,lo,hi){return v<lo?lo:v>hi?hi:v}


function updateNag(level) {
  var row = document.getElementById('nag-row');
  if (level === 0) { row.style.display='none'; return; }
  row.style.display='flex';
  var filled = Math.min(5, Math.round(level/3)+1);
  var col = level<=5?'#ffcc00':level<=10?'#ff8800':'#ff3344';
  for (var i=0;i<5;i++)
    document.getElementById('nd'+i).style.background = i<filled?col:'#1a2530';
}

// Speed tick marks
(function buildTicks(){
  var g=document.getElementById('spd-ticks');
  var cx=250,cy=250,r=190,ns='http://www.w3.org/2000/svg';
  for(var i=0;i<=10;i++){
    var deg=135+27*i, rad=deg*Math.PI/180;
    var cos=Math.cos(rad),sin=Math.sin(rad);
    var major=(i%2===0);
    var r1=r-(major?24:14),r2=r+4;
    var ln=document.createElementNS(ns,'line');
    ln.setAttribute('x1',(cx+r1*cos).toFixed(2));
    ln.setAttribute('y1',(cy+r1*sin).toFixed(2));
    ln.setAttribute('x2',(cx+r2*cos).toFixed(2));
    ln.setAttribute('y2',(cy+r2*sin).toFixed(2));
    ln.setAttribute('stroke',major?'#2a4560':'#1e3040');
    ln.setAttribute('stroke-width',major?'2.5':'1.5');
    g.appendChild(ln);
  }
})();

function updateClock(){
  var n=new Date();
  document.getElementById('tb-time').textContent=
    ('0'+n.getHours()).slice(-2)+':'+('0'+n.getMinutes()).slice(-2);
  // Time-based theme: day 06:00–18:00 light, otherwise dark
  document.body.classList.toggle('day', n.getHours() >= 6 && n.getHours() < 18);
}

var GEARS={1:'P',2:'R',3:'N',4:'D'};
var _lastGear=0;
function setGear(g){
  var el=document.getElementById('gear-big');
  if(!el)return;
  var label=GEARS[g]||null;
  if(label){
    el.textContent=label;
    el.className='gear-big sel-'+label;
  } else {
    el.textContent='--';
    el.className='gear-big sel-none';
  }
  if(g!==_lastGear&&label){
    el.classList.add('gear-anim');
    el.addEventListener('animationend',function f(){el.classList.remove('gear-anim');el.removeEventListener('animationend',f);});
  }
  _lastGear=g;
}

// Helper: show card with fade-in animation
function showCard(el){
  if(!el)return;
  el.style.display='flex';
  requestAnimationFrame(function(){
    el.classList.add('visible');
    el.addEventListener('animationend',function f(){el.classList.remove('visible');el.removeEventListener('animationend',f);},{once:true});
  });
}
function hideCard(el){if(el)el.style.display='none';}

// Animation loop
function frame(){
  requestAnimationFrame(frame);
  cSpd=lerp(cSpd,tSpd,K);
  cBat=lerp(cBat,tBat,K);
  // Speed arc
  var sf=clamp(cSpd/MAX_SPD,0,1)*SA;
  document.getElementById('spd-arc').setAttribute('stroke-dasharray',sf.toFixed(1)+' '+(SA-sf).toFixed(1));
  document.getElementById('spd-txt').textContent=Math.round(cSpd);

  // Battery arc
  var bf=clamp(cBat/100,0,1)*GA;
  document.getElementById('bat-arc2').setAttribute('stroke-dasharray',bf.toFixed(1)+' '+(GA-bf).toFixed(1));

}

function poll(){
  fetch('/api/status'+(tok?'?token='+tok:''))
    .then(function(r){
      if(r.status===403){location.href='/';return null;}
      return r.json();
    })
    .then(function(d){
      if(!d) return;

      // CAN status badge
      document.getElementById('can-warn').style.display = !d.canOK ? 'inline-block' : 'none';

      // Speed + gear from CAN
      tSpd = (d.speedD||0)/10;
      setGear(d.gearRaw);


      // ── Left panel: power kW ─────────────────────────────────
      var lpPow = document.getElementById('lp-power');
      var lpPowNum = document.getElementById('lp-pow-num');
      var powerKw = 0;
      if (d.bmsSeen) {
        powerKw = ((d.bmsV||0)/100) * ((d.bmsA||0)/10) / 1000;
      }
      if (d.bmsSeen) {
        lpPow.style.display = 'flex';
        lpPowNum.textContent = (powerKw>=0?'+':'')+powerKw.toFixed(1);
        lpPowNum.className = powerKw>1?'drive':powerKw<-1?'regen':'';
        // Update bolt icon color in power div
        var boltPath=lpPow.querySelector('path');
        if(boltPath)boltPath.setAttribute('fill',powerKw<-1?'#3ad4ff':powerKw>1?'#00d4aa':'#2e3f50');
      } else {
        lpPow.style.display = 'none';
      }

      // ── Battery card — CAN BMS ────────────────────────────────
      var rcBat = document.getElementById('rc-bat');
      if (d.bmsSeen) {
        if(rcBat.style.display==='none'||rcBat.style.display==='')showCard(rcBat);
        var v  = (d.bmsV||0)/100;
        var a  = (d.bmsA||0)/10;
        var kw = (v*a/1000);
        var soc = (d.bmsSoc||0)/10;
        tBat = soc;
        var socTxt = document.getElementById('bat-soc-txt');
        socTxt.textContent = soc.toFixed(1)+'%';
        socTxt.setAttribute('fill', soc<15?'#ff3344':soc<30?'#ff8800':'#00d4aa');
        document.getElementById('bat-v-txt').textContent = v.toFixed(1)+' V';
        var kwEl = document.getElementById('bat-kw');
        kwEl.textContent = (kw>=0?'+':'')+kw.toFixed(1);
        kwEl.className = 'bat-cell-val'+(a<0?' chg':'');
        document.getElementById('bat-a2').textContent = a.toFixed(1);
        var avgT = ((d.bmsMinT||0)+(d.bmsMaxT||0))/2;
        document.getElementById('bat-t2').textContent = avgT.toFixed(0);
        var isChg = (a < -2);
        document.getElementById('chg-icon').style.display = isChg?'block':'none';
        rcBat.className = 'rcard'+(isChg?' charging':'');
      } else {
        hideCard(rcBat);
        tBat = 0;
        document.getElementById('chg-icon').style.display = 'none';
      }

      // ── Speed limits card ─────────────────────────────────────
      var fl = d.fusedLimit||0, flOk = fl>0&&fl<31;
      var vl = d.visionLimit||0, vlOk = vl>0&&vl<31;
      var fEl = document.getElementById('lim-fused');
      var vEl = document.getElementById('lim-vision');
      fEl.textContent = flOk ? fl*5 : '--';
      fEl.className   = 'lim-circle fused'+(flOk?'':' sna');
      vEl.textContent = vlOk ? vl*5 : '--';
      vEl.className   = 'lim-circle vision'+(vlOk?'':' sna');

      // Speed limit sign in arc gap (vision limit)
      var sg = document.getElementById('spd-limit-g');
      if (vlOk) {
        document.getElementById('spd-limit-txt').textContent = vl*5;
        sg.setAttribute('visibility','visible');
      } else if (flOk) {
        document.getElementById('spd-limit-txt').textContent = fl*5;
        sg.setAttribute('visibility','visible');
      } else {
        sg.setAttribute('visibility','hidden');
      }

      // ── 0-100 Performance timer ───────────────────────────────
      var prEl = document.getElementById('perf-row');
      var pa = d.perfAccel||0;
      if (pa === 0) {
        prEl.style.display = 'none';
      } else if (pa === 1) {
        prEl.style.display = 'block';
        prEl.className = 'armed';
        prEl.textContent = '0 → 100  准备中';
      } else if (pa === 2) {
        prEl.style.display = 'block';
        prEl.className = 'running';
        prEl.textContent = '0 → 100  计时中…';
      } else {
        prEl.style.display = 'block';
        prEl.className = 'done';
        prEl.textContent = '0 → 100  '+((d.perfAccelMs||0)/1000).toFixed(2)+'s';
      }

      // ── Temperature — CAN (tempSeen) ─────────────────────────
      if (d.canOK && d.tempSeen) {
        var tOut = (d.tempOutRaw*0.5-40).toFixed(1);
        var tIn  = (d.tempInRaw*0.25).toFixed(1);
        document.getElementById('tb-tout').textContent = tOut;
        document.getElementById('tb-tin').textContent  = tIn;
        var outC = parseFloat(tOut);
        var inC  = parseFloat(tIn);
        var ocol = outC<5?'#44aaff':outC<20?'#5a9fd4':outC<30?'#ffaa44':'#ff5522';
        var icol = inC<18?'#5a9fd4':inC<25?'#d47a3a':'#ff5522';
        document.getElementById('temp-out').querySelectorAll('circle,rect,path').forEach(function(el){
          if(el.tagName==='path')el.setAttribute('stroke',ocol);else el.setAttribute('fill',ocol);
        });
        document.getElementById('temp-in').querySelectorAll('circle,rect,path').forEach(function(el){
          if(el.tagName==='path')el.setAttribute('stroke',icol);else el.setAttribute('fill',icol);
        });
      } else {
        document.getElementById('tb-tout').textContent='--';
        document.getElementById('tb-tin').textContent='--';
      }

      // ── AP / FSD badges ───────────────────────────────────────
      var apOn  = d.accState>0;
      var fsdOn = d.fsdTriggered&&!!d.fsdEnable&&!apOn;
      document.getElementById('ap-badge').className  = 'fsd-badge'+(apOn?' on':'');
      document.getElementById('fsd-badge').className = 'fsd-badge'+(fsdOn?' on':'');
      document.getElementById('ap-badge').style.display  = (apOn||!fsdOn)?'':'none';
      document.getElementById('fsd-badge').style.display = (!apOn)?'':'none';

      // ── Safety warnings ───────────────────────────────────────
      document.getElementById('fcw-warn').style.display   = d.fcw>0?'inline-block':'none';
      document.getElementById('bsd-left').className  = 'bsd'+((d.sideCol&1)?' on':'');
      document.getElementById('bsd-right').className = 'bsd'+((d.sideCol&2)?' on':'');
      document.getElementById('ldw-warn').style.display   = d.laneWarn>0?'inline-block':'none';
      document.getElementById('brake-dot').className = d.brake?'brake-dot on':'brake-dot';

      // ── Nag ───────────────────────────────────────────────────
      updateNag(d.nagLevel||0);

      pollFails=0;
      document.getElementById('net-warn').style.display='none';
      updateClock();
    })
    .catch(function(){
      pollFails++;
      if(pollFails>=3) document.getElementById('net-warn').style.display='inline-block';
    });
}

// Fullscreen
function lockLandscape(){
  try{if(screen.orientation&&screen.orientation.lock)screen.orientation.lock('landscape').catch(function(){});}catch(e){}
}
function enterImmersive(){
  var el=document.documentElement;
  var req=el.requestFullscreen||el.webkitRequestFullscreen||el.mozRequestFullScreen;
  if(req){req.call(el).then(function(){lockLandscape();}).catch(function(){});}
  else{document.body.classList.add('no-fs');lockLandscape();}
}
function toggleFS(){
  if(!document.fullscreenElement&&!document.webkitFullscreenElement)enterImmersive();
  else{var ex=document.exitFullscreen||document.webkitExitFullscreen||document.mozCancelFullScreen;if(ex)ex.call(document);}
}
document.addEventListener('fullscreenchange',function(){document.getElementById('fs-btn').textContent=document.fullscreenElement?'✕':'⛶';});
document.addEventListener('webkitfullscreenchange',function(){document.getElementById('fs-btn').textContent=document.webkitFullscreenElement?'✕':'⛶';});
document.addEventListener('click',function autoFs(){document.removeEventListener('click',autoFs);if(!document.fullscreenElement&&!document.webkitFullscreenElement)enterImmersive();});

// Wake lock — try native API first (requires HTTPS), fall back to canvas stream trick
var _wakeLock=null;
function acquireWakeLock(){
  if(navigator.wakeLock){
    navigator.wakeLock.request('screen').then(function(wl){
      _wakeLock=wl;
      wl.addEventListener('release',function(){_wakeLock=null;});
    }).catch(function(){});
  }
}
document.addEventListener('visibilitychange',function(){
  if(document.visibilityState==='visible'){acquireWakeLock();if(_noSleepVid)_noSleepVid.play().catch(function(){});}
});

// Canvas-stream no-sleep: works on HTTP where Wake Lock API is blocked
var _noSleepVid=null;
function setupNoSleep(){
  if(_noSleepVid)return;
  try{
    var canvas=document.createElement('canvas');
    canvas.width=canvas.height=1;
    var ctx=canvas.getContext('2d');
    var stream=canvas.captureStream(1);
    _noSleepVid=document.createElement('video');
    _noSleepVid.setAttribute('muted','');
    _noSleepVid.setAttribute('playsinline','');
    _noSleepVid.style.cssText='position:fixed;top:0;left:0;width:1px;height:1px;opacity:0;pointer-events:none;z-index:-1';
    _noSleepVid.srcObject=stream;
    document.body.appendChild(_noSleepVid);
    setInterval(function(){ctx.fillStyle='rgba(0,0,0,'+Math.random()*0.002+')';ctx.fillRect(0,0,1,1);},500);
    _noSleepVid.play().catch(function(){});
  }catch(e){}
  acquireWakeLock();
}
// Start no-sleep on first user interaction (required by browsers)
document.addEventListener('click',function nsInit(){setupNoSleep();document.removeEventListener('click',nsInit);},{once:true,capture:true});

setInterval(poll,1000);
setInterval(updateClock,15000);
updateClock();
requestAnimationFrame(frame);
poll();
</script>
</body>
</html>
)dash";
