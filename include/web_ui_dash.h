#pragma once
// ── Dashboard page — /dash ────────────────────────────────────────────────────
// Instrument cluster UI served from PROGMEM.
// Polls /api/status every 1 s; animates gauges at 60 fps with lerp.
// Auth: reads sessionStorage 'fsd_tok'; redirects to / on 403.
//
// Layout (landscape-first, flex row):
//   [POWER 24%] | [SPEED 52%] | [BATTERY 24%]
//
// SVG arc geometry (verified):
//   Speed   ViewBox 500×500, r=190, 270° arc:  M 115.6 384.4 A 190 190 0 1 1 384.4 384.4  len≈895
//   Side    ViewBox 260×260, r=108, 270° arc:  M 53.6  206.4 A 108 108 0 1 1 206.4 206.4  len≈509
//   Both arcs span 135°→45° SVG-clockwise (reflecting bottom-left→top→bottom-right).
//   Horizontal linearGradient works perfectly: both arc endpoints share identical Y coordinate.
//
// Power gauge — bidirectional:
//   Discharge (bmsA > 0): orange arc fills from lower-left clockwise         → teal/orange
//   Regen     (bmsA < 0): cyan  arc fills from lower-right counterclockwise  → cyan
//   Mirror transform: scale(-1,1) translate(-260,0)  maps x → (260−x)

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
  height:50px;
  padding-top:env(safe-area-inset-top);
  padding-left:max(18px, env(safe-area-inset-left));
  padding-right:max(18px, env(safe-area-inset-right));
  background:rgba(255,255,255,0.025);
  border-bottom:1px solid rgba(255,255,255,0.05);
}
#tb-time{
  flex:1;
  font-size:13px;font-weight:300;letter-spacing:2px;
  color:#6677aa;
}
#tb-gear{
  font-size:34px;font-weight:800;letter-spacing:5px;
  transition:color 0.3s,text-shadow 0.3s;
}
#tb-gear.P{color:#8899cc;text-shadow:0 0 14px rgba(136,153,204,0.4)}
#tb-gear.R{color:#ff3355;text-shadow:0 0 18px rgba(255,51,85,0.5)}
#tb-gear.N{color:#4d6070;text-shadow:none}
#tb-gear.D{color:#00d4aa;text-shadow:0 0 18px rgba(0,212,170,0.5)}
#tb-right{
  flex:1;
  display:flex;align-items:center;gap:10px;
  justify-content:flex-end;
}
#tb-temp{font-size:11px;color:#44556a;white-space:nowrap}
#can-warn,#net-warn{
  display:none;
  font-size:10px;font-weight:700;letter-spacing:1.5px;
  padding:3px 8px;border-radius:3px;white-space:nowrap;
  background:rgba(255,40,40,0.15);color:#ff4444;
  border:1px solid rgba(255,40,40,0.4);
  animation:blink 1s step-start infinite;
}
@keyframes blink{50%{opacity:0.3}}
/* Unknown gear */
#tb-gear.unknown{color:#334455;text-shadow:none}
#fs-btn{
  background:none;border:1px solid rgba(255,255,255,0.1);
  border-radius:6px;color:#445566;cursor:pointer;
  font-size:14px;line-height:1;padding:4px 7px;
  transition:color 0.2s,border-color 0.2s;flex-shrink:0;
}
#fs-btn:hover{color:#00d4aa;border-color:#00d4aa55}
/* hide fullscreen btn on iOS (API not supported) */
@supports not (will-change: transform) {}
.no-fs #fs-btn{display:none}
.fsd-badge{
  font-size:9px;font-weight:700;letter-spacing:1.5px;
  padding:3px 7px;border-radius:3px;
  background:rgba(255,255,255,0.05);color:#445566;
  border:1px solid rgba(255,255,255,0.07);
  transition:all 0.3s;white-space:nowrap;
}
.fsd-badge.on{
  background:rgba(0,212,170,0.12);color:#00d4aa;
  border-color:rgba(0,212,170,0.35);
  box-shadow:0 0 10px rgba(0,212,170,0.2);
}
/* ── AP / FCW / Brake badges ───────────────────────────────── */
#fcw-warn{
  display:none;
  font-size:10px;font-weight:700;letter-spacing:1px;
  padding:3px 8px;border-radius:3px;white-space:nowrap;
  background:rgba(255,170,0,0.15);color:#ffaa00;
  border:1px solid rgba(255,170,0,0.45);
  animation:blink 0.6s step-start infinite;
}
#ap-badge{cursor:default}
#ap-badge.on{
  background:rgba(0,212,170,0.12);color:#00d4aa;
  border-color:rgba(0,212,170,0.35);
}
#brake-dot{
  width:8px;height:8px;border-radius:50%;flex-shrink:0;
  background:#1a2530;border:1px solid rgba(255,255,255,0.06);
  transition:background 0.15s,box-shadow 0.15s;
}
#brake-dot.on{
  background:#ff3344;
  box-shadow:0 0 6px rgba(255,51,68,0.7);
}
/* ── Blind spot / lane warning indicators ──────────────────── */
.bsd{
  display:none;
  font-size:10px;font-weight:700;letter-spacing:1px;
  padding:3px 7px;border-radius:3px;white-space:nowrap;
  background:rgba(255,170,0,0.15);color:#ffaa00;
  border:1px solid rgba(255,170,0,0.45);
}
.bsd.on{display:inline-block}
#ldw-warn{
  display:none;
  font-size:10px;font-weight:700;letter-spacing:1px;
  padding:3px 7px;border-radius:3px;white-space:nowrap;
  background:rgba(255,100,0,0.15);color:#ff7700;
  border:1px solid rgba(255,100,0,0.4);
}
/* ── Nag indicator in left panel ───────────────────────────── */
#nag-row{
  display:none;
  justify-content:center;align-items:center;gap:4px;
  margin-top:6px;
}
.nag-dot{
  width:6px;height:6px;border-radius:50%;
  background:#1a2530;border:1px solid rgba(255,255,255,0.08);
  transition:background 0.3s;
}

/* ── Main layout ───────────────────────────────────────────── */
#panels{
  display:flex;align-items:center;
  width:100%;height:calc(100vh - 50px);
  padding-top:8px;
  padding-bottom:max(10px, env(safe-area-inset-bottom));
  padding-left:max(6px, env(safe-area-inset-left));
  padding-right:max(6px, env(safe-area-inset-right));
}
.panel{
  display:flex;flex-direction:column;align-items:center;
  justify-content:center;height:100%;
}
#left-panel{flex:0 0 24%}
#center-panel{flex:1}
#right-panel{flex:0 0 24%}
.vdiv{width:1px;height:55%;background:rgba(255,255,255,0.04);flex-shrink:0}

/* ── Common gauge elements ─────────────────────────────────── */
.glabel{
  font-size:9px;letter-spacing:2.5px;text-transform:uppercase;
  color:#2e3f50;margin-bottom:4px;font-weight:500;
  text-align:center;line-height:1.4;
}
.glabel span{
  display:block;font-size:9px;letter-spacing:1.5px;
  color:#1e2e3c;text-transform:none;font-weight:400;margin-top:1px;
}
.gnum{font-size:30px;font-weight:700;letter-spacing:-.5px;transition:color 0.3s}
.gunit{font-size:10px;color:#3a4f5f;letter-spacing:1.5px;margin-top:1px}
.gsub{
  display:flex;flex-wrap:wrap;justify-content:center;gap:6px;
  margin-top:12px;align-items:baseline;
  font-size:12px;color:#4a6070;
}
.gsub b{font-size:15px;font-weight:600;color:#7a9aaa}
.gsub small{font-size:10px;color:#3a4f5f;letter-spacing:.8px}

/* ── Power colors ──────────────────────────────────────────── */
#pwr-val.pos{color:#ff8844}
#pwr-val.neg{color:#22ccff}
#pwr-val.zero{color:#2e3f50}

/* ── Battery color ─────────────────────────────────────────── */
#bat-pct{color:#00d4aa}
#bat-v{color:#3a5060}

/* ── SVG wrapper sizes — constrained by both width and height ─ */
/* Speed SVG: square ViewBox, must not exceed panel height.     */
/* calc((100vh-80px)*0.9) ensures height fits the screen.       */
.side-wrap{width:min(100%,215px)}
.spd-wrap{width:min(100%, min(52vw, calc((100vh - 80px) * 0.92)))}

/* ── Portrait mode overlay ─────────────────────────────────── */
#rotate-hint{
  display:none;
  position:fixed;inset:0;z-index:200;
  background:#05080f;
  flex-direction:column;align-items:center;justify-content:center;
  gap:12px;
}
#rotate-hint svg{opacity:.35}
#rotate-hint p{font-size:14px;color:#445566;letter-spacing:2px;text-transform:uppercase}
#rotate-hint small{font-size:11px;color:#2e3f50;letter-spacing:1px}
@media (orientation:portrait){
  #rotate-hint{display:flex}
}
</style>
</head>
<body>

<!-- Portrait overlay — hidden in landscape via CSS @media -->
<div id="rotate-hint">
  <svg width="48" height="48" viewBox="0 0 48 48" fill="none">
    <rect x="10" y="4" width="28" height="40" rx="4" stroke="#445566" stroke-width="2"/>
    <path d="M40 22 C40 14 34 8 24 8" stroke="#00d4aa" stroke-width="2" stroke-linecap="round"/>
    <polyline points="20,4 24,8 28,4" fill="none" stroke="#00d4aa" stroke-width="2" stroke-linejoin="round"/>
  </svg>
  <p>请横向握持设备</p>
  <small>ROTATE TO LANDSCAPE</small>
</div>

<!-- Top bar -->
<div id="topbar">
  <div id="tb-time">--:--</div>
  <div style="display:flex;align-items:center;gap:6px">
    <div id="tb-gear">--</div>
    <div id="brake-dot" title="刹车 Brake"></div>
  </div>
  <div id="tb-right">
    <span id="net-warn">NO WIFI</span>
    <span id="can-warn">NO CAN</span>
    <span id="fcw-warn">⚠ FCW</span>
    <span class="bsd" id="bsd-left">◀ BSD</span>
    <span class="bsd" id="bsd-right">BSD ▶</span>
    <span id="ldw-warn">↔ LDW</span>
    <div id="tb-temp">--/-- °C</div>
    <div class="fsd-badge" id="ap-badge" title="AP/NOA 激活状态">AP</div>
    <div class="fsd-badge" id="fsd-badge">FSD</div>
    <button id="fs-btn" onclick="toggleFS()" title="全屏">⛶</button>
  </div>
</div>

<!-- Panels -->
<div id="panels">

  <!-- LEFT: Power gauge -->
  <div class="panel" id="left-panel">
    <div class="glabel">POWER<span>功率</span></div>
    <div class="side-wrap">
      <svg viewBox="0 0 260 260" style="width:100%">
        <defs>
          <filter id="glow3">
            <feGaussianBlur in="SourceGraphic" stdDeviation="3.5" result="b"/>
            <feMerge><feMergeNode in="b"/><feMergeNode in="SourceGraphic"/></feMerge>
          </filter>
        </defs>
        <!-- Background track -->
        <path d="M 53.6 206.4 A 108 108 0 1 1 206.4 206.4"
              fill="none" stroke="#0b111c" stroke-width="14" stroke-linecap="round"/>
        <!-- Regen arc — fills from lower-right counterclockwise (mirrored) -->
        <path id="pwr-regen" d="M 53.6 206.4 A 108 108 0 1 1 206.4 206.4"
              fill="none" stroke="#22ccff" stroke-width="11" stroke-linecap="round"
              stroke-dasharray="0 509"
              filter="url(#glow3)"
              transform="scale(-1,1) translate(-260,0)"/>
        <!-- Discharge arc — fills from lower-left clockwise -->
        <path id="pwr-disch" d="M 53.6 206.4 A 108 108 0 1 1 206.4 206.4"
              fill="none" stroke="#ff7733" stroke-width="11" stroke-linecap="round"
              stroke-dasharray="0 509"
              filter="url(#glow3)"/>
        <!-- Zero tick at top (130, 22) -->
        <line x1="130" y1="12" x2="130" y2="27" stroke="#1a2a3a" stroke-width="2"/>
        <!-- Small label hints -->
        <text x="42"  y="218" font-family="Arial" font-size="9" fill="#1e2e3e" text-anchor="middle">REGEN</text>
        <text x="42"  y="229" font-family="Arial" font-size="9" fill="#162430" text-anchor="middle">回收</text>
        <text x="218" y="218" font-family="Arial" font-size="9" fill="#1e2e3e" text-anchor="middle">OUT</text>
        <text x="218" y="229" font-family="Arial" font-size="9" fill="#162430" text-anchor="middle">输出</text>
      </svg>
    </div>
    <div class="gnum zero" id="pwr-val">0</div>
    <div class="gunit">kW</div>
    <div class="gsub">
      <span><small>后轴指令 </small><b id="tf-val">--</b><small> Nm</small></span>
      <span><small>后轴实际 </small><b id="tr-val">--</b><small> Nm</small></span>
    </div>
    <div id="nag-row">
      <div class="nag-dot" id="nd0"></div>
      <div class="nag-dot" id="nd1"></div>
      <div class="nag-dot" id="nd2"></div>
      <div class="nag-dot" id="nd3"></div>
      <div class="nag-dot" id="nd4"></div>
    </div>
  </div>

  <div class="vdiv"></div>

  <!-- CENTER: Speed gauge -->
  <div class="panel" id="center-panel">
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

        <!-- Outer ambient ring -->
        <circle cx="250" cy="250" r="200" fill="none"
                stroke="rgba(0,212,170,0.035)" stroke-width="1.5"/>
        <circle cx="250" cy="250" r="178" fill="none"
                stroke="rgba(255,255,255,0.012)" stroke-width="1"/>

        <!-- Background track -->
        <path d="M 115.6 384.4 A 190 190 0 1 1 384.4 384.4"
              fill="none" stroke="#0a1018" stroke-width="20" stroke-linecap="round"/>

        <!-- Tick marks (injected by JS) -->
        <g id="spd-ticks"></g>

        <!-- Speed fill arc -->
        <path id="spd-arc" d="M 115.6 384.4 A 190 190 0 1 1 384.4 384.4"
              fill="none" stroke="url(#spdGrad)" stroke-width="16" stroke-linecap="round"
              stroke-dasharray="0 895"
              filter="url(#glow5)"/>

        <!-- Speed number -->
        <text id="spd-txt" x="250" y="290" text-anchor="middle"
              font-family="-apple-system,'Helvetica Neue',Arial,sans-serif"
              font-size="104" font-weight="800" fill="#ffffff" letter-spacing="-6">0</text>

        <!-- km/h unit -->
        <text x="250" y="318" text-anchor="middle"
              font-family="-apple-system,'Helvetica Neue',Arial,sans-serif"
              font-size="11" fill="#3a5060" letter-spacing="3">km/h · 公里/时</text>

        <!-- Scale endpoints -->
        <text x="100" y="400" text-anchor="middle" font-family="Arial" font-size="11" fill="#1e2e3e">0</text>
        <text x="400" y="400" text-anchor="middle" font-family="Arial" font-size="11" fill="#1e2e3e">200</text>
        <text x="250" y="52"  text-anchor="middle" font-family="Arial" font-size="11" fill="#1e2e3e">100</text>

        <!-- Speed limit sign — camera-detected, shown in arc gap below -->
        <g id="spd-limit-g" visibility="hidden">
          <circle cx="250" cy="445" r="42" fill="white" stroke="#e01020" stroke-width="9"/>
          <circle cx="250" cy="445" r="34" fill="none" stroke="#e01020" stroke-width="1.5" opacity="0.25"/>
          <text id="spd-limit-txt" x="250" y="457"
                text-anchor="middle"
                font-family="-apple-system,'Helvetica Neue',Arial,sans-serif"
                font-size="30" font-weight="900" fill="#111111">--</text>
        </g>
      </svg>
    </div>
  </div>

  <div class="vdiv"></div>

  <!-- RIGHT: Battery gauge -->
  <div class="panel" id="right-panel">
    <div class="glabel">BATTERY<span>电量</span></div>
    <div class="side-wrap">
      <svg viewBox="0 0 260 260" style="width:100%">
        <defs>
          <linearGradient id="batGrad" x1="53.6" y1="0" x2="206.4" y2="0" gradientUnits="userSpaceOnUse">
            <stop offset="0%"   stop-color="#ff2244"/>
            <stop offset="22%"  stop-color="#ff8800"/>
            <stop offset="58%"  stop-color="#44dd88"/>
            <stop offset="100%" stop-color="#00d4aa"/>
          </linearGradient>
          <filter id="glow3b">
            <feGaussianBlur in="SourceGraphic" stdDeviation="3.5" result="b"/>
            <feMerge><feMergeNode in="b"/><feMergeNode in="SourceGraphic"/></feMerge>
          </filter>
        </defs>
        <!-- Background track -->
        <path d="M 53.6 206.4 A 108 108 0 1 1 206.4 206.4"
              fill="none" stroke="#0b111c" stroke-width="14" stroke-linecap="round"/>
        <!-- Battery fill -->
        <path id="bat-arc" d="M 53.6 206.4 A 108 108 0 1 1 206.4 206.4"
              fill="none" stroke="url(#batGrad)" stroke-width="11" stroke-linecap="round"
              stroke-dasharray="0 509"
              filter="url(#glow3b)"/>
        <!-- Scale hints -->
        <text x="42" y="222" font-family="Arial" font-size="9" fill="#1e2e3e" text-anchor="middle">0%</text>
        <text x="218" y="222" font-family="Arial" font-size="9" fill="#1e2e3e" text-anchor="middle">100%</text>
      </svg>
    </div>
    <div class="gnum" id="bat-pct">--%</div>
    <div class="gunit" id="bat-v">-- V</div>
    <div class="gsub">
      <span><small>电流 </small><b id="bat-a">--</b><small> A</small></span>
      <span><small>温度 </small><b id="bat-t">--/--</b><small> °C</small></span>
    </div>
  </div>

</div><!-- /panels -->

<script>
// ── Auth ──────────────────────────────────────────────────────
var tok = sessionStorage.getItem('fsd_tok') || '';

// ── Arc constants ─────────────────────────────────────────────
var SA = 895;          // speed arc total stroke-length px
var GA = 509;          // side gauge arc total stroke-length px
var MAX_SPD   = 200;   // kph at full scale
var MAX_PWR_D = 150;   // kW discharge full scale
var MAX_PWR_R = 60;    // kW regen full scale

// ── Lerp state ────────────────────────────────────────────────
var tSpd=0, cSpd=0;
var tBat=0, cBat=0;
var tPwr=0, cPwr=0;
var K = 0.10;

// ── Network health ────────────────────────────────────────────
var pollFails = 0;

// ── Nag dot colors by level (0-15 → 5 dots) ──────────────────
function updateNag(level) {
  var row = document.getElementById('nag-row');
  if (level === 0) { row.style.display = 'none'; return; }
  row.style.display = 'flex';
  var filled = Math.min(5, Math.round(level / 3) + 1);
  var col = level <= 5 ? '#ffcc00' : level <= 10 ? '#ff8800' : '#ff3344';
  for (var i = 0; i < 5; i++) {
    document.getElementById('nd' + i).style.background =
      i < filled ? col : '#1a2530';
  }
}

function lerp(a, b, k) { return a + (b - a) * k; }
function clamp(v, lo, hi) { return v < lo ? lo : v > hi ? hi : v; }

// ── Build speed tick marks via JS ─────────────────────────────
// Arc: SVG clockwise from 135° to 135°+270°=405° (≡45°) around cx=cy=250, r=190
(function buildTicks() {
  var g  = document.getElementById('spd-ticks');
  var cx = 250, cy = 250, r = 190;
  var ns = 'http://www.w3.org/2000/svg';
  for (var i = 0; i <= 10; i++) {
    var deg = 135 + 27 * i;   // 27° per step, 10 steps = 270°
    var rad = deg * Math.PI / 180;
    var cos = Math.cos(rad), sin = Math.sin(rad);
    var major = (i % 2 === 0);
    var r1 = r - (major ? 24 : 14), r2 = r + 4;
    var ln = document.createElementNS(ns, 'line');
    ln.setAttribute('x1', (cx + r1 * cos).toFixed(2));
    ln.setAttribute('y1', (cy + r1 * sin).toFixed(2));
    ln.setAttribute('x2', (cx + r2 * cos).toFixed(2));
    ln.setAttribute('y2', (cy + r2 * sin).toFixed(2));
    ln.setAttribute('stroke', major ? '#1a2e44' : '#111e2e');
    ln.setAttribute('stroke-width', major ? '2.5' : '1.5');
    g.appendChild(ln);
  }
})();

// ── Clock ─────────────────────────────────────────────────────
function updateClock() {
  var n = new Date();
  document.getElementById('tb-time').textContent =
    ('0' + n.getHours()).slice(-2) + ':' + ('0' + n.getMinutes()).slice(-2);
}

// ── Gear ──────────────────────────────────────────────────────
var GEARS = { 1:'P', 2:'R', 3:'N', 4:'D' };
function setGear(g) {
  var el = document.getElementById('tb-gear');
  var label = GEARS[g];
  if (label) {
    el.textContent = label;
    el.className   = label;
  } else {
    el.textContent = '--';
    el.className   = 'unknown';
  }
}

// ── Animation loop ────────────────────────────────────────────
function frame() {
  requestAnimationFrame(frame);
  cSpd = lerp(cSpd, tSpd, K);
  cBat = lerp(cBat, tBat, K);
  cPwr = lerp(cPwr, tPwr, K);

  // Speed arc
  var sf = clamp(cSpd / MAX_SPD, 0, 1) * SA;
  document.getElementById('spd-arc')
    .setAttribute('stroke-dasharray', sf.toFixed(1) + ' ' + (SA - sf).toFixed(1));
  document.getElementById('spd-txt').textContent = Math.round(cSpd);

  // Battery arc
  var bf = clamp(cBat / 100, 0, 1) * GA;
  document.getElementById('bat-arc')
    .setAttribute('stroke-dasharray', bf.toFixed(1) + ' ' + (GA - bf).toFixed(1));

  // Power arcs — show only the active direction
  var pv = document.getElementById('pwr-val');
  if (cPwr >= 0) {
    var df = clamp(cPwr / MAX_PWR_D, 0, 1) * GA;
    document.getElementById('pwr-disch')
      .setAttribute('stroke-dasharray', df.toFixed(1) + ' ' + (GA - df).toFixed(1));
    document.getElementById('pwr-regen')
      .setAttribute('stroke-dasharray', '0 509');
    var pv_int = Math.round(cPwr);
    pv.textContent = (pv_int > 0 ? '+' : '') + pv_int;
    pv.className = 'gnum ' + (pv_int > 1 ? 'pos' : 'zero');
  } else {
    var rf = clamp(-cPwr / MAX_PWR_R, 0, 1) * GA;
    document.getElementById('pwr-regen')
      .setAttribute('stroke-dasharray', rf.toFixed(1) + ' ' + (GA - rf).toFixed(1));
    document.getElementById('pwr-disch')
      .setAttribute('stroke-dasharray', '0 509');
    pv.textContent = Math.round(cPwr);
    pv.className = 'gnum neg';
  }
}

// ── Data poll ─────────────────────────────────────────────────
function poll() {
  fetch('/api/status' + (tok ? '?token=' + tok : ''))
    .then(function(r) {
      if (r.status === 403) { location.href = '/'; return null; }
      return r.json();
    })
    .then(function(d) {
      if (!d) return;

      // CAN bus status
      var canWarn = document.getElementById('can-warn');
      canWarn.style.display = d.canOK ? 'none' : 'inline-block';

      // Speed & gear (only meaningful when CAN is online)
      tSpd = d.canOK ? (d.speedD || 0) / 10 : 0;
      setGear(d.canOK ? d.gearRaw : 0);

      // Power (from BMS — valid only when bmsSeen)
      if (d.bmsSeen) {
        var v = (d.bmsV || 0) / 100, a = (d.bmsA || 0) / 10;
        tPwr = parseFloat((v * a / 1000).toFixed(2));
        tBat = (d.bmsSoc || 0) / 10;
        document.getElementById('bat-pct').textContent =
          ((d.bmsSoc || 0) / 10).toFixed(1) + '%';
        document.getElementById('bat-v').textContent =
          ((d.bmsV || 0) / 100).toFixed(1) + ' V';
        document.getElementById('bat-a').textContent =
          ((d.bmsA || 0) / 10).toFixed(1);
        document.getElementById('bat-t').textContent =
          d.bmsMinT + '/' + d.bmsMaxT;
        document.getElementById('tb-temp').textContent =
          d.bmsMinT + '/' + d.bmsMaxT + ' °C';
      } else {
        tPwr = 0;
        document.getElementById('bat-pct').textContent = '--%';
        document.getElementById('bat-v').textContent   = '-- V';
        document.getElementById('bat-a').textContent   = '--';
        document.getElementById('bat-t').textContent   = '--/--';
        document.getElementById('tb-temp').textContent = '--/-- °C';
      }

      // Torque
      document.getElementById('tf-val').textContent =
        d.canOK ? Math.round((d.torqueF || 0) * 2) : '--';
      document.getElementById('tr-val').textContent =
        d.canOK ? Math.round((d.torqueR || 0) * 2) : '--';

      // AP / FSD badge — show only one: AP takes priority when ACC/AP is engaged
      // fsdTriggered alone only means "UI selected FSD"; actual injection requires fsdEnable too
      var apOn = d.accState > 0;
      var fsdOn = d.fsdTriggered && !!d.fsdEnable && !apOn;
      document.getElementById('ap-badge').className  = 'fsd-badge' + (apOn  ? ' on' : '');
      document.getElementById('fsd-badge').className = 'fsd-badge' + (fsdOn ? ' on' : '');
      document.getElementById('ap-badge').style.display  = (apOn || !fsdOn) ? '' : 'none';
      document.getElementById('fsd-badge').style.display = (!apOn) ? '' : 'none';

      // FCW warning
      document.getElementById('fcw-warn').style.display =
        d.fcw > 0 ? 'inline-block' : 'none';

      // Blind spot warning (sideCol: bit0=left, bit1=right)
      document.getElementById('bsd-left').className  = 'bsd' + ((d.sideCol & 1) ? ' on' : '');
      document.getElementById('bsd-right').className = 'bsd' + ((d.sideCol & 2) ? ' on' : '');

      // Lane departure warning
      document.getElementById('ldw-warn').style.display = (d.laneWarn > 0) ? 'inline-block' : 'none';

      // Brake indicator
      document.getElementById('brake-dot').className =
        'dot' + (d.brake ? ' on' : '');
      document.getElementById('brake-dot').className =
        d.brake ? 'brake-dot on' : 'brake-dot';

      // Nag level dots
      updateNag(d.nagLevel || 0);

      // Vision speed limit sign
      var lim = d.visionLimit || 0;
      var sg = document.getElementById('spd-limit-g');
      if (lim > 0) {
        document.getElementById('spd-limit-txt').textContent = lim * 5;
        sg.setAttribute('visibility', 'visible');
      } else {
        sg.setAttribute('visibility', 'hidden');
      }


      pollFails = 0;
      document.getElementById('net-warn').style.display = 'none';
      updateClock();
    })
    .catch(function() {
      pollFails++;
      // Show NO WIFI after 3 consecutive failures (~3s)
      if (pollFails >= 3) {
        document.getElementById('net-warn').style.display = 'inline-block';
      }
    });
}


// ── Fullscreen ────────────────────────────────────────────────
function toggleFS() {
  var el = document.documentElement;
  if (!document.fullscreenElement) {
    var req = el.requestFullscreen || el.webkitRequestFullscreen || el.mozRequestFullScreen;
    if (req) {
      req.call(el).catch(function() {});
    } else {
      // iOS Safari: no Fullscreen API — suggest Add to Home Screen
      document.body.classList.add('no-fs');
      alert('iOS 不支持网页全屏。\n请点击 Safari 分享按钮 → 添加到主屏幕，从主屏幕打开即可全屏显示。');
    }
  } else {
    var ex = document.exitFullscreen || document.webkitExitFullscreen || document.mozCancelFullScreen;
    if (ex) ex.call(document);
  }
}

document.addEventListener('fullscreenchange', function() {
  document.getElementById('fs-btn').textContent = document.fullscreenElement ? '✕' : '⛶';
});
document.addEventListener('webkitfullscreenchange', function() {
  document.getElementById('fs-btn').textContent = document.webkitFullscreenElement ? '✕' : '⛶';
});

// ── Wake Lock — prevent screen sleep ─────────────────────────
var _wakeLock = null;
async function acquireWakeLock() {
  if (!navigator.wakeLock) return;
  try {
    _wakeLock = await navigator.wakeLock.request('screen');
    _wakeLock.addEventListener('release', function() { _wakeLock = null; });
  } catch(e) {}
}
// Re-acquire after page becomes visible again (tab switch / phone unlock)
document.addEventListener('visibilitychange', function() {
  if (document.visibilityState === 'visible') acquireWakeLock();
});
acquireWakeLock();

// ── Start ─────────────────────────────────────────────────────
setInterval(poll, 1000);
setInterval(updateClock, 15000);
updateClock();
requestAnimationFrame(frame);
poll();
</script>
</body>
</html>
)dash";
