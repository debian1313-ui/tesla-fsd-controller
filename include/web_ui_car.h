#pragma once
// ── Tesla 车机横屏 UI — /car ──────────────────────────────────────────────────
// 目标：Tesla 车机浏览器（AMD Ryzen/Intel MCU），1920×1200 横屏触控。
// 特点：大字号（16~28px 基线）、大按钮（≥56px）、左导航 + 主内容、最少打字。
// Auth：复用 sessionStorage 'fsd_tok'；403 时回 / 输密码。
// API：完全复用 /api/status、/api/set、/api/reboot、/api/ota、/api/wifi-bridge/*。
//
// Tabs：仪表 / 控制 / CAN / 网络 / DNS / OTA / 设置（7 个）

#ifdef ARDUINO
#include <pgmspace.h>
#else
#define PROGMEM
#endif

const char CAR_HTML[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="zh">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no">
<title>特斯拉控制器 · 车机</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
html,body{height:100%;background:#0b1120;color:#e2e8f0;font-family:-apple-system,"PingFang SC","Microsoft YaHei",sans-serif;overflow:hidden;-webkit-tap-highlight-color:transparent;user-select:none;-webkit-user-select:none}
button{font-family:inherit;cursor:pointer}
.app{display:grid;grid-template-rows:88px 1fr;grid-template-columns:240px 1fr;grid-template-areas:"header header" "nav main";height:100vh;width:100vw}
.header{grid-area:header;display:flex;align-items:center;gap:32px;padding:0 32px;background:#131d32;border-bottom:1px solid #1e293b}
.logo{font-size:28px;font-weight:800;color:#38bdf8;letter-spacing:2px}
.logo-ver{font-size:16px;color:#64748b;font-weight:500;margin-left:8px}
.hstat{display:flex;gap:24px;margin-left:auto;font-size:18px;font-weight:600}
.hs{display:flex;align-items:center;gap:8px}
.hs .dot{width:12px;height:12px;border-radius:50%;background:#64748b}
.hs.ok .dot{background:#22c55e}
.hs.warn .dot{background:#f59e0b}
.hs.err .dot{background:#ef4444}
.nav{grid-area:nav;background:#131d32;border-right:1px solid #1e293b;padding:20px 12px;display:flex;flex-direction:column;gap:8px;overflow-y:auto}
.nbtn{height:88px;background:transparent;color:#94a3b8;border:2px solid transparent;border-radius:12px;font-size:20px;font-weight:600;display:flex;align-items:center;gap:14px;padding:0 18px;text-align:left;transition:all .15s}
.nbtn .ic{font-size:28px}
.nbtn:active{background:#1e293b}
.nbtn.active{background:#1e40af;color:#fff;border-color:#38bdf8}
.main{grid-area:main;padding:28px;overflow-y:auto}
.main h2{font-size:24px;font-weight:700;color:#64748b;letter-spacing:3px;margin-bottom:20px}
.page{display:none}
.page.show{display:block}
/* ── dashboard v1.5 (全新设计) ─────────────────── */
.cluster{position:relative;padding:30px 40px 24px;margin-bottom:24px;background:radial-gradient(ellipse 70% 55% at 50% 48%,#1a2c4e 0%,#0b1120 75%);border-radius:28px;overflow:hidden;min-height:660px;animation:cluster-in .6s cubic-bezier(.2,.8,.2,1)}
.cluster::before{content:'';position:absolute;inset:0;background:radial-gradient(ellipse 30% 22% at 50% 48%,rgba(56,189,248,.1) 0%,transparent 70%),radial-gradient(ellipse 12% 8% at 20% 30%,rgba(52,211,153,.05),transparent 70%),radial-gradient(ellipse 12% 8% at 80% 30%,rgba(239,68,68,.05),transparent 70%);pointer-events:none}
.scene{position:relative;display:grid;grid-template-columns:240px 1fr 170px;gap:20px;align-items:center;min-height:680px}
/* ── 挡位列 ── */
.c-gear{text-align:center;position:relative;z-index:2}
.gear-big{font-size:260px;font-weight:900;line-height:.82;font-family:-apple-system,'SF Pro Display',system-ui,sans-serif;letter-spacing:-16px;color:#64748b;transition:color .35s;text-shadow:0 6px 80px currentColor;display:inline-block}
.gear-big.P{color:#60a5fa}
.gear-big.R{color:#f87171}
.gear-big.N{color:#94a3b8}
.gear-big.D{color:#34d399}
.gear-big.pop{animation:gear-pop .85s cubic-bezier(.25,1.5,.35,1)}
@keyframes gear-pop{0%{transform:scale(.4) rotate(-8deg);opacity:.2;filter:blur(8px)}40%{transform:scale(1.55) rotate(3deg);opacity:1;filter:blur(0)}62%{transform:scale(.86)}78%{transform:scale(1.08)}92%{transform:scale(.98)}100%{transform:scale(1)}}
.ap-mini{margin-top:14px;padding:9px 22px;display:inline-flex;align-items:center;gap:10px;background:rgba(15,23,42,.55);border:1px solid rgba(30,41,59,.9);border-radius:22px;font-size:16px;color:#64748b;letter-spacing:3px;font-weight:700;backdrop-filter:blur(6px)}
.ap-mini .apv{color:#64748b}
.ap-mini .ap-dot{width:10px;height:10px;border-radius:50%;background:#475569;transition:all .3s}
.ap-mini.on{background:rgba(52,211,153,.1);border-color:rgba(52,211,153,.35)}
.ap-mini.on .apv{color:#34d399;animation:ap-pulse 2.2s ease-in-out infinite}
.ap-mini.on .ap-dot{background:#34d399;box-shadow:0 0 14px #34d399;animation:ap-dot-blink 1.6s ease-in-out infinite}
@keyframes ap-pulse{0%,100%{text-shadow:0 0 16px rgba(52,211,153,.55)}50%{text-shadow:0 0 32px rgba(52,211,153,.95),0 0 60px rgba(52,211,153,.5)}}
@keyframes ap-dot-blink{0%,100%{opacity:1}50%{opacity:.3}}
/* ── 速度环 (最大化) ── */
.speedo-wrap{position:relative;width:100%;max-width:860px;aspect-ratio:1;margin:0 auto}
.speedo-arc{position:absolute;inset:0;width:100%;height:100%;overflow:visible;filter:drop-shadow(0 0 30px rgba(56,189,248,.5))}
.speedo-arc circle{transition:stroke-dasharray .55s cubic-bezier(.3,.9,.3,1)}
.speedo-inner{position:absolute;inset:6%;display:flex;flex-direction:column;align-items:center;justify-content:center;text-align:center;background:radial-gradient(circle at 50% 55%,rgba(15,23,42,.55) 0%,rgba(11,17,32,.2) 55%,transparent 72%);border-radius:50%}
.spd-huge{font-size:340px;font-weight:900;line-height:.8;letter-spacing:-10px;color:#f8fafc;font-variant-numeric:tabular-nums;font-family:-apple-system,'SF Pro Display',system-ui,sans-serif;text-shadow:0 4px 80px rgba(56,189,248,.35);will-change:transform;transition:text-shadow .3s}
.spd-huge.tick{animation:spd-tick .5s ease-out}
@keyframes spd-tick{0%{transform:scale(1)}30%{transform:scale(1.06);text-shadow:0 6px 100px rgba(56,189,248,.75)}100%{transform:scale(1);text-shadow:0 4px 80px rgba(56,189,248,.35)}}
.spd-unit{margin-top:18px;font-size:30px;color:#475569;letter-spacing:12px;font-weight:700;text-transform:uppercase}
.spd-off{margin-top:24px;padding:10px 26px;background:rgba(251,191,36,.08);border:1px solid rgba(251,191,36,.4);border-radius:24px;font-size:20px;color:#fbbf24;font-weight:700;letter-spacing:1px;display:inline-flex;align-items:center;gap:12px;box-shadow:0 4px 24px rgba(251,191,36,.2)}
.spd-off .spd-off-sep{color:rgba(251,191,36,.4)}
/* ── 限速牌 (缩小) ── */
.c-limit{display:flex;flex-direction:column;align-items:center;gap:8px;position:relative;z-index:2}
.c-limit .l-src{font-size:12px;color:#64748b;letter-spacing:3px;min-height:16px;font-weight:600;text-transform:uppercase}
.limit-sign{width:140px;height:140px;border-radius:50%;background:radial-gradient(circle at 30% 30%,#fff 0%,#f8fafc 70%);border:11px solid #dc2626;display:flex;flex-direction:column;align-items:center;justify-content:center;box-shadow:0 6px 36px rgba(220,38,38,.4),inset 0 3px 10px rgba(0,0,0,.08);animation:glow-breath 3.5s ease-in-out infinite}
@keyframes glow-breath{0%,100%{box-shadow:0 6px 36px rgba(220,38,38,.4),inset 0 3px 10px rgba(0,0,0,.08)}50%{box-shadow:0 6px 54px rgba(220,38,38,.65),inset 0 3px 10px rgba(0,0,0,.08)}}
.limit-num{font-size:58px;font-weight:900;color:#0f172a;line-height:1;letter-spacing:-3px;font-family:-apple-system,system-ui,sans-serif}
.limit-unit{font-size:11px;color:#475569;font-weight:700;letter-spacing:1px;margin-top:-2px}
/* ── SOC 宽条 ── */
.soc-wide{margin-top:22px;padding:0 10px}
.soc-wide-head{display:flex;justify-content:space-between;align-items:baseline;margin-bottom:10px}
.soc-wide-lbl{font-size:13px;color:#64748b;letter-spacing:4px;font-weight:700;text-transform:uppercase}
.soc-wide-val{font-size:32px;font-weight:900;color:#34d399;font-variant-numeric:tabular-nums;letter-spacing:-1px}
.soc-wide-val.low{color:#fbbf24}
.soc-wide-val.crit{color:#ef4444}
.soc-wide-bar{position:relative;height:14px;background:rgba(15,23,42,.9);border:1px solid rgba(30,41,59,.9);border-radius:7px;overflow:hidden}
.soc-wide-fill{height:100%;background:linear-gradient(90deg,#34d399 0%,#22c55e 100%);background-size:200% 100%;transition:width .55s cubic-bezier(.3,.9,.3,1);box-shadow:0 0 22px rgba(52,211,153,.5);animation:soc-shimmer 3s linear infinite}
.soc-wide-fill.low{background:linear-gradient(90deg,#fbbf24,#f59e0b);box-shadow:0 0 22px rgba(251,191,36,.5)}
.soc-wide-fill.crit{background:linear-gradient(90deg,#ef4444,#dc2626);box-shadow:0 0 22px rgba(239,68,68,.5)}
@keyframes soc-shimmer{0%{background-position:0 0}100%{background-position:-200% 0}}
/* ── 信息芯片 ── */
.chips{margin-top:18px;display:flex;gap:14px;justify-content:center;flex-wrap:wrap;padding:0 10px}
.chip{display:inline-flex;align-items:center;gap:10px;padding:10px 22px;background:rgba(15,23,42,.55);border:1px solid rgba(30,41,59,.9);border-radius:22px;font-size:17px;color:#cbd5e1;font-weight:700;backdrop-filter:blur(6px)}
.chip-ic{font-size:17px}
.chip-v{font-variant-numeric:tabular-nums;color:#e2e8f0}
/* ── fade 通用 ── */
.fade-el{transition:opacity .32s ease, transform .32s ease, visibility .32s}
.fade-el.hide{opacity:0;visibility:hidden;transform:scale(.9);pointer-events:none}
@keyframes cluster-in{from{opacity:0;transform:translateY(10px)}to{opacity:1;transform:translateY(0)}}
/* ── 仪表盘模式：导航栏变成窄图标栏（每项始终可点） ── */
.app{transition:grid-template-columns .35s ease}
.nav{transition:padding .25s ease}
.nbtn{transition:all .15s, padding .25s, height .25s, gap .25s}
.app.nav-collapsed{grid-template-columns:78px 1fr}
.app.nav-collapsed .nav{padding:14px 6px;gap:6px}
.app.nav-collapsed .nbtn{flex-direction:column;height:72px;padding:6px 0;gap:4px;font-size:11px;letter-spacing:1px;justify-content:center}
.app.nav-collapsed .nbtn .ic{font-size:26px;margin:0}
.panel{background:#131d32;border:1px solid #1e293b;border-radius:16px;padding:24px;margin-bottom:20px}
.ptitle{font-size:18px;font-weight:700;color:#e2e8f0;margin-bottom:18px;letter-spacing:1px}
.row{display:flex;align-items:center;gap:16px;padding:14px 0;border-bottom:1px solid #1e293b}
.row:last-child{border-bottom:none}
.rlbl{font-size:20px;font-weight:600;flex:0 0 200px}
.rval{font-size:20px;color:#94a3b8;font-family:monospace}
.bgroup{display:flex;gap:12px;flex:1}
.pill{flex:1;height:72px;background:#1e293b;color:#cbd5e1;border:2px solid #334155;border-radius:12px;font-size:22px;font-weight:700;letter-spacing:1px;transition:all .15s}
.pill:active{transform:scale(.98)}
.pill.active{background:#1e40af;color:#fff;border-color:#38bdf8}
.pill.dis{opacity:.4;pointer-events:none}
.stepper{display:flex;align-items:center;gap:16px;flex:1}
.sbtn{width:72px;height:72px;background:#1e293b;color:#e2e8f0;border:2px solid #334155;border-radius:12px;font-size:32px;font-weight:700}
.sbtn:active{transform:scale(.96)}
.sval{min-width:140px;height:72px;background:#0f172a;border:2px solid #38bdf8;border-radius:12px;display:flex;align-items:center;justify-content:center;font-size:32px;font-weight:800;color:#38bdf8;font-variant-numeric:tabular-nums}
.tog{width:88px;height:48px;background:#334155;border-radius:24px;position:relative;transition:background .2s;flex:0 0 88px;cursor:pointer}
.tog.on{background:#22c55e}
.tog::after{content:'';position:absolute;top:4px;left:4px;width:40px;height:40px;background:#fff;border-radius:50%;transition:left .2s}
.tog.on::after{left:44px}
.actions{display:flex;gap:16px;margin-top:24px}
.abtn{flex:1;height:80px;background:#22c55e;color:#fff;border:none;border-radius:14px;font-size:22px;font-weight:700;letter-spacing:2px}
.abtn:active{transform:scale(.98)}
.abtn.red{background:#ef4444}
.abtn.blue{background:#1e40af}
.abtn.gray{background:#1e293b;color:#94a3b8;border:2px solid #334155}
.toast{position:fixed;top:100px;left:50%;transform:translateX(-50%);background:#131d32;border:2px solid #38bdf8;color:#e2e8f0;padding:16px 32px;border-radius:12px;font-size:18px;font-weight:600;z-index:1000;opacity:0;pointer-events:none;transition:opacity .2s}
.toast.show{opacity:1}
.toast.err{border-color:#ef4444;color:#fca5a5}
.toast.ok{border-color:#22c55e;color:#86efac}
.info-grid{display:grid;grid-template-columns:repeat(2,1fr);gap:16px}
.info-item{background:#0f172a;border:1px solid #1e293b;border-radius:10px;padding:14px 18px}
.info-lbl{font-size:13px;color:#64748b;letter-spacing:2px;margin-bottom:4px;font-weight:600}
.info-val{font-size:20px;color:#e2e8f0;font-family:monospace;font-weight:600}
.stat-grid{display:grid;grid-template-columns:repeat(4,1fr);gap:16px;margin-bottom:20px}
.scard{background:#0f172a;border:1px solid #1e293b;border-radius:12px;padding:20px 16px;text-align:center}
.snum{font-size:36px;font-weight:800;color:#38bdf8;font-variant-numeric:tabular-nums;line-height:1}
.slbl{font-size:14px;color:#64748b;margin-top:8px;letter-spacing:2px;font-weight:600}
.sunit{font-size:16px;color:#64748b;margin-left:4px;font-weight:600}
.tip{font-size:14px;color:#64748b;margin-top:8px;line-height:1.6}
.disclaimer{background:#7f1d1d;border:2px solid #ef4444;border-radius:12px;padding:16px 20px;margin-bottom:20px;color:#fca5a5;font-size:16px;font-weight:600;line-height:1.6}
.ota-zone{border:3px dashed #334155;border-radius:16px;padding:40px;text-align:center;background:#0f172a;margin-bottom:20px}
.ota-zone.drag{border-color:#38bdf8;background:#131d32}
.ota-tip{font-size:20px;color:#94a3b8;margin-bottom:16px}
.ota-fname{font-size:18px;color:#e2e8f0;margin:12px 0;font-family:monospace;word-break:break-all}
.progress{width:100%;height:20px;background:#1e293b;border-radius:10px;overflow:hidden;margin:16px 0;display:none}
.progress-bar{height:100%;background:linear-gradient(90deg,#38bdf8,#22c55e);transition:width .2s}
.smart-rule{display:flex;align-items:center;gap:14px;padding:10px 0;font-size:18px;color:#cbd5e1}
.smart-rule input[type=number]{background:#0f172a;color:#38bdf8;border:2px solid #334155;border-radius:8px;padding:8px 10px;font-size:20px;text-align:center;font-weight:700;width:90px;-moz-appearance:textfield}
.smart-rule input::-webkit-outer-spin-button,.smart-rule input::-webkit-inner-spin-button{-webkit-appearance:none;margin:0}
.car-num{width:100px;font-size:22px;background:#0f172a;border:2px solid #38bdf8;color:#38bdf8;border-radius:8px;padding:8px 10px;text-align:center;font-weight:700;font-variant-numeric:tabular-nums;-moz-appearance:textfield}
.car-num::-webkit-outer-spin-button,.car-num::-webkit-inner-spin-button{-webkit-appearance:none;margin:0}
.phone-link{position:fixed;bottom:20px;right:20px;background:#1e293b;color:#94a3b8;border:1px solid #334155;border-radius:8px;padding:10px 16px;font-size:14px;text-decoration:none}
.pin-overlay{position:fixed;inset:0;background:rgba(11,17,32,.95);display:none;align-items:center;justify-content:center;z-index:2000}
.pin-overlay.show{display:flex}
.pin-box{background:#131d32;border:2px solid #38bdf8;border-radius:20px;padding:40px;width:520px}
.pin-box h3{font-size:26px;color:#38bdf8;margin-bottom:20px;text-align:center}
.pin-box input{width:100%;background:#0b1120;border:2px solid #334155;border-radius:12px;padding:16px 20px;color:#e2e8f0;font-size:24px;text-align:center;letter-spacing:8px;font-weight:700}
.pin-box input:focus{outline:none;border-color:#38bdf8}
.pin-err{color:#ef4444;font-size:16px;text-align:center;margin-top:12px;min-height:20px}
/* ── 响应式：车机浏览器不全屏时 (~800-1200px) 或小屏设备 ─────────── */
@media (max-width:1400px){
  .app{grid-template-columns:160px 1fr;grid-template-rows:72px 1fr}
  .header{padding:0 18px;gap:18px}
  .logo{font-size:22px}
  .logo-ver{font-size:13px}
  .hstat{gap:16px;font-size:15px}
  .nbtn{height:68px;font-size:16px;padding:0 12px;gap:10px}
  .nbtn .ic{font-size:22px}
  .main{padding:18px}
  .main h2{font-size:20px;margin-bottom:14px}
}
@media (max-width:1000px){
  html,body{overflow:auto}
  .app{grid-template-rows:auto auto 1fr;grid-template-columns:1fr;grid-template-areas:"header" "nav" "main";height:auto;min-height:100vh}
  .header{flex-wrap:wrap;height:auto;padding:10px 14px;gap:10px}
  .hstat{width:100%;gap:10px;font-size:13px;flex-wrap:wrap}
  .nav{flex-direction:row;overflow-x:auto;overflow-y:hidden;padding:8px 10px;gap:6px;border-right:none;border-bottom:1px solid #1e293b;-webkit-overflow-scrolling:touch}
  .nbtn{height:46px;flex:0 0 auto;white-space:nowrap;padding:0 14px;font-size:14px;border-radius:10px}
  .nbtn .ic{font-size:18px}
  .main{padding:14px}
  .stat-grid{grid-template-columns:repeat(2,1fr);gap:10px}
  .scard{padding:14px 10px}
  .snum{font-size:28px}
  .slbl{font-size:12px;letter-spacing:1px}
  .info-grid{grid-template-columns:1fr}
  /* 仪表盘缩放 */
  .cluster{padding:14px;min-height:auto}
  .scene{grid-template-columns:110px 1fr 96px;gap:8px;min-height:auto}
  .gear-big{font-size:140px;letter-spacing:-8px}
  .spd-huge{font-size:180px}
  .spd-unit{font-size:18px;letter-spacing:6px}
  .limit-sign{width:96px;height:96px;border-width:7px}
  .limit-num{font-size:38px}
  .speedo-wrap{max-width:520px}
  .pin-box{width:92%;max-width:460px;padding:28px}
}
</style>
</head>
<body>

<a href="/?ui=phone" class="phone-link" title="切换到手机版">📱 手机版</a>

<div class="app">
  <!-- 顶栏 -->
  <div class="header">
    <div>
      <span class="logo">特斯拉控制器</span>
      <span class="logo-ver">v)rawliteral" FIRMWARE_VERSION R"rawliteral(</span>
    </div>
    <div class="hstat">
      <div class="hs" id="hsCAN"><span class="dot"></span><span id="hsCANTxt">CAN --</span></div>
      <div class="hs" id="hsWifi"><span class="dot"></span><span id="hsWifiTxt">Wi-Fi --</span></div>
      <div class="hs" id="hsTemp"><span>🌡 <span id="hsTempTxt">--°C</span></span></div>
      <div class="hs" id="hsHeap"><span>💾 <span id="hsHeapTxt">--KB</span></span></div>
    </div>
  </div>

  <!-- 左导航（仪表盘页自动收窄为图标栏） -->
  <div class="nav" id="nav">
    <button class="nbtn active" data-page="dash"><span class="ic">🏎</span>仪表</button>
    <button class="nbtn" data-page="ctrl"><span class="ic">⚙️</span>控制</button>
    <button class="nbtn" data-page="can"><span class="ic">📡</span>CAN</button>
    <button class="nbtn" data-page="net"><span class="ic">🌐</span>网络</button>
    <button class="nbtn" data-page="dns"><span class="ic">🛡</span>DNS</button>
    <button class="nbtn" data-page="ota"><span class="ic">⬆️</span>OTA</button>
    <button class="nbtn" data-page="log"><span class="ic">📋</span>日志</button>
    <button class="nbtn" data-page="set"><span class="ic">🔧</span>设置</button>
  </div>

  <div class="main">

    <!-- ───── 仪表 ───── -->
    <div class="page show" id="pg-dash">
      <div id="rowSafeMode" style="display:none;background:#7f1d1d;border:2px solid #ef4444;border-radius:12px;padding:16px 20px;margin-bottom:20px;color:#fca5a5;font-size:18px;font-weight:700;line-height:1.5">
        ⚠️ 安全模式 — 设备连续崩溃，CAN 已禁用。请重新刷写固件或恢复出厂。
      </div>
      <div class="cluster">
        <div class="scene">
          <!-- 左：挡位 + 自驾 -->
          <div class="c-gear fade-el hide" id="cGear">
            <div class="gear-big" id="gearBig"></div>
            <div class="ap-mini fade-el hide" id="apChip"><span class="ap-dot"></span>AP<span class="apv" id="vGwAp"></span></div>
          </div>
          <!-- 中：速度环 (最大化) -->
          <div class="speedo-wrap">
            <svg class="speedo-arc" viewBox="0 0 100 100" preserveAspectRatio="xMidYMid meet" aria-hidden="true">
              <defs>
                <linearGradient id="arcGrad" x1="0" y1="1" x2="1" y2="0">
                  <stop offset="0%" stop-color="#38bdf8"/>
                  <stop offset="50%" stop-color="#60a5fa"/>
                  <stop offset="100%" stop-color="#a78bfa"/>
                </linearGradient>
              </defs>
              <circle cx="50" cy="50" r="45" fill="none" stroke="rgba(30,41,59,.65)" stroke-width="2.5" stroke-dasharray="212.06 283" transform="rotate(135 50 50)" stroke-linecap="round"/>
              <circle id="arcFill" cx="50" cy="50" r="45" fill="none" stroke="url(#arcGrad)" stroke-width="2.8" stroke-dasharray="0 283" transform="rotate(135 50 50)" stroke-linecap="round"/>
            </svg>
            <div class="speedo-inner">
              <div class="spd-huge" id="vSpeed">0</div>
              <div class="spd-unit">km/h</div>
              <div class="spd-off fade-el hide" id="spdOff">
                <span id="vOffsetPct"></span>
                <span class="spd-off-sep">·</span>
                <span>+<span id="vOffset"></span> km/h</span>
              </div>
            </div>
          </div>
          <!-- 右：限速牌 (小) -->
          <div class="c-limit fade-el hide" id="limitSign">
            <div class="l-src" id="vFusedSrc">&nbsp;</div>
            <div class="limit-sign">
              <div class="limit-num" id="vFused"></div>
              <div class="limit-unit">km/h</div>
            </div>
          </div>
        </div>
        <div class="soc-wide fade-el hide" id="socMini">
          <div class="soc-wide-head">
            <span class="soc-wide-lbl">电量</span>
            <span class="soc-wide-val" id="vSocWrap"><span id="vSoc"></span>%</span>
          </div>
          <div class="soc-wide-bar">
            <div class="soc-wide-fill" id="socFill" style="width:0%"></div>
          </div>
        </div>
        <div class="chips">
          <div class="chip fade-el hide" id="miBmsT"><span class="chip-ic">🔋</span><span class="chip-v"><span id="vBmsT"></span>°C</span></div>
          <div class="chip fade-el hide" id="rowCabinTemp"><span class="chip-ic">🌡</span><span class="chip-v"><span id="vCabinTemp"></span>°C</span></div>
          <div class="chip fade-el hide" id="miSmart"><span class="chip-ic">⚡</span><span class="chip-v" id="vSmart"></span></div>
          <div class="chip fade-el hide" id="miUp"><span class="chip-ic">⏱</span><span class="chip-v" id="vUp"></span></div>
        </div>
      </div>

    </div>

    <!-- ───── 控制 ───── -->
    <div class="page" id="pg-ctrl">
      <h2>控制 · CONTROL</h2>
      <div class="disclaimer">⚠️ 仅用于技术探讨 · 所有改动风险自负 · 不建议在真实驾驶中启用</div>

      <div class="panel">
        <div class="ptitle">主开关</div>
        <div class="row"><div class="rlbl">功能启用</div><div class="tog" id="tgFsd" data-k="fsdEnable"></div><div class="rval">关闭后所有改写停止</div></div>
      </div>

      <div class="panel">
        <div class="ptitle">硬件型号</div>
        <div class="bgroup" id="grpHw">
          <button class="pill" data-v="1">HW3</button>
          <button class="pill" data-v="2">HW4</button>
          <button class="pill" data-v="0">Legacy</button>
        </div>
        <div class="tip" id="hwTip">HW3 = Model 3/Y 大部分；HW4 = Highland/新 S/X；Legacy = 老款</div>
      </div>

      <div class="panel">
        <div class="ptitle">速度模式</div>
        <div class="bgroup" id="grpSpd">
          <button class="pill" data-sp="0" data-hw3="轻松" data-hw4="平缓">平缓</button>
          <button class="pill" data-sp="1" data-hw3="标准" data-hw4="舒适">舒适</button>
          <button class="pill" data-sp="2" data-hw3="迅捷" data-hw4="标准">标准</button>
          <button class="pill" data-sp="3" data-hw3="" data-hw4="迅捷">迅捷</button>
          <button class="pill" data-sp="4" data-hw3="" data-hw4="狂飙">狂飙</button>
        </div>
        <div class="row" style="margin-top:12px">
          <div class="rlbl">模式来源</div>
          <div class="bgroup" id="grpPMode" style="flex:1">
            <button class="pill" data-pm="1">自动（拨杆）</button>
            <button class="pill" data-pm="0">手动</button>
          </div>
        </div>
        <div class="tip">🛈 "自动（拨杆）"时由驾驶模式按钮切换；"手动"时固定为上方选择的档位。</div>
      </div>

      <div class="panel" id="panelHw3" style="display:none">
        <div class="ptitle">HW3 速度偏移</div>
        <div class="row" id="rowHw3Manual">
          <div class="rlbl">手动偏移</div>
          <div class="stepper">
            <button class="sbtn" id="hw3Minus">−</button>
            <div class="sval"><span id="vHw3M">--</span>%</div>
            <button class="sbtn" id="hw3Plus">+</button>
            <div class="rval" style="margin-left:12px">% 限速（0~100%，每档 5%）</div>
          </div>
        </div>
        <div class="row"><div class="rlbl">智能模式</div><div class="tog" id="tgSmart" data-k="hw3Smart"></div><div class="rval">随车速自动调节偏移</div></div>
      </div>

      <!-- HW3 智能规则（分段偏移） -->
      <div class="panel" id="panelSmart" style="display:none">
        <div class="ptitle">HW3 智能规则 · 分段偏移（% of 限速）</div>
        <div class="row">
          <div class="rlbl">低速</div>
          <div class="rval" style="flex:0 0 180px">&le; <input type="number" id="smT1" min="20" max="195" class="car-num"> km/h</div>
          <div class="stepper"><button class="sbtn" data-sm="O1" data-d="-5">−</button><div class="sval"><span id="smO1V">--</span>%</div><button class="sbtn" data-sm="O1" data-d="5">+</button></div>
        </div>
        <div class="row">
          <div class="rlbl">1 档</div>
          <div class="rval" style="flex:0 0 180px"><span id="smT1L">--</span>~<input type="number" id="smT2" min="20" max="200" class="car-num"> km/h</div>
          <div class="stepper"><button class="sbtn" data-sm="O2" data-d="-5">−</button><div class="sval"><span id="smO2V">--</span>%</div><button class="sbtn" data-sm="O2" data-d="5">+</button></div>
        </div>
        <div class="row">
          <div class="rlbl">2 档</div>
          <div class="rval" style="flex:0 0 180px"><span id="smT2L">--</span>~<input type="number" id="smT3" min="20" max="200" class="car-num"> km/h</div>
          <div class="stepper"><button class="sbtn" data-sm="O3" data-d="-5">−</button><div class="sval"><span id="smO3V">--</span>%</div><button class="sbtn" data-sm="O3" data-d="5">+</button></div>
        </div>
        <div class="row">
          <div class="rlbl">3 档</div>
          <div class="rval" style="flex:0 0 180px"><span id="smT3L">--</span>~<input type="number" id="smT4" min="20" max="200" class="car-num"> km/h</div>
          <div class="stepper"><button class="sbtn" data-sm="O4" data-d="-5">−</button><div class="sval"><span id="smO4V">--</span>%</div><button class="sbtn" data-sm="O4" data-d="5">+</button></div>
        </div>
        <div class="row">
          <div class="rlbl">高速</div>
          <div class="rval" style="flex:0 0 180px">&gt; <span id="smT4L">--</span> km/h</div>
          <div class="stepper"><button class="sbtn" data-sm="O5" data-d="-5">−</button><div class="sval"><span id="smO5V">--</span>%</div><button class="sbtn" data-sm="O5" data-d="5">+</button></div>
        </div>
        <div class="actions" style="margin-top:16px">
          <button class="abtn gray" onclick="smartReset()">🔄 恢复默认</button>
          <button class="abtn blue" onclick="smartSave()">💾 保存规则</button>
        </div>
        <div class="tip">🛈 每档偏移范围 0~50%；阈值 T1&lt;T2&lt;T3&lt;T4 必须递增；保存后立即生效。</div>
      </div>

      <div class="panel" id="panelHw4" style="display:none">
        <div class="ptitle">HW4 速度偏移</div>
        <div class="bgroup" id="grpHw4Off">
          <button class="pill" data-hw4="0">关闭</button>
          <button class="pill" data-hw4="7">+5 km/h</button>
          <button class="pill" data-hw4="10">+7 km/h</button>
          <button class="pill" data-hw4="14">+10 km/h</button>
          <button class="pill" data-hw4="21">+15 km/h</button>
        </div>
      </div>

      <div class="panel">
        <div class="ptitle">常用开关</div>
        <div class="row"><div class="rlbl">ISA 提示音</div><div class="tog" id="tgIsa" data-k="isaChime"></div><div class="rval">屏蔽限速提示音</div></div>
        <div class="row" id="rowEmerg" style="display:none"><div class="rlbl">紧急检测</div><div class="tog" id="tgEmerg" data-k="emergencyDet"></div><div class="rval">紧急制动/碰撞检测（HW4 专用）</div></div>
        <div class="row"><div class="rlbl">强制激活</div><div class="tog" id="tgForce" data-k="forceActivate"></div><div class="rval">强制启用 FSD（风险高）</div></div>
        <div class="row" id="rowOvr" style="display:none"><div class="rlbl">忽略限速</div><div class="tog" id="tgOvr" data-k="overrideSL"></div><div class="rval">忽略路牌限速（Legacy 专用）</div></div>
        <div class="row"><div class="rlbl">AP 自动恢复</div><div class="tog" id="tgApRs"></div><div class="rval">刹车后自动恢复 AP</div></div>
        <div class="row" id="rowTrack" style="display:none"><div class="rlbl">赛道模式</div><div class="tog" id="tgTrack" data-k="trackMode"></div><div class="rval">解锁高性能（HW3 专用）</div></div>
      </div>
    </div>

    <!-- ───── CAN ───── -->
    <div class="page" id="pg-can">
      <h2>CAN · 总线监控</h2>
      <div class="stat-grid">
        <div class="scard"><div class="snum"><span id="cRx">--</span></div><div class="slbl">接收帧</div></div>
        <div class="scard"><div class="snum"><span id="cMod">--</span></div><div class="slbl">改写帧</div></div>
        <div class="scard"><div class="snum"><span id="cErr">--</span></div><div class="slbl">错误</div></div>
        <div class="scard"><div class="snum"><span id="cRate">--</span><span class="sunit">Hz</span></div><div class="slbl">速率</div></div>
      </div>
      <div class="panel">
        <div class="ptitle">状态</div>
        <div class="info-grid">
          <div class="info-item"><div class="info-lbl">CAN 控制器</div><div class="info-val" id="cOK">--</div></div>
          <div class="info-item"><div class="info-lbl">硬件检测</div><div class="info-val" id="cHw">--</div></div>
          <div class="info-item"><div class="info-lbl">FSD 触发</div><div class="info-val" id="cFsd">--</div></div>
          <div class="info-item"><div class="info-lbl">安全模式</div><div class="info-val" id="cSafe">--</div></div>
        </div>
      </div>
      <div class="actions">
        <button class="abtn gray" onclick="openPhone()">📱 打开手机版</button>
      </div>
    </div>

    <!-- ───── 网络 ───── -->
    <div class="page" id="pg-net">
      <h2>网络 · NETWORK</h2>
      <div class="panel">
        <div class="ptitle">热点信息</div>
        <div class="info-grid">
          <div class="info-item"><div class="info-lbl">AP SSID</div><div class="info-val" id="nAp">--</div></div>
          <div class="info-item"><div class="info-lbl">AP IP</div><div class="info-val">9.9.9.9</div></div>
          <div class="info-item"><div class="info-lbl">上联热点</div><div class="info-val" id="nSta">--</div></div>
          <div class="info-item"><div class="info-lbl">上联 IP</div><div class="info-val" id="nStaIp">--</div></div>
        </div>
      </div>
      <div class="panel" id="panelBridge">
        <div class="ptitle">WiFi 桥接（上联互联网）</div>
        <div class="info-grid">
          <div class="info-item"><div class="info-lbl">状态</div><div class="info-val" id="brStat">--</div></div>
          <div class="info-item"><div class="info-lbl">当前连接</div><div class="info-val" id="brSsid">--</div></div>
          <div class="info-item"><div class="info-lbl">NAT</div><div class="info-val" id="brNat">--</div></div>
          <div class="info-item"><div class="info-lbl">信号/温度</div><div class="info-val" id="brRssi">--</div></div>
        </div>
        <div style="margin-top:16px">
          <div style="font-size:16px;color:#94a3b8;margin-bottom:8px">已保存热点</div>
          <div id="brList" style="font-size:17px;line-height:1.9;color:#cbd5e1">--</div>
        </div>
        <div style="margin-top:18px;padding-top:16px;border-top:1px solid #1e293b">
          <div style="font-size:16px;color:#94a3b8;margin-bottom:10px">添加热点</div>
          <div style="display:flex;gap:10px;flex-wrap:wrap">
            <input id="brAddSsid" placeholder="SSID"
              style="flex:1;min-width:220px;height:52px;padding:0 14px;background:#0f172a;color:#e2e8f0;border:1px solid #334155;border-radius:10px;font-size:18px">
            <input id="brAddPass" type="password" placeholder="密码 (开放网络留空)"
              style="flex:1;min-width:220px;height:52px;padding:0 14px;background:#0f172a;color:#e2e8f0;border:1px solid #334155;border-radius:10px;font-size:18px">
            <button class="abtn blue" onclick="brAdd()" style="min-width:130px">➕ 添加</button>
          </div>
          <div id="brAddMsg" class="tip" style="margin-top:8px;display:none"></div>
        </div>
      </div>
      <div class="actions">
        <button class="abtn gray" onclick="openPhone()">📱 打开手机版配置</button>
      </div>
    </div>

    <!-- ───── DNS ───── -->
    <div class="page" id="pg-dns">
      <h2>DNS 过滤 · DNS FILTER</h2>
      <div class="panel">
        <div class="ptitle">过滤状态</div>
        <div class="row"><div class="rlbl">启用过滤</div><div class="tog" id="tgDns"></div><div class="rval" id="dnsMode">关闭</div></div>
      </div>
      <div class="panel">
        <div class="ptitle">快捷预设</div>
        <div class="bgroup" id="grpDnsPreset" style="flex-direction:column;gap:14px">
          <button class="pill" id="pstTesla" style="flex:0 0 72px" onclick="dnsPreset('tesla_min')">⭐ Tesla 推荐（实测）</button>
          <button class="pill" id="pstBl"    style="flex:0 0 72px" onclick="dnsPreset('bl_telemetry')">只屏蔽遥测</button>
          <button class="pill" id="pstClr"   style="flex:0 0 72px" onclick="dnsPreset('clear')">清空所有规则</button>
        </div>
        <div class="tip" style="margin-top:14px">🛈 自定义域名列表请用手机版编辑</div>
      </div>
      <div class="panel" id="panelDnsBlk">
        <div class="ptitle">最近拦截 <span style="color:#64748b;font-size:13px;font-weight:500">总计 <span id="brBlkTot">0</span> 次</span></div>
        <div id="brBlkList" style="font-size:17px;line-height:1.9;color:#cbd5e1">--</div>
      </div>
      <div class="panel" id="panelIpStats">
        <div class="ptitle">IP 拦截缓存</div>
        <div class="info-grid">
          <div class="info-item"><div class="info-lbl">已丢弃包</div><div class="info-val"><span id="brIpDrops">--</span></div></div>
          <div class="info-item"><div class="info-lbl">缓存条数</div><div class="info-val"><span id="brIpCache">--</span> / <span id="brIpCacheCap">--</span></div></div>
          <div class="info-item"><div class="info-lbl">峰值</div><div class="info-val"><span id="brIpPeak">--</span></div></div>
        </div>
      </div>
      <div class="actions">
        <button class="abtn gray" onclick="openPhone()">📱 打开手机版</button>
      </div>
    </div>

    <!-- ───── OTA ───── -->
    <div class="page" id="pg-ota">
      <h2>固件升级 · OTA</h2>
      <div class="panel">
        <div class="ptitle">当前版本</div>
        <div class="info-grid">
          <div class="info-item"><div class="info-lbl">固件版本</div><div class="info-val" id="oVer">--</div></div>
          <div class="info-item"><div class="info-lbl">变体</div><div class="info-val" id="oVar">--</div></div>
        </div>
      </div>
      <div class="panel">
        <div class="ptitle">上传新固件</div>
        <div class="ota-zone" id="otaZone">
          <div class="ota-tip">点击下方按钮选择固件 .bin 文件</div>
          <div class="ota-fname" id="otaFname">（未选择）</div>
          <div class="actions" style="margin-top:10px">
            <button class="abtn gray" onclick="otaPick()">📁 选择文件</button>
            <button class="abtn blue" id="otaUp" onclick="showOtaConfirm()" disabled style="opacity:.5">⬆️ 开始升级</button>
          </div>
          <div id="otaConfirmBox" style="display:none;background:#0b1120;border:2px solid #f59e0b;border-radius:12px;padding:16px;margin-top:14px;color:#fcd34d">
            <div style="font-size:18px;font-weight:700;margin-bottom:8px">⚠️ 确认刷入 <span id="otaConfirmName" style="font-family:monospace;color:#fbbf24">--</span>？</div>
            <div style="font-size:15px;margin-bottom:12px;color:#fca5a5">升级期间请勿断电或切换页面。</div>
            <div class="actions" style="margin-top:0">
              <button class="abtn" style="background:#dc2626" onclick="otaUpload()">✓ 确认升级</button>
              <button class="abtn gray" onclick="hideOtaConfirm()">取消</button>
            </div>
          </div>
        </div>
        <div class="progress" id="progBox"><div class="progress-bar" id="progBar" style="width:0%"></div></div>
        <div class="tip">🛈 升级过程中请勿断电或关闭页面。升级成功后模块将自动重启。</div>
        <input type="file" id="otaFile" accept=".bin" style="display:none">
      </div>
    </div>

    <!-- ───── 日志 ───── -->
    <div class="page" id="pg-log">
      <h2>诊断日志 · LOG</h2>
      <div class="panel">
        <div class="ptitle">事件日志 <span style="color:#64748b;font-size:13px;font-weight:500">掉电清除 · 最近 80 条</span></div>
        <textarea id="diagLog" readonly style="width:100%;height:460px;background:#0f172a;color:#94a3b8;border:1px solid #1e293b;border-radius:10px;padding:12px;font-size:15px;font-family:monospace;resize:none;box-sizing:border-box;line-height:1.5">(加载中...)</textarea>
        <div class="actions">
          <button class="abtn blue" onclick="logRefresh()">🔄 刷新</button>
          <button class="abtn" onclick="logCopy()">📋 复制</button>
          <button class="abtn gray" onclick="showLogClear()">🗑 清除</button>
        </div>
        <div id="logClearBox" style="display:none;background:#0b1120;border:2px solid #ef4444;border-radius:12px;padding:16px;margin-top:14px;color:#fca5a5">
          <div style="font-size:18px;font-weight:700;margin-bottom:12px">确认清除所有日志？</div>
          <div class="actions" style="margin-top:0">
            <button class="abtn red" onclick="logClear()">✓ 清除</button>
            <button class="abtn gray" onclick="hideLogClear()">取消</button>
          </div>
        </div>
        <div class="tip">🛈 日志下载持久版本请用手机版（车机浏览器不便保存文件）</div>
      </div>
    </div>

    <!-- ───── 设置 ───── -->
    <div class="page" id="pg-set">
      <h2>设置 · SETTINGS</h2>
      <div class="panel">
        <div class="ptitle">系统信息</div>
        <div class="info-grid">
          <div class="info-item"><div class="info-lbl">固件版本</div><div class="info-val" id="sVer">--</div></div>
          <div class="info-item"><div class="info-lbl">变体</div><div class="info-val" id="sVar">--</div></div>
          <div class="info-item"><div class="info-lbl">运行时间</div><div class="info-val" id="sUp">--</div></div>
          <div class="info-item"><div class="info-lbl">可用内存</div><div class="info-val" id="sHeap">--</div></div>
        </div>
      </div>
      <div class="panel">
        <div class="ptitle">操作</div>
        <div class="actions" style="margin-top:0">
          <button class="abtn blue" onclick="showRebootConfirm()">🔄 重启模块</button>
          <button class="abtn gray" onclick="openPhone()">📱 打开手机版</button>
        </div>
        <div id="rebootConfirmBox" style="display:none;background:#0b1120;border:2px solid #f59e0b;border-radius:12px;padding:16px;margin-top:14px;color:#fcd34d">
          <div style="font-size:18px;font-weight:700;margin-bottom:12px">确认重启模块？</div>
          <div class="actions" style="margin-top:0">
            <button class="abtn blue" onclick="doReboot()">✓ 确认重启</button>
            <button class="abtn gray" onclick="hideRebootConfirm()">取消</button>
          </div>
        </div>
        <div class="actions">
          <button class="abtn red" onclick="showFactoryConfirm()">⚠️ 恢复出厂</button>
        </div>
        <div id="factoryConfirmBox" style="display:none;background:#450a0a;border:2px solid #dc2626;border-radius:12px;padding:16px;margin-top:14px;color:#fca5a5">
          <div style="font-size:18px;font-weight:700;margin-bottom:8px">⚠️ 确认恢复出厂？</div>
          <div style="font-size:15px;margin-bottom:12px">将清空所有配置：热点 / 密码 / DNS / 速度偏移 / 智能规则 / PIN 等。此操作不可撤销。</div>
          <div class="actions" style="margin-top:0">
            <button class="abtn red" onclick="doFactory()">✓ 确认恢复出厂</button>
            <button class="abtn gray" onclick="hideFactoryConfirm()">取消</button>
          </div>
        </div>
        <div class="tip">⚠️ 恢复出厂会清空所有配置（热点/密码/DNS/速度偏移等），请谨慎操作。</div>
      </div>
    </div>

  </div>
</div>

<div class="toast" id="toast"></div>

<!-- PIN 验证弹窗 -->
<div class="pin-overlay" id="pinOv">
  <div class="pin-box">
    <h3>🔒 输入访问密码</h3>
    <input type="password" id="pinInput" maxlength="8" inputmode="numeric" pattern="[0-9]*">
    <div class="pin-err" id="pinErr"></div>
    <div class="actions" style="margin-top:20px">
      <button class="abtn blue" onclick="submitPin()">确认</button>
    </div>
  </div>
</div>

<script>
var tok = sessionStorage.getItem('fsd_tok') || '';
var pollT = null;
var last = null;
var smartLocal = {T1:40,T2:60,T3:80,T4:100,O1:50,O2:25,O3:15,O4:10,O5:8};
var smartDirty = false;

// ───── 导航 ─────
function applyNavForPage(p){
  var app = document.querySelector('.app');
  // 仪表盘：导航变窄图标栏；其他页：全宽导航
  if(p === 'dash'){ app.classList.add('nav-collapsed'); }
  else { app.classList.remove('nav-collapsed'); }
}
document.querySelectorAll('.nbtn').forEach(function(b){
  b.onclick = function(){
    document.querySelectorAll('.nbtn').forEach(function(x){x.classList.remove('active')});
    b.classList.add('active');
    var p = b.dataset.page;
    document.querySelectorAll('.page').forEach(function(x){x.classList.remove('show')});
    document.getElementById('pg-'+p).classList.add('show');
    applyNavForPage(p);
    if(p === 'log') logRefresh();
  };
});
// 初始启动：默认在仪表盘 → 收起导航
applyNavForPage('dash');

// ───── 速度平滑插值（60fps, 与轮询解耦） ─────
window.__spdDisplay = 0; window.__spdTarget = 0; window.__spdLast = 0;
(function smoothSpeed(){
  var t = window.__spdTarget || 0;
  var v = window.__spdDisplay || 0;
  var diff = t - v;
  if(Math.abs(diff) > 0.4){ v += diff * 0.085; }
  else { v = t; }
  window.__spdDisplay = v;
  var show = Math.round(v);
  var el = document.getElementById('vSpeed');
  if(el && el.textContent !== String(show)) el.textContent = show;
  var arcFill = document.getElementById('arcFill');
  if(arcFill){
    var pct = Math.min(1, Math.max(0, v/200));
    arcFill.setAttribute('stroke-dasharray', (212.06 * pct).toFixed(1) + ' 283');
  }
  requestAnimationFrame(smoothSpeed);
})();

// ───── Toast ─────
function toast(msg, kind){
  var t = document.getElementById('toast');
  t.textContent = msg;
  t.className = 'toast show ' + (kind||'');
  clearTimeout(t._timer);
  t._timer = setTimeout(function(){t.className='toast'}, 2200);
}

// ───── PIN ─────
function showPin(){document.getElementById('pinOv').classList.add('show');setTimeout(function(){document.getElementById('pinInput').focus()},100)}
function hidePin(){document.getElementById('pinOv').classList.remove('show');document.getElementById('pinInput').value='';document.getElementById('pinErr').textContent=''}
function submitPin(){
  var pin = document.getElementById('pinInput').value;
  if(!pin){document.getElementById('pinErr').textContent='请输入密码';return;}
  var fd = new FormData(); fd.append('pin', pin);
  fetch('/api/auth',{method:'POST',body:fd}).then(function(r){return r.text().then(function(t){return {s:r.status,t:t}})})
    .then(function(res){
      if(res.s===200 && res.t){
        sessionStorage.setItem('fsd_tok', res.t);
        tok = res.t;
        hidePin();
        poll();
      } else {
        document.getElementById('pinErr').textContent='密码错误';
      }
    });
}
document.getElementById('pinInput').addEventListener('keydown', function(e){if(e.key==='Enter')submitPin()});

// ───── 状态轮询 ─────
function authSuffix(){return tok ? '?token='+encodeURIComponent(tok) : ''}
function poll(){
  fetch('/api/status'+authSuffix())
    .then(function(r){
      if(r.status===403){showPin();return null}
      return r.json();
    })
    .then(function(d){
      if(!d) return;
      last = d;
      render(d);
      // if pinRequired and no token, prompt
      if(d.pinRequired && !tok) showPin();
    })
    .catch(function(){/* ignore */});
}

// ───── 渲染 ─────
var GEARS = {1:'P',2:'R',3:'N',4:'D'};
function fmtUptime(s){
  var h = Math.floor(s/3600), m = Math.floor((s%3600)/60), ss = s%60;
  if(h>0) return h+'h '+m+'m';
  if(m>0) return m+'m '+ss+'s';
  return ss+'s';
}
function setStat(id, ok, txt){
  var el = document.getElementById(id);
  el.className = 'hs ' + (ok?'ok':'err');
  document.getElementById(id+'Txt').textContent = txt;
}

function render(d){
  // 顶栏
  setStat('hsCAN', d.canOK, 'CAN ' + (d.canOK?'已连接':'断开'));
  setStat('hsWifi', d.staOK, 'Wi-Fi ' + (d.staOK ? (d.staIP||'OK') : '仅热点'));
  var tempC = d.tempSeen ? Math.round(d.tempInRaw*0.25) : null;
  document.getElementById('hsTempTxt').textContent = tempC!==null ? tempC+'°C' : '--';
  document.getElementById('hsHeapTxt').textContent = Math.round(d.freeHeap/1024)+'KB';

  // ── 仪表盘 ──
  // 速度：仅设置目标值；RAF 循环负责平滑过渡
  var spd = Math.round(d.speedD * 0.2);
  var newSpd = (d.canOK && isFinite(spd)) ? spd : 0;
  var spdEl = document.getElementById('vSpeed');
  window.__spdTarget = newSpd;
  if(Math.abs(newSpd - (window.__spdLast||0)) >= 3){
    spdEl.classList.remove('tick'); void spdEl.offsetWidth; spdEl.classList.add('tick');
  }
  window.__spdLast = newSpd;
  // fusedLimit/visionLimit 原始 5-bit，×5 得 km/h
  var fusedKmh = (d.fusedLimit>0 && d.fusedLimit<31) ? d.fusedLimit*5
               : (d.visionLimit>0 && d.visionLimit<31 ? d.visionLimit*5 : 0);
  var offKmh = 0, offPctTxt = '', offShow = false;
  if(d.hwMode === 2){
    // HW4：离散档，与限速无关，只要非零就显示
    var HW4MAP = {0:0, 7:5, 10:7, 14:10, 21:15};
    offKmh = HW4MAP[d.hw4Offset] || 0;
    offPctTxt = 'HW4';
    offShow = offKmh > 0;
  } else if(fusedKmh > 0){
    // HW3/Legacy：需要真实限速才能算出有意义的 km/h
    var offPct = (d.hw3Smart || d.hw3Offset < 0) ? (d.smartPct || 0) : d.hw3Offset;
    offKmh = Math.round(fusedKmh * offPct / 100);
    offPctTxt = (offPct > 0 ? '+' : '') + offPct + '%';
    offShow = offKmh > 0;
  }
  var spdOff = document.getElementById('spdOff');
  if(offShow){
    document.getElementById('vOffset').textContent = offKmh;
    document.getElementById('vOffsetPct').textContent = offPctTxt;
  }
  spdOff.classList.toggle('hide', !offShow);
  // SOC：仅 bmsSeen 时显示
  var socMini = document.getElementById('socMini');
  var socFill = document.getElementById('socFill');
  if(d.bmsSeen){
    var socPct = d.bmsSoc / 10;
    document.getElementById('vSoc').textContent = socPct.toFixed(0);
    socFill.style.width = socPct + '%';
    socFill.classList.remove('low','crit');
    var vSocWrap = document.getElementById('vSocWrap');
    if(vSocWrap) vSocWrap.classList.remove('low','crit');
    if(socPct < 10){ socFill.classList.add('crit'); if(vSocWrap) vSocWrap.classList.add('crit'); }
    else if(socPct < 25){ socFill.classList.add('low'); if(vSocWrap) vSocWrap.classList.add('low'); }
  }
  socMini.classList.toggle('hide', !d.bmsSeen);
  // 电池温度
  var miBmsT = document.getElementById('miBmsT');
  if(d.bmsSeen) document.getElementById('vBmsT').textContent = d.bmsMaxT;
  miBmsT.classList.toggle('hide', !d.bmsSeen);
  // 限速牌：无数据隐藏
  var lim = document.getElementById('limitSign');
  if(fusedKmh > 0){
    document.getElementById('vFused').textContent = fusedKmh;
    var usingVision = !(d.fusedLimit>0 && d.fusedLimit<31) && (d.visionLimit>0 && d.visionLimit<31);
    document.getElementById('vFusedSrc').innerHTML = usingVision ? '视觉识别' : '&nbsp;';
  }
  lim.classList.toggle('hide', !(fusedKmh > 0));
  // 挡位：未知隐藏
  var gear = GEARS[d.gearRaw];
  var cGear = document.getElementById('cGear');
  var gbig = document.getElementById('gearBig');
  var apChip = document.getElementById('apChip');
  if(gear){
    // 换挡时触发弹出动画（隐藏后重新出现也要触发）
    if(gbig.textContent !== gear){
      gbig.textContent = gear;
      gbig.className = 'gear-big ' + gear;
      gbig.classList.remove('pop'); void gbig.offsetWidth; gbig.classList.add('pop');
    }
  } else {
    // 隐藏时清空，便于再次挂挡时触发 pop
    gbig.textContent = '';
  }
  cGear.classList.toggle('hide', !gear);
  // 自驾：仅 gwAutopilot>0（实际激活）且挡位可见时显示；0/NONE 或未感知直接隐藏
  var showAp = d.gwAutopilot > 0 && !!gear;
  if(showAp){
    var GWAP = ['NONE','HWY','EAP','FSD','BASIC'];
    document.getElementById('vGwAp').textContent = ' ' + (GWAP[d.gwAutopilot] || ('AP'+d.gwAutopilot));
    apChip.className = 'ap-mini fade-el on';
  }
  apChip.classList.toggle('hide', !showAp);
  // 智能档
  var miSmart = document.getElementById('miSmart');
  if(d.hw3Smart) document.getElementById('vSmart').textContent = '档位 ' + d.smartTier;
  miSmart.classList.toggle('hide', !d.hw3Smart);
  // 运行时间：始终有
  document.getElementById('vUp').textContent = fmtUptime(d.uptime);
  document.getElementById('miUp').classList.remove('hide');
  // 车内/外温度
  if(d.tempSeen){
    var tIn = (d.tempInRaw*0.25).toFixed(1);
    var tOut = (d.tempOutRaw*0.5 - 40).toFixed(1);
    document.getElementById('vCabinTemp').textContent = tIn+' / '+tOut;
  }
  document.getElementById('rowCabinTemp').classList.toggle('hide', !d.tempSeen);
  // 安全模式横幅
  document.getElementById('rowSafeMode').style.display = d.safeMode ? '' : 'none';

  // 控制页
  setTog('tgFsd', d.fsdEnable);
  setTog('tgSmart', d.hw3Smart);
  setTog('tgIsa', d.isaChime);
  setTog('tgEmerg', d.emergencyDet);
  setTog('tgForce', d.forceActivate);
  setTog('tgOvr', d.overrideSL);
  setTog('tgTrack', d.trackMode);
  // HW 选中
  document.querySelectorAll('#grpHw .pill').forEach(function(b){
    b.classList.toggle('active', parseInt(b.dataset.v)===d.hwMode);
  });
  // 速度模式 labels/可见 per hwMode（HW3/Legacy 3 档，HW4 5 档）
  var isHW3 = (d.hwMode===1), isHW4 = (d.hwMode===2), isLegacy = (d.hwMode===0);
  document.querySelectorAll('#grpSpd .pill').forEach(function(b){
    var sp = parseInt(b.dataset.sp);
    var lbl = isHW4 ? b.dataset.hw4 : b.dataset.hw3;
    if(lbl){ b.textContent = lbl; b.style.display = ''; }
    else   { b.style.display = 'none'; }
    b.classList.toggle('active', sp===d.speedProfile);
    b.classList.toggle('dis', !!d.profileMode);
  });
  document.querySelectorAll('#grpPMode .pill').forEach(function(b){
    b.classList.toggle('active', parseInt(b.dataset.pm)===d.profileMode);
  });
  setTog('tgApRs', d.apRestart);

  // HW3/HW4 面板切换（0=Legacy, 1=HW3, 2=HW4）
  var smartOn = !!d.hw3Smart;
  document.getElementById('panelHw3').style.display = isHW3 ? 'block' : 'none';
  document.getElementById('panelHw4').style.display = isHW4 ? 'block' : 'none';
  document.getElementById('panelSmart').style.display = (isHW3 && smartOn) ? 'block' : 'none';
  // HW3 内：智能 ON 时隐藏手动偏移；OFF 时显示
  document.getElementById('rowHw3Manual').style.display = smartOn ? 'none' : '';
  // 模式专属开关行
  document.getElementById('rowEmerg').style.display = isHW4 ? '' : 'none';
  document.getElementById('rowOvr').style.display = isLegacy ? '' : 'none';
  document.getElementById('rowTrack').style.display = isHW3 ? '' : 'none';
  // HW4 偏移 pill 选中
  document.querySelectorAll('#grpHw4Off .pill').forEach(function(b){
    b.classList.toggle('active', parseInt(b.dataset.hw4)===d.hw4Offset);
  });
  // HW3 手动偏移显示
  var hw3pct = d.hw3Offset >= 0 ? d.hw3Offset : (d.hw3AutoOffset >= 0 ? d.hw3AutoOffset : 0);
  document.getElementById('vHw3M').textContent = hw3pct;
  // 智能规则：阈值 & 各档偏移
  if(document.getElementById('smT1') && !smartDirty){
    document.getElementById('smT1').value = d.hw3SmT1;
    document.getElementById('smT2').value = d.hw3SmT2;
    document.getElementById('smT3').value = d.hw3SmT3;
    document.getElementById('smT4').value = d.hw3SmT4;
    document.getElementById('smT1L').textContent = d.hw3SmT1;
    document.getElementById('smT2L').textContent = d.hw3SmT2;
    document.getElementById('smT3L').textContent = d.hw3SmT3;
    document.getElementById('smT4L').textContent = d.hw3SmT4;
    document.getElementById('smO1V').textContent = d.hw3SmO1;
    document.getElementById('smO2V').textContent = d.hw3SmO2;
    document.getElementById('smO3V').textContent = d.hw3SmO3;
    document.getElementById('smO4V').textContent = d.hw3SmO4;
    document.getElementById('smO5V').textContent = d.hw3SmO5;
    smartLocal = {T1:d.hw3SmT1,T2:d.hw3SmT2,T3:d.hw3SmT3,T4:d.hw3SmT4,
                  O1:d.hw3SmO1,O2:d.hw3SmO2,O3:d.hw3SmO3,O4:d.hw3SmO4,O5:d.hw3SmO5};
  }

  // CAN 页
  document.getElementById('cRx').textContent = d.rx;
  document.getElementById('cMod').textContent = d.modified;
  document.getElementById('cErr').textContent = d.errors;
  document.getElementById('cRate').textContent = '~';
  document.getElementById('cOK').textContent = d.canOK ? '✅ OK' : '❌ FAIL';
  document.getElementById('cHw').textContent = ['未知','HW3','HW4'][d.hwDetected]||'未知';
  document.getElementById('cFsd').textContent = d.fsdTriggered ? '✅ 已触发' : '-';
  document.getElementById('cSafe').textContent = d.safeMode ? '⚠️ 安全模式' : '正常';

  // 网络页
  document.getElementById('nAp').textContent = d.apSSID||'--';
  document.getElementById('nSta').textContent = d.staSSID||'(未配置)';
  document.getElementById('nStaIp').textContent = d.staIP||'--';

  // 设置页
  document.getElementById('sVer').textContent = d.version + (d.variant?(' ('+d.variant+')'):'');
  document.getElementById('sVar').textContent = d.variant||'baseline';
  document.getElementById('sUp').textContent = fmtUptime(d.uptime);
  document.getElementById('sHeap').textContent = Math.round(d.freeHeap/1024)+' KB';

  // OTA 页
  document.getElementById('oVer').textContent = d.version;
  document.getElementById('oVar').textContent = d.variant||'baseline';
}

function setTog(id, on){
  var t = document.getElementById(id);
  if(!t) return;
  t.classList.toggle('on', !!on);
}

// ───── 开关点击 ─────
var TRACK_WARN = '⚠️ 实验性功能，效果未经完全验证。开启后将向总线持续注入赛道模式请求，可能影响车辆稳定控制。请知悉风险后开启。';
document.querySelectorAll('.tog[data-k]').forEach(function(t){
  t.onclick = function(){
    var k = t.dataset.k;
    var newVal = !t.classList.contains('on');
    if(k==='trackMode' && newVal && !confirm(TRACK_WARN)) return;
    t.classList.toggle('on', newVal);
    apiSet(k, newVal?1:0);
  };
});
// DNS 开关（单独，需要调 wifi-bridge API）
document.getElementById('tgDns').onclick = function(){
  var t = this;
  var newVal = !t.classList.contains('on');
  t.classList.toggle('on', newVal);
  document.getElementById('dnsMode').textContent = newVal?'启用':'关闭';
  var qs = '?dnsEnable='+(newVal?1:0) + (tok?'&token='+encodeURIComponent(tok):'');
  fetch('/api/wifi-bridge/set'+qs)
    .then(function(r){if(r.status===200){toast('DNS 过滤 '+(newVal?'已启用':'已关闭'), 'ok');brPoll();}else toast('失败','err')});
};

// ───── WiFi 桥接 / DNS 状态轮询 ─────
function brPoll(){
  fetch('/api/wifi-bridge/status'+(tok?'?token='+encodeURIComponent(tok):'')).then(function(r){return r.ok?r.json():null;}).then(function(d){
    if(!d) return;
    document.getElementById('brStat').textContent = (d.upstreamStatus||'--')
      + (d.upstreamRSSI!==null && d.upstreamRSSI!==undefined ? ' · '+d.upstreamRSSI+' dBm' : '');
    document.getElementById('brSsid').textContent = d.upstreamSSID || '--';
    document.getElementById('brNat').textContent  = d.natStatus || '--';
    var t = '--';
    if(d.chipTempC!==null && d.chipTempC!==undefined){
      t = d.chipTempC.toFixed(1)+'°C';
      if(d.thermalStatus) t += ' · '+d.thermalStatus;
    }
    document.getElementById('brRssi').textContent = t;
    // DNS 开关 & 文案
    var tg = document.getElementById('tgDns');
    tg.classList.toggle('on', !!d.dnsEnable);
    document.getElementById('dnsMode').textContent = d.dnsEnable ? '启用' : '关闭';
    // 已保存热点列表（可添加/删除，带内联确认）
    var list = document.getElementById('brList');
    var nets = d.upstreamNetworks || [];
    if(!nets.length){
      list.innerHTML = '<span style="color:#64748b">无已保存热点</span>';
    } else {
      list.innerHTML = nets.map(function(n){
        var dot = n.connected ? '🟢' : (n.active ? '🟡' : '⚪');
        var pass = n.hasPass ? '' : ' <span style="color:#fbbf24;font-size:14px">(开放)</span>';
        var safeSsid = escHtml(n.ssid);
        var ssidAttr = n.ssid.replace(/&/g,'&amp;').replace(/"/g,'&quot;');
        return ''
          + '<div style="padding:6px 0;border-bottom:1px solid #1e293b">'
          +   '<div style="display:flex;justify-content:space-between;align-items:center">'
          +     '<span>'+dot+' '+safeSsid+pass+'</span>'
          +     '<button class="abtn gray" style="min-width:84px;height:38px;font-size:15px" '
          +       'onclick="brDelConfirm(\''+ssidAttr+'\')">🗑 删除</button>'
          +   '</div>'
          +   '<div data-del-ssid="'+ssidAttr+'" style="display:none;margin-top:8px;padding:10px;background:#0b1120;border:2px solid #ef4444;border-radius:10px">'
          +     '<div style="color:#fca5a5;margin-bottom:8px">确认删除 <b>'+safeSsid+'</b>？</div>'
          +     '<div style="display:flex;gap:10px">'
          +       '<button class="abtn red" onclick="brDelDo(\''+ssidAttr+'\')">✓ 确认</button>'
          +       '<button class="abtn gray" onclick="brDelCancel(\''+ssidAttr+'\')">取消</button>'
          +     '</div>'
          +   '</div>'
          + '</div>';
      }).join('');
    }
    // 预设激活态
    highlightPreset(d.dnsAllow, d.dnsBlock);
    // IP 拦截缓存统计
    var setIp = function(id, v){ var el=document.getElementById(id); if(el) el.textContent = (v==null?'--':v); };
    setIp('brIpDrops',    d.ipBlockDrops);
    setIp('brIpCache',    d.ipCacheCount);
    setIp('brIpCacheCap', d.ipCacheCap);
    setIp('brIpPeak',     d.ipCachePeak);
    // 最近拦截
    document.getElementById('brBlkTot').textContent = d.dnsBlockedTotal || 0;
    var blk = d.dnsBlockedRecent || [];
    var blkEl = document.getElementById('brBlkList');
    if(!blk.length){
      blkEl.innerHTML = '<span style="color:#64748b">暂无拦截</span>';
    } else {
      blkEl.innerHTML = blk.map(function(e){
        return '<div style="display:flex;justify-content:space-between;padding:3px 0;border-bottom:1px solid #1e293b">'+
          '<span>'+escHtml(e.domain)+'</span><span style="color:#94a3b8">×'+e.count+'</span></div>';
      }).join('');
    }
  }).catch(function(){});
}
function escHtml(s){ var d=document.createElement('div'); d.textContent=String(s==null?'':s); return d.innerHTML; }

// ───── HW 选择 ─────
document.querySelectorAll('#grpHw .pill').forEach(function(b){
  b.onclick = function(){apiSet('hwMode', b.dataset.v)};
});
// ───── 速度模式 / 模式来源 ─────
document.querySelectorAll('#grpSpd .pill').forEach(function(b){
  b.onclick = function(){
    if(last && last.profileMode) return; // 自动模式下 speedProfile 只读
    apiSet('speedProfile', b.dataset.sp);
  };
});
document.querySelectorAll('#grpPMode .pill').forEach(function(b){
  b.onclick = function(){apiSet('profileMode', b.dataset.pm)};
});
// ───── AP 自动恢复（专用端点，不走 /api/set） ─────
document.getElementById('tgApRs').onclick = function(){
  var t = this;
  var newVal = !t.classList.contains('on');
  t.classList.toggle('on', newVal);
  fetch('/api/aprestart?en='+(newVal?1:0)+(tok?'&token='+encodeURIComponent(tok):''))
    .then(function(r){return r.json()})
    .then(function(j){if(j && j.ok){toast('已保存','ok');poll()}else{toast('失败','err')}})
    .catch(function(){toast('网络错误','err')});
};

// ───── HW3 偏移步进 ─────
document.getElementById('hw3Plus').onclick = function(){
  if(!last) return;
  var cur = last.hw3Offset >= 0 ? last.hw3Offset : 0;
  var v = Math.min(100, cur + 5); // 5 raw = 1 km/h
  apiSet('hw3Offset', v);
};
document.getElementById('hw3Minus').onclick = function(){
  if(!last) return;
  var cur = last.hw3Offset >= 0 ? last.hw3Offset : 0;
  var v = Math.max(0, cur - 5);
  apiSet('hw3Offset', v);
};
document.querySelectorAll('#grpHw4Off .pill').forEach(function(b){
  b.onclick = function(){apiSet('hw4Offset', b.dataset.hw4)};
});

// ───── 智能规则 ─────
document.querySelectorAll('#panelSmart [data-sm]').forEach(function(b){
  b.onclick = function(){
    var key = b.dataset.sm; // O1..O5
    var d = parseInt(b.dataset.d); // -5 or +5
    var cur = smartLocal[key] || 0;
    var v = Math.max(0, Math.min(50, cur + d));
    smartLocal[key] = v;
    smartDirty = true;
    document.getElementById('sm'+key+'V').textContent = v;
  };
});
['smT1','smT2','smT3','smT4'].forEach(function(id){
  var el = document.getElementById(id);
  if(!el) return;
  el.oninput = function(){
    smartDirty = true;
    var k = id.substring(2); // T1..T4
    smartLocal[k] = parseInt(el.value) || 0;
  };
});
function smartSave(){
  var t1 = smartLocal.T1, t2 = smartLocal.T2, t3 = smartLocal.T3, t4 = smartLocal.T4;
  if(!(t1 < t2 && t2 < t3 && t3 < t4)){
    toast('阈值必须递增 T1<T2<T3<T4','err'); return;
  }
  var qs = '?hw3SmT1='+t1+'&hw3SmT2='+t2+'&hw3SmT3='+t3+'&hw3SmT4='+t4
         +'&hw3SmO1='+smartLocal.O1+'&hw3SmO2='+smartLocal.O2
         +'&hw3SmO3='+smartLocal.O3+'&hw3SmO4='+smartLocal.O4+'&hw3SmO5='+smartLocal.O5;
  if(tok) qs += '&token='+encodeURIComponent(tok);
  fetch('/api/set'+qs).then(function(r){
    if(r.status===200){ smartDirty=false; toast('规则已保存','ok'); poll(); }
    else if(r.status===403){ showPin(); }
    else { smartDirty=false; toast('保存失败','err'); }
  }).catch(function(){ smartDirty=false; toast('网络错误','err'); });
}
function smartReset(){
  smartLocal = {T1:40,T2:60,T3:80,T4:100,O1:50,O2:25,O3:15,O4:10,O5:8};
  smartDirty = true;
  document.getElementById('smT1').value = 40;
  document.getElementById('smT2').value = 60;
  document.getElementById('smT3').value = 80;
  document.getElementById('smT4').value = 100;
  document.getElementById('smO1V').textContent = 50;
  document.getElementById('smO2V').textContent = 25;
  document.getElementById('smO3V').textContent = 15;
  document.getElementById('smO4V').textContent = 10;
  document.getElementById('smO5V').textContent = 8;
  smartSave();  // auto-persist so reset survives page navigation
}

// ───── apiSet ─────
function apiSet(k, v){
  fetch('/api/set?'+k+'='+v+(tok?'&token='+encodeURIComponent(tok):''))
    .then(function(r){if(r.status===200){toast('已保存','ok');poll()}else if(r.status===403){showPin()}else{toast('失败','err')}});
}

// ───── DNS 预设 ─────
var DNS_PRESETS = {
  tesla_min: {
    allow: 'connman.vn.cloud.tesla.cn nav-prd-maps.tesla.cn hermes-prd.vn.cloud.tesla.cn signaling.vn.cloud.tesla.cn media-server-me.tesla.cn www.tesla.cn maps-cn-prd.go.tesla.services volcengine.com volces.com volcengineapi.com volccdn.com api.map.baidu.com lc.map.baidu.com newvector.map.baidu.com route.map.baidu.com newclient.map.baidu.com tracknavi.baidu.com itsmap3.baidu.com app.navi.baidu.com mapapip0.bdimg.com mapapisp0.bdimg.com automap0.bdimg.com baidunavi.cdn.bcebos.com lbsnavi.cdn.bcebos.com enlargeroad-view.su.bcebos.com',
    block: 'tesla.cn tesla.com teslamotors.com tesla.services'
  },
  bl_telemetry: { allow:'', block:'hermes-stream-prd.vn.cloud.tesla.cn vehicle-files.prd.cnn1.vn.cloud.tesla.cn vehicle-files.prd.cn1.vn.cloud.tesla.cn firmware.tesla.cn' },
  clear: { allow:'', block:'' }
};
var PRESET_IDS = { tesla_min:'pstTesla', bl_telemetry:'pstBl', clear:'pstClr' };
function normList(s){ return String(s||'').trim().split(/\s+/).filter(Boolean).sort().join(' '); }
function detectActivePreset(allow, block){
  var a = normList(allow), b = normList(block);
  for(var k in DNS_PRESETS){
    if(normList(DNS_PRESETS[k].allow) === a && normList(DNS_PRESETS[k].block) === b) return k;
  }
  return null;
}
function highlightPreset(allow, block){
  var active = detectActivePreset(allow, block);
  for(var k in PRESET_IDS){
    var el = document.getElementById(PRESET_IDS[k]);
    if(el) el.classList.toggle('active', k === active);
  }
}
function dnsPreset(name){
  var p = DNS_PRESETS[name];
  if(!p) return;
  var qs = '?dnsAllow='+encodeURIComponent(p.allow)+'&dnsBlock='+encodeURIComponent(p.block);
  if(tok) qs += '&token='+encodeURIComponent(tok);
  fetch('/api/wifi-bridge/set'+qs)
    .then(function(r){if(r.status===200){toast('预设已应用','ok');brPoll()}else{toast('失败','err')}});
}
// ───── 添加 / 删除上联热点 ─────
function brAdd(){
  var ssid = document.getElementById('brAddSsid').value.trim();
  var pass = document.getElementById('brAddPass').value;
  var msg = document.getElementById('brAddMsg');
  if(!ssid){ msg.style.display=''; msg.textContent='⚠️ 请填写 SSID'; msg.style.color='#fca5a5'; return; }
  if(ssid.length > 32 || pass.length > 63){ msg.style.display=''; msg.textContent='⚠️ SSID ≤32 / 密码 ≤63'; msg.style.color='#fca5a5'; return; }
  var qs = 'ssid='+encodeURIComponent(ssid)+'&pass='+encodeURIComponent(pass);
  if(tok) qs += '&token='+encodeURIComponent(tok);
  fetch('/api/wifi-bridge/add?'+qs).then(function(r){
    if(r.status===200){
      msg.style.display=''; msg.textContent='✓ 已添加'; msg.style.color='#34d399';
      document.getElementById('brAddSsid').value = '';
      document.getElementById('brAddPass').value = '';
      brPoll();
    } else if(r.status===403){ showPin(); }
    else r.text().then(function(t){ msg.style.display=''; msg.textContent='✘ '+(t||'失败'); msg.style.color='#fca5a5'; });
  });
}
function brDelConfirm(ssid){
  // 展开当前行的内联确认（绿/红按钮），避免 native confirm
  var row = document.querySelector('[data-del-ssid="'+ssid.replace(/"/g,'&quot;')+'"]');
  if(row) row.style.display = '';
}
function brDelCancel(ssid){
  var row = document.querySelector('[data-del-ssid="'+ssid.replace(/"/g,'&quot;')+'"]');
  if(row) row.style.display = 'none';
}
function brDelDo(ssid){
  fetch('/api/wifi-bridge/delete?ssid='+encodeURIComponent(ssid)+(tok?'&token='+encodeURIComponent(tok):''))
    .then(function(r){
      if(r.status===200){ toast('已删除','ok'); brPoll(); }
      else if(r.status===403){ showPin(); }
      else { toast('删除失败','err'); }
    });
}

// ───── 操作 ─────
function showRebootConfirm(){ document.getElementById('rebootConfirmBox').style.display=''; }
function hideRebootConfirm(){ document.getElementById('rebootConfirmBox').style.display='none'; }
function showFactoryConfirm(){ document.getElementById('factoryConfirmBox').style.display=''; }
function hideFactoryConfirm(){ document.getElementById('factoryConfirmBox').style.display='none'; }
function doReboot(){
  hideRebootConfirm();
  fetch('/api/reboot'+authSuffix()).then(function(){toast('正在重启...','ok')});
}
function doFactory(){
  hideFactoryConfirm();
  var fd = new FormData(); fd.append('confirm','YES');
  fetch('/api/reset'+(tok?'?token='+encodeURIComponent(tok):''),{method:'POST',body:fd})
    .then(function(){toast('出厂重置中...','ok')});
}
function openPhone(){location.href='/?ui=phone'}

// ───── 诊断日志 ─────
function logRefresh(){
  var el = document.getElementById('diagLog');
  fetch('/api/log'+authSuffix())
    .then(function(r){ if(r.status===403){ showPin(); return ''; } return r.text(); })
    .then(function(txt){ if(txt==null) return; el.value = txt || '(暂无日志)'; el.scrollTop = el.scrollHeight; })
    .catch(function(){ toast('日志加载失败','err'); });
}
function logCopy(){
  var el = document.getElementById('diagLog');
  if(navigator.clipboard && navigator.clipboard.writeText){
    navigator.clipboard.writeText(el.value).then(
      function(){ toast('日志已复制','ok'); },
      function(){ el.select(); document.execCommand('copy'); toast('已选中，请手动复制','ok'); }
    );
  } else {
    el.select(); document.execCommand('copy'); toast('已选中，请手动复制','ok');
  }
}
function showLogClear(){ document.getElementById('logClearBox').style.display=''; }
function hideLogClear(){ document.getElementById('logClearBox').style.display='none'; }
function logClear(){
  hideLogClear();
  fetch('/api/log/clear'+(tok?'?token='+encodeURIComponent(tok):''), {method:'POST'})
    .then(function(r){ if(r.status===200){ toast('日志已清除','ok'); logRefresh(); } else if(r.status===403){ showPin(); } else toast('清除失败','err'); });
}

// ───── OTA ─────
var otaSelected = null;
function otaPick(){ document.getElementById('otaFile').click() }
document.getElementById('otaFile').addEventListener('change', function(e){
  var f = e.target.files[0];
  if(!f) return;
  otaSelected = f;
  document.getElementById('otaFname').textContent = f.name + ' ('+Math.round(f.size/1024)+' KB)';
  var btn = document.getElementById('otaUp');
  btn.disabled = false; btn.style.opacity = 1;
});
function showOtaConfirm(){
  if(!otaSelected) return;
  document.getElementById('otaConfirmName').textContent = otaSelected.name;
  document.getElementById('otaConfirmBox').style.display = '';
}
function hideOtaConfirm(){ document.getElementById('otaConfirmBox').style.display = 'none'; }
function otaUpload(){
  if(!otaSelected) return;
  hideOtaConfirm();
  var btn = document.getElementById('otaUp');
  var box = document.getElementById('progBox');
  var bar = document.getElementById('progBar');
  function restore(){ btn.disabled=false; btn.style.opacity=1; box.style.display='none'; bar.style.width='0%'; }
  btn.disabled = true; btn.style.opacity = .5;
  box.style.display = 'block'; bar.style.width = '0%';
  var fd = new FormData();
  fd.append('update', otaSelected, otaSelected.name);
  var xhr = new XMLHttpRequest();
  xhr.open('POST', '/api/ota'+(tok?'?token='+encodeURIComponent(tok):''));
  xhr.upload.onprogress = function(e){
    if(e.lengthComputable) bar.style.width = (e.loaded/e.total*100)+'%';
  };
  xhr.onload = function(){
    if(xhr.status===200){
      toast('升级成功，模块重启中...','ok');
      setTimeout(function(){location.reload()}, 8000);
    } else {
      toast('升级失败: '+(xhr.responseText||xhr.status),'err');
      restore();
    }
  };
  xhr.onerror = function(){ toast('网络错误','err'); restore(); };
  xhr.onabort = function(){ restore(); };
  xhr.send(fd);
}

// ───── 键盘避让：输入框获得焦点时滚动到视口中央，避免被软键盘挡住 ─────
document.addEventListener('focusin', function(e){
  var t = e.target;
  if (!t || !(t.matches && t.matches('input,textarea'))) return;
  setTimeout(function(){ try { t.scrollIntoView({block:'center',behavior:'smooth'}); } catch(_){} }, 300);
});

// ───── 启动 ─────
poll();
brPoll();
pollT = setInterval(poll, 2000);
setInterval(brPoll, 4000);
</script>
</body>
</html>)rawliteral";
