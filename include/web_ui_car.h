#pragma once
// ── Tesla 车机横屏 UI — /car ──────────────────────────────────────────────────
// 目标：Tesla 车机浏览器（AMD Ryzen/Intel MCU），1920×1200 横屏触控。
// 特点：大字号（16~28px 基线）、大按钮（≥56px）、左导航 + 主内容、最少打字。
// Auth：复用 sessionStorage 'fsd_tok'；403 时回 / 输密码。
// API：完全复用 /api/status、/api/set、/api/reboot、/api/ota、/api/wifi-bridge/*。
//
// Tabs：仪表 / 控制 / CAN / 网络 / DNS / OTA / 测速 / 日志 / 设置（9 个）

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
.gear-big{font-size:260px;font-weight:900;line-height:.82;font-family:-apple-system,'SF Pro Display',system-ui,sans-serif;letter-spacing:-16px;color:#64748b;transform-origin:center center;transition:color .35s, transform .45s cubic-bezier(.2,.8,.2,1);text-shadow:0 6px 80px currentColor;display:inline-block;will-change:transform}
.cluster.driving .gear-big{transform:scale(.37)}
.gear-big.P{color:#60a5fa}
.gear-big.R{color:#f87171}
.gear-big.N{color:#94a3b8}
.gear-big.D{color:#34d399}
.gear-big.pop{animation:gear-pop .55s cubic-bezier(.2,1.3,.3,1)}
.cluster.driving .gear-big.pop{animation:gear-pop-small .5s cubic-bezier(.2,1.25,.3,1)}
@keyframes gear-pop{0%{transform:scale(.55);opacity:.4}55%{transform:scale(1.2);opacity:1}100%{transform:scale(1);opacity:1}}
@keyframes gear-pop-small{0%{transform:scale(.2);opacity:.4}55%{transform:scale(.44);opacity:1}100%{transform:scale(.37);opacity:1}}
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
.speedo-arc{position:absolute;inset:0;width:100%;height:100%;overflow:visible;filter:drop-shadow(0 0 36px rgba(56,189,248,.55));transition:filter .5s}
.speedo-arc circle{transition:stroke-dasharray .55s cubic-bezier(.3,.9,.3,1),stroke .8s ease}
.cluster.warn .speedo-arc{filter:drop-shadow(0 0 44px rgba(239,68,68,.9));animation:arc-warn 1s ease-in-out infinite}
.cluster.warn .spd-huge{color:#fecaca;text-shadow:0 4px 90px rgba(239,68,68,.85)}
@keyframes arc-warn{0%,100%{filter:drop-shadow(0 0 44px rgba(239,68,68,.9))}50%{filter:drop-shadow(0 0 70px rgba(239,68,68,1))}}
.speedo-inner{position:absolute;inset:13%;display:flex;flex-direction:column;align-items:center;justify-content:center;text-align:center;background:radial-gradient(circle at 50% 55%,rgba(15,23,42,.55) 0%,rgba(11,17,32,.2) 55%,transparent 72%);border-radius:50%}
.spd-huge{font-size:260px;font-weight:900;line-height:.8;letter-spacing:-8px;color:#f8fafc;font-variant-numeric:tabular-nums;font-family:-apple-system,'SF Pro Display',system-ui,sans-serif;text-shadow:0 4px 80px rgba(56,189,248,.35);will-change:transform;transition:text-shadow .3s, font-size .3s}
.spd-huge.big3{font-size:220px;letter-spacing:-6px}
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
/* ── 测速页 ────────────────────────────────────────────────── */
.perf-speed{background:#131d32;border-radius:16px;padding:24px 20px;margin-bottom:18px;text-align:center;position:relative;overflow:hidden}
.perf-speed-val{font-size:96px;font-weight:900;color:#38bdf8;line-height:1;letter-spacing:-4px;font-variant-numeric:tabular-nums;transition:color .3s}
.perf-speed-val.amber{color:#f59e0b}
.perf-speed-val.red{color:#ef4444}
.perf-speed-lbl{font-size:14px;color:#64748b;letter-spacing:4px;margin-top:8px;font-weight:700}
.perf-card{background:#131d32;border:2px solid transparent;border-radius:16px;padding:24px;margin-bottom:16px;transition:border-color .4s,box-shadow .4s}
.perf-card.done{border-color:#22c55e;box-shadow:0 0 28px rgba(34,197,94,.28)}
.perf-head{display:flex;align-items:center;justify-content:space-between;margin-bottom:16px}
.perf-title{font-size:22px;font-weight:700;color:#e2e8f0;letter-spacing:1px}
.perf-badge{font-size:14px;font-weight:700;padding:6px 16px;border-radius:20px;letter-spacing:2px;transition:all .3s}
.perf-badge.idle{background:#1e293b;color:#64748b}
.perf-badge.armed{background:#3d2f00;color:#f59e0b}
.perf-badge.running{background:#0d2d1a;color:#22c55e;animation:perf-blink .7s ease-in-out infinite}
.perf-badge.done{background:#0d2d4f;color:#38bdf8}
@keyframes perf-blink{0%,100%{opacity:1}50%{opacity:.4}}
.perf-bar{height:6px;background:#1e293b;border-radius:3px;overflow:hidden;margin-bottom:18px}
.perf-fill{height:100%;width:0%;border-radius:3px;transition:width .1s linear}
.perf-fill.accel{background:linear-gradient(90deg,#38bdf8,#22c55e)}
.perf-fill.brake{background:linear-gradient(90deg,#f59e0b,#ef4444)}
.perf-result{display:flex;align-items:baseline;gap:10px;min-height:68px;margin-bottom:20px;flex-wrap:wrap}
.perf-val{font-size:60px;font-weight:800;color:#f8fafc;font-variant-numeric:tabular-nums;line-height:1}
.perf-unit{font-size:20px;color:#64748b;font-weight:500}
.perf-entry{font-size:14px;color:#94a3b8;background:#1e293b;border-radius:6px;padding:5px 12px;align-self:center;margin-left:6px}
.perf-actions{display:flex;gap:14px}
.perf-arm{flex:1;height:76px;background:#c41530;color:#fff;border:none;border-radius:12px;font-size:22px;font-weight:700;letter-spacing:3px;transition:background .2s,opacity .2s}
.perf-arm:active:not(:disabled){transform:scale(.98)}
.perf-arm:disabled{opacity:.35;cursor:not-allowed}
.perf-reset{flex:0 0 140px;height:76px;background:#1e293b;color:#94a3b8;border:2px solid #334155;border-radius:12px;font-size:18px;font-weight:700;letter-spacing:2px}
.perf-reset:active{transform:scale(.98)}
.perf-hist-head{display:flex;align-items:center;justify-content:space-between;margin:18px 2px 8px}
.perf-hist-t{font-size:14px;color:#64748b;font-weight:700;letter-spacing:3px}
.perf-hist-clr{font-size:13px;color:#475569;cursor:pointer;background:none;border:none;padding:4px 8px}
.perf-hist-clr:active{color:#94a3b8}
.perf-hist-row{display:flex;align-items:center;justify-content:space-between;padding:12px 16px;background:#0f172a;border-radius:10px;margin-bottom:6px;border-left:4px solid transparent}
.perf-hist-row.accel{border-left-color:#38bdf8}
.perf-hist-row.brake{border-left-color:#f59e0b}
.perf-hist-ts{font-size:13px;color:#64748b}
.perf-hist-v{font-size:22px;font-weight:800;color:#f8fafc}
.perf-hist-u{font-size:14px;color:#94a3b8;margin-left:4px}
.perf-hist-e{font-size:12px;color:#94a3b8;background:#1e293b;border-radius:5px;padding:4px 10px;margin-left:10px}
.perf-hist-empty{font-size:14px;color:#334155;text-align:center;padding:16px}
.perf-disc{background:#3d2f00;border:2px solid #f59e0b;border-radius:14px;padding:18px 22px;margin-bottom:18px;color:#fde68a}
.perf-disc-t{font-size:18px;font-weight:700;margin-bottom:10px;color:#fbbf24;letter-spacing:1px}
.perf-disc-b{font-size:14px;line-height:1.9;margin-bottom:14px}
.perf-disc-b b{color:#fff;font-weight:700}
.perf-disc-btn{background:#f59e0b;color:#0b1120;border:none;border-radius:10px;padding:12px 22px;font-size:14px;font-weight:700;letter-spacing:2px}
.perf-disc-btn:active{transform:scale(.98)}
.perf-note{font-size:13px;color:#475569;text-align:center;padding:10px 0 4px}
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
  .spd-huge{font-size:160px}
  .spd-huge.big3{font-size:130px}
  .spd-unit{font-size:18px;letter-spacing:6px}
  .limit-sign{width:96px;height:96px;border-width:7px}
  .limit-num{font-size:38px}
  .speedo-wrap{max-width:720px}
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
    <button class="nbtn" data-page="perf"><span class="ic">🏁</span>测速</button>
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
                  <stop offset="0%" stop-color="#22c55e"/>
                  <stop offset="30%" stop-color="#38bdf8"/>
                  <stop offset="50%" stop-color="#818cf8"/>
                  <stop offset="65%" stop-color="#f472b6"/>
                  <stop offset="75%" stop-color="#ef4444"/>
                  <stop offset="100%" stop-color="#b91c1c"/>
                </linearGradient>
                <linearGradient id="arcBg" x1="0" y1="1" x2="1" y2="0">
                  <stop offset="0%" stop-color="rgba(30,41,59,.55)"/>
                  <stop offset="100%" stop-color="rgba(51,65,85,.85)"/>
                </linearGradient>
              </defs>
              <circle cx="50" cy="50" r="45" fill="none" stroke="url(#arcBg)" stroke-width="4" stroke-dasharray="212.06 283" transform="rotate(135 50 50)" stroke-linecap="round"/>
              <circle id="arcFill" cx="50" cy="50" r="45" fill="none" stroke="url(#arcGrad)" stroke-width="4.5" stroke-dasharray="0 283" transform="rotate(135 50 50)" stroke-linecap="round"/>
            </svg>
            <div class="speedo-inner">
              <div class="spd-huge" id="vSpeed">0</div>
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
        <div class="row"><div class="rlbl">自动突破</div><div class="tog" id="tgAuto" data-k="hw3AutoSpeed"></div><div class="rval">&lt;80 km/h 限速时自动拉到 64/85/100 目标</div></div>
        <div class="row"><div class="rlbl">自定义</div><div class="tog" id="tgCustom" data-k="hw3CustomSpeed"></div><div class="rval">按档位设定目标速度（覆盖自动）</div></div>
        <div id="rowHw3Custom" style="display:none;padding:14px;background:rgba(0,0,0,.25);border-radius:10px;margin-top:6px">
          <div style="font-size:14px;color:#94a3b8;margin-bottom:10px">遇到限速时的目标速度（km/h）。≥80 限速始终透传原厂。</div>
          <div style="display:grid;grid-template-columns:repeat(5,1fr);gap:10px">
            <div><div style="font-size:13px;color:#64748b;text-align:center;margin-bottom:4px">限速 30</div><input type="number" id="hw3CT0" min="30" max="200" style="width:100%;box-sizing:border-box;padding:10px;text-align:center;background:#0f172a;color:#e2e8f0;border:1px solid #334155;border-radius:6px;font-size:18px" onchange="apiSetHw3CT('hw3CT0',this.value)"></div>
            <div><div style="font-size:13px;color:#64748b;text-align:center;margin-bottom:4px">限速 40</div><input type="number" id="hw3CT1" min="40" max="200" style="width:100%;box-sizing:border-box;padding:10px;text-align:center;background:#0f172a;color:#e2e8f0;border:1px solid #334155;border-radius:6px;font-size:18px" onchange="apiSetHw3CT('hw3CT1',this.value)"></div>
            <div><div style="font-size:13px;color:#64748b;text-align:center;margin-bottom:4px">限速 50</div><input type="number" id="hw3CT2" min="50" max="200" style="width:100%;box-sizing:border-box;padding:10px;text-align:center;background:#0f172a;color:#e2e8f0;border:1px solid #334155;border-radius:6px;font-size:18px" onchange="apiSetHw3CT('hw3CT2',this.value)"></div>
            <div><div style="font-size:13px;color:#64748b;text-align:center;margin-bottom:4px">限速 60</div><input type="number" id="hw3CT3" min="60" max="200" style="width:100%;box-sizing:border-box;padding:10px;text-align:center;background:#0f172a;color:#e2e8f0;border:1px solid #334155;border-radius:6px;font-size:18px" onchange="apiSetHw3CT('hw3CT3',this.value)"></div>
            <div><div style="font-size:13px;color:#64748b;text-align:center;margin-bottom:4px">限速 70</div><input type="number" id="hw3CT4" min="70" max="200" style="width:100%;box-sizing:border-box;padding:10px;text-align:center;background:#0f172a;color:#e2e8f0;border:1px solid #334155;border-radius:6px;font-size:18px" onchange="apiSetHw3CT('hw3CT4',this.value)"></div>
          </div>
        </div>
        <div class="tip">🛈 ≥80 km/h 限速让原车 EAP 偏移接管；低速限速下按固定目标提速。自动/自定义互斥；两者都关 = 原厂透传。</div>
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
        <div class="row"><div class="rlbl">启用桥接</div><div class="tog" id="tgBrEn"></div><div class="rval" id="brEnMode">关闭</div></div>
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
      <div class="panel">
        <div class="ptitle">在线更新（GitHub）</div>
        <div class="info-grid">
          <div class="info-item"><div class="info-lbl">当前</div><div class="info-val" id="otaCur">--</div></div>
          <div class="info-item"><div class="info-lbl">最新</div><div class="info-val" id="otaLat">--</div></div>
          <div class="info-item"><div class="info-lbl">变体</div><div class="info-val" id="otaEnv">--</div></div>
        </div>
        <div class="actions" style="margin-top:14px">
          <button class="abtn blue" id="otaChkBtn" onclick="otaCheck()">🔍 检查更新</button>
          <button class="abtn" id="otaPullBtn" onclick="otaPull()" disabled style="opacity:.5">⬇️ 下载并安装</button>
        </div>
        <div class="progress" id="otaDlBox" style="display:none;margin-top:14px"><div class="progress-bar" id="otaDlBar" style="width:0%"></div></div>
        <div id="otaOnlineMsg" style="margin-top:10px;font-size:15px;color:#94a3b8;min-height:22px"></div>
        <div class="tip">🛈 需连接路由器（网络页 STA）才能联网检查和下载。</div>
      </div>
    </div>

    <!-- ───── 测速 ───── -->
    <div class="page" id="pg-perf">
      <h2>性能测试 · PERFORMANCE</h2>

      <div class="perf-disc" id="perfDisc">
        <div class="perf-disc-t">⚠️ 使用前须知</div>
        <div class="perf-disc-b">
          · 本测试<b>仅限封闭道路或专用场地</b>使用<br>
          · <b>严禁在公共道路、交通区域进行</b><br>
          · 测试结果受路面、坡度、温度等影响，<b>数据仅供参考</b>（精度 ±0.1 秒）<br>
          · 请确保测试区域安全，无行人及障碍物 · 所有风险由使用者自行承担
        </div>
        <button class="perf-disc-btn" onclick="perfDiscOk()">我已了解，在封闭场地使用</button>
      </div>

      <div class="perf-speed">
        <div class="perf-speed-val" id="perfSpd">--</div>
        <div class="perf-speed-lbl">KM/H</div>
      </div>

      <div class="perf-card" id="perfCardA">
        <div class="perf-head">
          <div class="perf-title">0 → 100 km/h</div>
          <span class="perf-badge idle" id="perfBadgeA">待命</span>
        </div>
        <div class="perf-bar"><div class="perf-fill accel" id="perfBarA"></div></div>
        <div class="perf-result">
          <span class="perf-val" id="perfValA">--</span>
          <span class="perf-unit" id="perfUnitA"></span>
        </div>
        <div class="perf-actions">
          <button class="perf-arm" id="perfArmA" onclick="perfArm('accel')">预 备</button>
          <button class="perf-reset" onclick="perfReset('accel')">重置</button>
        </div>
      </div>

      <div class="perf-card" id="perfCardB">
        <div class="perf-head">
          <div class="perf-title">100 → 0 km/h</div>
          <span class="perf-badge idle" id="perfBadgeB">待命</span>
        </div>
        <div class="perf-bar"><div class="perf-fill brake" id="perfBarB"></div></div>
        <div class="perf-result">
          <span class="perf-val" id="perfValB">--</span>
          <span class="perf-unit" id="perfUnitB"></span>
          <span class="perf-entry" id="perfEntryB" style="display:none"></span>
        </div>
        <div class="perf-actions">
          <button class="perf-arm" id="perfArmB" onclick="perfArm('brake')">预 备</button>
          <button class="perf-reset" onclick="perfReset('brake')">重置</button>
        </div>
      </div>

      <div class="perf-note">精度约 ±0.1 秒（受 CAN 10Hz 帧率限制）· 仅供参考 · 请在安全场地使用</div>

      <div class="perf-hist-head">
        <span class="perf-hist-t">0→100 历史</span>
        <button class="perf-hist-clr" onclick="perfClear('accel')">清除</button>
      </div>
      <div id="perfHistA"></div>

      <div class="perf-hist-head">
        <span class="perf-hist-t">100→0 历史</span>
        <button class="perf-hist-clr" onclick="perfClear('brake')">清除</button>
      </div>
      <div id="perfHistB"></div>
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
    if(p === 'ota'){
      fetch('/api/ota/status'+(tok?'?token='+encodeURIComponent(tok):''))
        .then(function(r){ return r.ok?r.json():null; })
        .then(function(j){ if(j) otaRenderOnline(j); })
        .catch(function(){});
    }
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
  if(Math.abs(diff) > 0.4){ v += diff * 0.45; }
  else { v = t; }
  window.__spdDisplay = v;
  var show = Math.round(v);
  var el = document.getElementById('vSpeed');
  if(el && el.textContent !== String(show)) el.textContent = show;
  if(el){
    var big3 = show >= 100;
    if(big3 !== !!window.__big3){ el.classList.toggle('big3', big3); window.__big3 = big3; }
  }
  var arcFill = document.getElementById('arcFill');
  if(arcFill){
    var pct = Math.min(1, Math.max(0, v/200));
    arcFill.setAttribute('stroke-dasharray', (212.06 * pct).toFixed(1) + ' 283');
  }
  var cl = document.querySelector('.cluster');
  if(cl){
    var drv = v >= 3;
    if(drv !== !!window.__drv){ cl.classList.toggle('driving', drv); window.__drv = drv; }
    var warn = v >= 150;
    if(warn !== !!window.__warn){ cl.classList.toggle('warn', warn); window.__warn = warn; }
  }
  // 测速页速度：复用同一 RAF 平滑值
  var pEl = document.getElementById('perfSpd');
  if(pEl){
    var pShow = show > 0 ? String(show) : '--';
    if(pEl.textContent !== pShow) pEl.textContent = pShow;
    var pCls = 'perf-speed-val' + (show >= 100 ? ' red' : show >= 60 ? ' amber' : '');
    if(pEl.className !== pCls) pEl.className = pCls;
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
  var spd = Math.round(d.speedD / 10);
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
    // HW3/Legacy：显示 Tesla 原车偏移百分比（从 mux-0 解码的 hw3AutoOffset）
    var offPct = d.hw3AutoOffset >= 0 ? d.hw3AutoOffset : 0;
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
  setTog('tgAuto', d.hw3AutoSpeed==null ? true : !!d.hw3AutoSpeed);
  setTog('tgCustom', !!d.hw3CustomSpeed);
  var rowCust = document.getElementById('rowHw3Custom');
  if(rowCust) rowCust.style.display = (d.hwMode===1 && d.hw3CustomSpeed) ? '' : 'none';
  if(!window._hw3CTLoaded && Array.isArray(d.hw3CustomTarget)){
    for(var _i=0;_i<5;_i++){var _e=document.getElementById('hw3CT'+_i); if(_e) _e.value=d.hw3CustomTarget[_i];}
    window._hw3CTLoaded=true;
  }
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
  document.getElementById('panelHw3').style.display = isHW3 ? 'block' : 'none';
  document.getElementById('panelHw4').style.display = isHW4 ? 'block' : 'none';
  // 模式专属开关行
  document.getElementById('rowEmerg').style.display = isHW4 ? '' : 'none';
  document.getElementById('rowOvr').style.display = isLegacy ? '' : 'none';
  document.getElementById('rowTrack').style.display = isHW3 ? '' : 'none';
  // HW4 偏移 pill 选中
  document.querySelectorAll('#grpHw4Off .pill').forEach(function(b){
    b.classList.toggle('active', parseInt(b.dataset.hw4)===d.hw4Offset);
  });

  // CAN 页
  document.getElementById('cRx').textContent = d.rx;
  document.getElementById('cMod').textContent = d.modified;
  document.getElementById('cErr').textContent = d.errors;
  // CAN 速率：按 rx 差值 / 时间差估算（客户端计算，后端不维护）
  var now = Date.now();
  if(window.__rxPrev !== undefined && window.__rxPrevT){
    var dt = (now - window.__rxPrevT) / 1000;
    var dr = d.rx - window.__rxPrev;
    if(dt > 0 && dr >= 0){
      var hz = Math.round(dr / dt);
      document.getElementById('cRate').textContent = hz;
    }
  } else {
    document.getElementById('cRate').textContent = '--';
  }
  window.__rxPrev = d.rx; window.__rxPrevT = now;
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

  // 测速页：若加载了则同步状态
  perfRender(d);
}

// ───── 测速 ─────
var PERF_BADGE = ['idle','armed','running','done'];
var PERF_TXT = ['待命','已备战','计时中','完成'];
var perfPrev = {accel:-1, brake:-1};  // -1: 首次 poll 用当前值初始化，避免页面刷新时把已完成状态当新完成重复入库
var perfAnimDone = {accel:false, brake:false};
var perfBrakeEntry = 0;
function perfAck(){ try{ return sessionStorage.getItem('perf_disc_ok') === '1'; }catch(e){ return false; } }
function perfDiscOk(){
  try{ sessionStorage.setItem('perf_disc_ok','1'); }catch(e){}
  document.getElementById('perfDisc').style.display = 'none';
  // 恢复按钮可用（当前 state 若非 running/done）
  if(last) perfApplyState('accel', last.perfAccel||0, last.perfAccelMs||0, Math.round((last.speedD||0)/10));
  if(last) perfApplyState('brake', last.perfBrake||0, last.perfBrakeMs||0, Math.round((last.speedD||0)/10));
}
function perfCountUp(elId, targetSec, duration){
  var el = document.getElementById(elId);
  var start = performance.now();
  function step(now){
    var p = Math.min((now-start)/duration, 1);
    var e = 1 - Math.pow(1-p, 3);
    el.textContent = (targetSec*e).toFixed(2);
    if(p < 1) requestAnimationFrame(step);
    else el.textContent = targetSec.toFixed(2);
  }
  requestAnimationFrame(step);
}
function perfFlash(id){
  var c = document.getElementById(id);
  c.classList.add('done');
  setTimeout(function(){ c.classList.remove('done'); }, 1500);
}
function perfApplyState(which, state, ms, spd){
  var isA = (which === 'accel');
  var badge = document.getElementById(isA ? 'perfBadgeA' : 'perfBadgeB');
  var val = document.getElementById(isA ? 'perfValA' : 'perfValB');
  var unit = document.getElementById(isA ? 'perfUnitA' : 'perfUnitB');
  var bar = document.getElementById(isA ? 'perfBarA' : 'perfBarB');
  var arm = document.getElementById(isA ? 'perfArmA' : 'perfArmB');
  badge.className = 'perf-badge ' + (PERF_BADGE[state] || 'idle');
  badge.textContent = PERF_TXT[state] || '--';
  if(state === 2){
    var pct = 0;
    if(isA) pct = Math.min(spd, 100);
    else pct = Math.min(100, Math.max(0, perfBrakeEntry > 0 ? (1 - spd/perfBrakeEntry)*100 : 0));
    bar.style.width = pct + '%';
  } else if(state === 3){
    bar.style.width = '100%';
  } else {
    bar.style.width = '0%';
  }
  arm.disabled = (state === 2 || state === 3) || !perfAck();
  if(state === 3 && ms > 0){
    unit.textContent = '秒';
    if(!perfAnimDone[which]){
      perfCountUp(isA ? 'perfValA' : 'perfValB', ms/1000, 900);
      perfFlash(isA ? 'perfCardA' : 'perfCardB');
      perfAnimDone[which] = true;
      if(!isA){
        var et = document.getElementById('perfEntryB');
        if(perfBrakeEntry > 0){ et.textContent = '进入 ' + perfBrakeEntry + ' km/h'; et.style.display = ''; }
      }
    }
  } else if(state === 2){
    val.textContent = '...';
    unit.textContent = '';
  } else {
    val.textContent = '--';
    unit.textContent = '';
    perfAnimDone[which] = false;
    if(!isA) document.getElementById('perfEntryB').style.display = 'none';
  }
}
function perfRender(d){
  if(!d) return;
  var spd = Math.round((d.speedD||0)/10);
  perfBrakeEntry = d.brakeEntryKph || 0;
  var na = d.perfAccel||0, nb = d.perfBrake||0;
  // 首次 poll：用当前状态初始化 prev，避免页面刷新时对已有 DONE 状态重复入库
  if(perfPrev.accel === -1){
    perfPrev.accel = na;
    if(na === 3){ perfAnimDone.accel = true; }  // 跳过动画，避免刷新后弹结果
  }
  if(perfPrev.brake === -1){
    perfPrev.brake = nb;
    if(nb === 3){ perfAnimDone.brake = true; }
  }
  if(perfPrev.accel !== 3 && na === 3 && d.perfAccelMs > 0) perfSave('accel', d.perfAccelMs, 0);
  if(perfPrev.brake !== 3 && nb === 3 && d.perfBrakeMs > 0) perfSave('brake', d.perfBrakeMs, d.brakeEntryKph||0);
  perfApplyState('accel', na, d.perfAccelMs||0, spd);
  perfApplyState('brake', nb, d.perfBrakeMs||0, spd);
  perfPrev.accel = na; perfPrev.brake = nb;
  // 若已 DONE 且跳过了动画，把结果直接写上
  if(na === 3 && d.perfAccelMs > 0 && document.getElementById('perfValA').textContent === '--'){
    document.getElementById('perfValA').textContent = (d.perfAccelMs/1000).toFixed(2);
    document.getElementById('perfUnitA').textContent = '秒';
  }
  if(nb === 3 && d.perfBrakeMs > 0 && document.getElementById('perfValB').textContent === '--'){
    document.getElementById('perfValB').textContent = (d.perfBrakeMs/1000).toFixed(2);
    document.getElementById('perfUnitB').textContent = '秒';
    var et = document.getElementById('perfEntryB');
    if(perfBrakeEntry > 0){ et.textContent = '进入 '+perfBrakeEntry+' km/h'; et.style.display = ''; }
  }
}
function perfArm(which){
  if(!perfAck()){ toast('请先确认使用须知', 'err'); return; }
  fetch('/api/perf?cmd=arm_'+which+(tok?'&token='+encodeURIComponent(tok):''))
    .then(function(r){return r.json();}).then(function(){ poll(); });
}
function perfReset(which){
  perfAnimDone[which] = false;
  if(which === 'brake') document.getElementById('perfEntryB').style.display = 'none';
  fetch('/api/perf?cmd=reset_'+which+(tok?'&token='+encodeURIComponent(tok):''))
    .then(function(r){return r.json();}).then(function(){ poll(); });
}
function perfSave(type, ms, entry){
  var key = 'perf_hist_'+type;
  var arr = [];
  try{ arr = JSON.parse(localStorage.getItem(key)||'[]'); }catch(e){}
  arr.unshift({ts:Date.now(), ms:ms, entry:entry||0});
  if(arr.length > 10) arr = arr.slice(0, 10);
  try{ localStorage.setItem(key, JSON.stringify(arr)); }catch(e){}
  perfRenderHist(type);
}
function perfClear(type){
  try{ localStorage.removeItem('perf_hist_'+type); }catch(e){}
  perfRenderHist(type);
}
function perfFmtTs(ts){
  var d = new Date(ts);
  var p = function(n){ return n<10?'0'+n:''+n; };
  return p(d.getMonth()+1)+'/'+p(d.getDate())+' '+p(d.getHours())+':'+p(d.getMinutes())+':'+p(d.getSeconds());
}
function perfRenderHist(type){
  var listEl = document.getElementById(type==='accel' ? 'perfHistA' : 'perfHistB');
  if(!listEl) return;
  var arr = [];
  try{ arr = JSON.parse(localStorage.getItem('perf_hist_'+type)||'[]'); }catch(e){}
  if(arr.length === 0){ listEl.innerHTML = '<div class="perf-hist-empty">暂无记录</div>'; return; }
  var html = '';
  for(var i=0;i<arr.length;i++){
    var r = arr[i];
    html += '<div class="perf-hist-row '+type+'">';
    html += '<span class="perf-hist-ts">'+perfFmtTs(r.ts)+'</span>';
    html += '<span><span class="perf-hist-v">'+(r.ms/1000).toFixed(2)+'</span><span class="perf-hist-u">秒</span>';
    if(type === 'brake' && r.entry > 0) html += '<span class="perf-hist-e">进入 '+r.entry+' km/h</span>';
    html += '</span></div>';
  }
  listEl.innerHTML = html;
}
// 初始化：若本会话已确认过免责声明，隐藏
try{ if(sessionStorage.getItem('perf_disc_ok') === '1'){
  var _pd = document.getElementById('perfDisc'); if(_pd) _pd.style.display = 'none';
} }catch(e){}
perfRenderHist('accel');
perfRenderHist('brake');

function setTog(id, on){
  var t = document.getElementById(id);
  if(!t) return;
  t.classList.toggle('on', !!on);
}

// ───── 开关点击 ─────
var TRACK_WARN = '⚠️ 实验性功能，效果未经完全验证。开启后将向总线持续注入赛道模式请求，可能影响车辆稳定控制。请知悉风险后开启。';
// Turning a paired toggle on forces its sibling off. Each side lists sibling's {key, togId}.
var TOG_MUTEX = {hw3AutoSpeed:{k:'hw3CustomSpeed',id:'tgCustom'}, hw3CustomSpeed:{k:'hw3AutoSpeed',id:'tgAuto'}};
function setTogCustomPanel(on){
  var r = document.getElementById('rowHw3Custom');
  if(r) r.style.display = on ? '' : 'none';
}
document.querySelectorAll('.tog[data-k]').forEach(function(t){
  t.onclick = function(){
    var k = t.dataset.k;
    var newVal = !t.classList.contains('on');
    if(k==='trackMode' && newVal && !confirm(TRACK_WARN)) return;
    t.classList.toggle('on', newVal);
    apiSet(k, newVal?1:0);
    if(newVal && TOG_MUTEX[k]){
      var sib = TOG_MUTEX[k], s = document.getElementById(sib.id);
      if(s && s.classList.contains('on')){ s.classList.remove('on'); apiSet(sib.k, 0); }
    }
    if(k==='hw3CustomSpeed') setTogCustomPanel(newVal);
    else if(k==='hw3AutoSpeed' && newVal) setTogCustomPanel(false);
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
// 桥接启用开关（upstreamEnable）
document.getElementById('tgBrEn').onclick = function(){
  var t = this;
  var newVal = !t.classList.contains('on');
  t.classList.toggle('on', newVal);
  document.getElementById('brEnMode').textContent = newVal?'启用':'关闭';
  var qs = '?upstreamEnable='+(newVal?1:0) + (tok?'&token='+encodeURIComponent(tok):'');
  fetch('/api/wifi-bridge/set'+qs)
    .then(function(r){if(r.status===200){toast('WiFi 桥接 '+(newVal?'已启用':'已关闭'), 'ok');brPoll();}else toast('失败','err')});
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
    // 桥接启用开关同步
    var tgEn = document.getElementById('tgBrEn');
    tgEn.classList.toggle('on', !!d.upstreamEnable);
    document.getElementById('brEnMode').textContent = d.upstreamEnable ? '启用' : '关闭';
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

document.querySelectorAll('#grpHw4Off .pill').forEach(function(b){
  b.onclick = function(){apiSet('hw4Offset', b.dataset.hw4)};
});

// ───── apiSet ─────
function apiSet(k, v){
  fetch('/api/set?'+k+'='+v+(tok?'&token='+encodeURIComponent(tok):''))
    .then(function(r){if(r.status===200){toast('已保存','ok');poll()}else if(r.status===403){showPin()}else{toast('失败','err')}});
}
// Debounced batch save for hw3CT* slots — 1 NVS commit per edit burst.
var _hw3CTTimer=null, _hw3CTPending={};
function apiSetHw3CT(k, v){
  _hw3CTPending[k]=v;
  clearTimeout(_hw3CTTimer);
  _hw3CTTimer=setTimeout(function(){
    var qs=''; for(var kk in _hw3CTPending){qs+='&'+kk+'='+encodeURIComponent(_hw3CTPending[kk]);}
    _hw3CTPending={};
    if(!qs)return;
    fetch('/api/set?'+qs.slice(1)+(tok?'&token='+encodeURIComponent(tok):''))
      .then(function(r){if(r.status===200){toast('已保存','ok');poll()}else if(r.status===403){showPin()}else{toast('失败','err')}});
  },400);
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

// ───── 在线 OTA（GitHub） ─────
var __otaOnlinePoll = null;
function otaRenderOnline(s){
  var cur = document.getElementById('otaCur');
  var lat = document.getElementById('otaLat');
  var env = document.getElementById('otaEnv');
  var msg = document.getElementById('otaOnlineMsg');
  var box = document.getElementById('otaDlBox');
  var bar = document.getElementById('otaDlBar');
  var chk = document.getElementById('otaChkBtn');
  var pull = document.getElementById('otaPullBtn');
  if(s.current) cur.textContent = 'v'+s.current;
  if(s.envTag)  env.textContent = s.envTag;
  function setBtn(b, on){ b.disabled=!on; b.style.opacity = on?1:.5; }
  if(s.state===1){ msg.textContent='正在查询 GitHub…'; msg.style.color='#94a3b8'; setBtn(chk,false); setBtn(pull,false); }
  else if(s.state===2){ msg.textContent='正在下载…'; msg.style.color='#38bdf8'; box.style.display='block'; setBtn(chk,false); setBtn(pull,false); }
  else if(s.state===3){ box.style.display='block'; var pct=s.total>0?Math.round(s.written/s.total*100):0; bar.style.width=pct+'%'; msg.textContent='正在写入 '+pct+'%'; msg.style.color='#38bdf8'; setBtn(chk,false); setBtn(pull,false); }
  else if(s.state===4){ if(s.latest) lat.textContent='v'+s.latest; msg.textContent=s.message||'已找到新版本'; msg.style.color='#22c55e'; setBtn(chk,true); setBtn(pull,!!s.url); if(__otaOnlinePoll){clearInterval(__otaOnlinePoll);__otaOnlinePoll=null;} }
  else if(s.state===5){ msg.textContent=s.message||'更新成功，正在重启…'; msg.style.color='#22c55e'; bar.style.width='100%'; setBtn(chk,false); setBtn(pull,false); if(__otaOnlinePoll){clearInterval(__otaOnlinePoll);__otaOnlinePoll=null;} setTimeout(function(){location.reload()}, 8000); }
  else if(s.state===6){ msg.textContent=s.message||'失败'; msg.style.color='#ef4444'; setBtn(chk,true); setBtn(pull,!!s.url); if(__otaOnlinePoll){clearInterval(__otaOnlinePoll);__otaOnlinePoll=null;} }
  else { setBtn(chk,true); setBtn(pull,!!s.url); }
}
function otaStartOnlinePoll(){
  if(__otaOnlinePoll) clearInterval(__otaOnlinePoll);
  var tick = function(){
    fetch('/api/ota/status'+(tok?'?token='+encodeURIComponent(tok):''))
      .then(function(r){ return r.ok?r.json():null; })
      .then(function(j){ if(j) otaRenderOnline(j); })
      .catch(function(){});
  };
  tick();
  __otaOnlinePoll = setInterval(tick, 1500);
}
function otaCheck(){
  var msg = document.getElementById('otaOnlineMsg');
  msg.textContent = ''; msg.style.color='#94a3b8';
  document.getElementById('otaChkBtn').disabled = true;
  fetch('/api/ota/check'+(tok?'?token='+encodeURIComponent(tok):''), {method:'POST'})
    .then(function(r){
      if(r.status===403){ showPin(); document.getElementById('otaChkBtn').disabled=false; return; }
      if(!r.ok){ msg.textContent='HTTP '+r.status; msg.style.color='#ef4444'; document.getElementById('otaChkBtn').disabled=false; return; }
      otaStartOnlinePoll();
    })
    .catch(function(){ msg.textContent='网络错误'; msg.style.color='#ef4444'; document.getElementById('otaChkBtn').disabled=false; });
}
function otaPull(){
  if(!confirm('确认下载并安装新版？期间请勿断电')) return;
  var msg = document.getElementById('otaOnlineMsg');
  msg.textContent = ''; msg.style.color='#94a3b8';
  document.getElementById('otaPullBtn').disabled = true;
  document.getElementById('otaChkBtn').disabled = true;
  fetch('/api/ota/pull'+(tok?'?token='+encodeURIComponent(tok):''), {method:'POST'})
    .then(function(r){
      if(r.status===403){ showPin(); return; }
      if(!r.ok){ msg.textContent='HTTP '+r.status; msg.style.color='#ef4444'; return; }
      otaStartOnlinePoll();
    })
    .catch(function(){ msg.textContent='网络错误'; msg.style.color='#ef4444'; });
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
pollT = setInterval(poll, 250);
setInterval(brPoll, 4000);
</script>
</body>
</html>)rawliteral";
