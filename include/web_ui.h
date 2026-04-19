#pragma once

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="zh">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1,user-scalable=no">
<title>Tesla Controller</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:-apple-system,system-ui,"PingFang SC","Microsoft YaHei",sans-serif;background:#0b1120;color:#e2e8f0;min-height:100vh;padding:16px}
.title-wrap{position:relative;text-align:center;padding:20px 0 24px}
h1{font-size:22px;color:#38bdf8;font-weight:700;letter-spacing:1px}
.lang-btn{position:absolute;right:0;top:50%;transform:translateY(-50%);background:#1e293b;color:#94a3b8;border:1px solid #334155;border-radius:8px;padding:5px 10px;font-size:12px;cursor:pointer;font-weight:600}
.lang-btn:hover{border-color:#38bdf8;color:#38bdf8}
.card{background:#131d32;border-radius:14px;padding:18px;margin-bottom:16px}
.card-title{font-size:12px;font-weight:700;color:#64748b;letter-spacing:2px;margin-bottom:14px}
.row{display:flex;align-items:center;justify-content:space-between;padding:12px 0;border-bottom:1px solid #1e293b}
.row:last-child{border-bottom:none}
.row-label{font-size:14px;font-weight:500}
.toggle,.switch{position:relative;display:inline-block;width:50px;height:28px}
.toggle input,.switch input{opacity:0;width:0;height:0}
.slider{position:absolute;cursor:pointer;top:0;left:0;right:0;bottom:0;background:#334155;border-radius:28px;transition:.3s}
.slider:before{content:"";position:absolute;height:22px;width:22px;left:3px;bottom:3px;background:#fff;border-radius:50%;transition:.3s}
input:checked+.slider{background:#22c55e}
input:checked+.slider:before{transform:translateX(22px)}
select{background:#1e293b;color:#e2e8f0;border:1px solid #334155;border-radius:8px;padding:8px 32px 8px 12px;font-size:13px;appearance:none;-webkit-appearance:none;background-image:url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' width='12' height='12' fill='%2394a3b8' viewBox='0 0 16 16'%3E%3Cpath d='M8 11L3 6h10z'/%3E%3C/svg%3E");background-repeat:no-repeat;background-position:right 10px center;min-width:110px;cursor:pointer}
select:focus{outline:none;border-color:#38bdf8}
.stats{display:grid;grid-template-columns:1fr 1fr;gap:10px;margin-bottom:14px}
.stat{background:#1a2740;border-radius:10px;padding:14px;text-align:center}
.stat-val{font-size:28px;font-weight:800;color:#38bdf8}
.stat-label{font-size:11px;color:#64748b;margin-top:2px;letter-spacing:1px}
.stat-val.green{color:#22c55e}
.stat-val.amber{color:#eab308}
.status-row{display:flex;justify-content:space-between;padding:10px 0;border-bottom:1px solid #1e293b;font-size:14px}
.status-row:last-child{border-bottom:none}
.status-ok{color:#22c55e;font-weight:700}
.status-err{color:#ef4444;font-weight:700}
.status-yes{color:#22c55e;font-weight:700}
.status-no{color:#64748b;font-weight:700}
.ota-row{display:flex;gap:10px;align-items:center;flex-wrap:wrap}
.file-btn{background:#1e293b;color:#94a3b8;border:1px solid #334155;border-radius:8px;padding:8px 14px;font-size:12px;cursor:pointer}
.file-name{font-size:12px;color:#64748b;flex:1}
.upload-btn{background:#e31937;color:#fff;border:none;border-radius:8px;padding:10px 0;font-size:14px;font-weight:600;cursor:pointer;width:100%;margin-top:10px;letter-spacing:1px}
.upload-btn:disabled{opacity:.4;cursor:not-allowed}
.upload-btn:hover:not(:disabled){background:#c41530}
.progress{width:100%;height:6px;background:#1e293b;border-radius:3px;margin-top:10px;display:none}
.progress-bar{height:100%;background:#22c55e;border-radius:3px;width:0%;transition:width .3s}
.msg{text-align:center;font-size:12px;margin-top:8px;min-height:16px}
.msg.ok{color:#22c55e}
.msg.err{color:#ef4444}
.text-input{background:#1e293b;color:#e2e8f0;border:1px solid #334155;border-radius:8px;padding:8px 12px;font-size:13px;width:100%;margin-top:4px}
.text-input:focus{outline:none;border-color:#38bdf8}
.save-btn{background:#22c55e;color:#fff;border:none;border-radius:8px;padding:10px 0;font-size:14px;font-weight:600;cursor:pointer;width:100%;margin-top:10px;letter-spacing:1px}
.save-btn:hover{background:#16a34a}
.save-btn:disabled{opacity:.4;cursor:not-allowed}
.hb-row .row-label{display:flex;flex-direction:column;gap:2px}
.hb-hint{font-size:11px;color:#64748b;font-weight:400}
.hb-hint.avail{color:#7a5a18}
.hb-row .toggle input:disabled+.slider{background:#1e293b;cursor:not-allowed}
.smart-rules{background:#0d1a2e;border-radius:10px;padding:12px 14px;margin-top:6px;border:1px solid #1e3a5f}
.smart-rule{display:flex;align-items:center;gap:6px;padding:6px 0;font-size:13px;color:#94a3b8;flex-wrap:wrap}
.smart-rule:not(:last-child){border-bottom:1px solid #1e293b}
.smart-rule input[type=number]{background:#1e293b;color:#e2e8f0;border:1px solid #334155;border-radius:6px;padding:5px 6px;font-size:13px;text-align:center;-moz-appearance:textfield}
.smart-rule input[type=number]::-webkit-inner-spin-button,.smart-rule input[type=number]::-webkit-outer-spin-button{-webkit-appearance:none}
.smart-rule input[type=number]:focus{outline:none;border-color:#38bdf8}
.smart-rule .km{color:#38bdf8;font-weight:600}
.stat-val.red{color:#ef4444}
.car-link{position:fixed;bottom:14px;right:14px;background:rgba(30,41,59,.9);color:#94a3b8;border:1px solid #334155;border-radius:20px;padding:8px 14px;font-size:12px;text-decoration:none;z-index:500;backdrop-filter:blur(6px)}
.car-link:active{background:#1e293b}
</style>
<script>
// 启发式车机检测：UA 不含 Tesla/QtCarBrowser 的 AMD 车机（Chromium）会落到这里。
// 触控屏 + 大视口 (≥1000×600) → 自动跳 /car；用户可用 /?ui=phone 强制手机版。
(function(){
  try{
    if(location.search.indexOf('ui=phone')!==-1) return;
    if(location.pathname!=='/'&&location.pathname!=='') return;
    var coarse=window.matchMedia&&matchMedia('(pointer:coarse)').matches;
    var touchLike=('ontouchstart' in window)||(navigator.maxTouchPoints>0);
    var wide=window.innerWidth>=1000&&window.innerHeight>=600;
    if((coarse||touchLike)&&wide){ location.replace('/car'); }
  }catch(_){}
})();
</script>
</head>
<body>
<a href="/car" class="car-link" title="切换到车机版">🚗 车机版</a>
<div id="disclaimer" style="position:fixed;top:0;left:0;right:0;bottom:0;background:rgba(0,0,0,0.88);z-index:999;display:none;align-items:center;justify-content:center;padding:20px">
  <div style="background:#131d32;border-radius:16px;padding:24px;max-width:360px;width:100%;border:1px solid #ef4444">
    <!-- 步骤1：免责声明 -->
    <div id="disclaimerContent">
      <div style="color:#ef4444;font-weight:700;font-size:16px;margin-bottom:14px">⚠️ 免责声明</div>
      <div style="font-size:13px;color:#94a3b8;line-height:1.8;margin-bottom:20px">
        本工具<b style="color:#e2e8f0">仅限技术探讨</b>，<b style="color:#ef4444">严禁用于实际道路行驶</b>。<br><br>
        操作涉及修改车辆 CAN 总线协议，可能导致：<br>
        · 硬件损毁<br>
        · 质量保修失效<br>
        · 人身及财产安全事故<br><br>
        <b style="color:#f59e0b">所有风险与法律责任由使用者自行承担，与开发者无关。</b>
      </div>
      <div id="disclaimerBtns">
        <button onclick="confirmDisclaimer()" style="background:#ef4444;color:#fff;border:none;border-radius:8px;padding:13px;width:100%;font-size:14px;font-weight:600;cursor:pointer;letter-spacing:1px;margin-bottom:10px">我已了解，仅用于技术探讨</button>
        <button onclick="rejectDisclaimer()" style="background:#1e293b;color:#94a3b8;border:1px solid #334155;border-radius:8px;padding:11px;width:100%;font-size:13px;cursor:pointer">不同意</button>
      </div>
      <div id="disclaimerRejected" style="display:none;text-align:center;color:#64748b;font-size:13px;padding:8px 0">您已拒绝，请关闭此页面。</div>
    </div>
    <!-- 步骤2：PIN 验证（设置了密码才显示）-->
    <div id="pinStep" style="display:none">
      <div style="color:#38bdf8;font-weight:700;font-size:16px;margin-bottom:14px">🔒 访问验证</div>
      <div style="color:#94a3b8;font-size:13px;margin-bottom:14px">请输入访问密码以继续</div>
      <input type="password" id="pinInput" maxlength="16" placeholder="访问密码"
        style="width:100%;background:#0b1120;border:1px solid #334155;border-radius:8px;padding:12px;color:#e2e8f0;font-size:15px;margin-bottom:10px;outline:none"
        onkeydown="if(event.key==='Enter')submitPin()">
      <button onclick="submitPin()" style="background:#38bdf8;color:#0b1120;border:none;border-radius:8px;padding:13px;width:100%;font-size:14px;font-weight:700;cursor:pointer;margin-bottom:8px">确认</button>
      <div id="pinError" style="display:none;color:#ef4444;font-size:12px;text-align:center">密码错误，请重试</div>
    </div>
  </div>
</div>

<div class="title-wrap">
  <h1 id="iTitle">特斯拉控制器</h1>
  <button class="lang-btn" id="iLangBtn" onclick="toggleLang()">EN</button>
</div>

<button onclick="location.href='/dash'" style="
  display:block;width:100%;margin-bottom:10px;padding:14px;
  background:linear-gradient(135deg,#0d4f4f,#0a7a6a);
  color:#00d4aa;border:1px solid #00d4aa44;border-radius:14px;
  font-size:15px;font-weight:700;cursor:pointer;letter-spacing:1px;
  text-align:center;
" id="iDashBtn">⬤ 仪表盘 · DASHBOARD</button>
<button onclick="location.href='/perf'" style="
  display:block;width:100%;margin-bottom:16px;padding:14px;
  background:linear-gradient(135deg,#2d1010,#7f1a1a);
  color:#ff6b6b;border:1px solid #ff6b6b44;border-radius:14px;
  font-size:15px;font-weight:700;cursor:pointer;letter-spacing:1px;
  text-align:center;
" id="iPerfBtn">⏱ 性能测试 · PERF TEST</button>

<div class="card" id="cardLive">
  <div class="card-title" id="iCardLive">实时摘要</div>
  <div class="stats">
    <div class="stat"><div class="stat-val" id="liveCAN">--</div><div class="stat-label" id="iLblLiveCAN">CAN</div></div>
    <div class="stat"><div class="stat-val" id="liveSpeed">--</div><div class="stat-label" id="iLblLiveSpeed">车速 km/h</div></div>
    <div class="stat"><div class="stat-val" id="liveLimit">--</div><div class="stat-label" id="iLblLiveLimit">限速 km/h</div></div>
    <div class="stat"><div class="stat-val" id="liveOffset">--</div><div class="stat-label" id="iLblLiveOffset">偏移 km/h</div></div>
  </div>
  <div class="stats" style="margin-top:0">
    <div class="stat"><div class="stat-val" id="liveTorque">--</div><div class="stat-label" id="iLblLiveTorque">扭矩 Nm</div></div>
    <div class="stat"><div class="stat-val" id="liveGear">--</div><div class="stat-label" id="iLblLiveGear">挡位</div></div>
    <div class="stat"><div class="stat-val" id="liveAP">--</div><div class="stat-label" id="iLblLiveAP">AP</div></div>
    <div class="stat"><div class="stat-val" id="liveBrake">--</div><div class="stat-label" id="iLblLiveBrake">刹车</div></div>
  </div>
  <div class="status-row"><span id="iLblLiveFSD">FSD 注入</span><span id="liveFSD" class="status-no">--</span></div>
  <div class="status-row" id="rowLiveGWAP" style="display:none"><span id="iLblLiveGWAP">AP 类型</span><span id="liveGWAP" style="color:#38bdf8;font-weight:700">--</span></div>
  <div class="status-row" id="rowLiveTier" style="display:none"><span id="iLblLiveTier">智能档位</span><span id="liveTier" style="color:#38bdf8;font-weight:700">--</span></div>
  <div class="status-row" id="rowLiveTemp" style="display:none"><span id="iLblLiveTemp">车内/外温度</span><span id="liveTemp" style="color:#38bdf8;font-weight:700">--</span></div>
</div>

<div class="card">
  <div class="card-title" id="iCardCtrl">控制</div>
  <div class="row">
    <span class="row-label" id="iLblFsdEn">FSD 开关</span>
    <label class="toggle"><input type="checkbox" id="fsdEnable" onchange="setVal('fsdEnable',this.checked?1:0)"><span class="slider"></span></label>
  </div>
  <div class="row" style="flex-wrap:wrap;gap:4px">
    <span class="row-label" id="iLblHW">硬件版本</span>
    <select id="hwMode" onchange="setVal('hwMode',this.value);updateSpeedOptions(parseInt(this.value))">
      <option value="0">LEGACY</option>
      <option value="1">HW3</option>
      <option value="2" selected>HW4</option>
    </select>
    <div id="iHWHint" style="width:100%;font-size:11px;color:#64748b;padding:2px 0">HW4 硬件 + 固件 2026.8.x 或更旧（FSD V13）→ 请选 HW3</div>
    <div id="iHWAuto" style="width:100%;font-size:11px;color:#22c55e;padding:2px 0;display:none"></div>
  </div>
  <div class="row">
    <span class="row-label" id="iLblSpeed">速度模式</span>
    <select id="speedProfile" onchange="setVal('speedProfile',this.value)">
      <option value="0" data-zh="平缓" data-en="Sloth">平缓</option>
      <option value="1" data-zh="舒适" data-en="Chill" selected>舒适</option>
      <option value="2" data-zh="标准" data-en="Standard">标准</option>
      <option value="3" data-zh="迅捷" data-en="Hurry">迅捷</option>
      <option value="4" data-zh="狂飙" data-en="Mad Max">狂飙</option>
    </select>
  </div>
  <div class="row">
    <span class="row-label" id="iLblPMode">模式来源</span>
    <select id="profileMode" onchange="setVal('profileMode',this.value);document.getElementById('speedProfile').disabled=this.value==='1';">
      <option value="1" data-zh="自动（拨杆）" data-en="Auto (Stalk)" selected>自动（拨杆）</option>
      <option value="0" data-zh="手动" data-en="Manual">手动</option>
    </select>
  </div>
  <div class="row">
    <span class="row-label" id="iLblISA">限速提示音抑制</span>
    <label class="toggle"><input type="checkbox" id="isaChime" onchange="setVal('isaChime',this.checked?1:0)"><span class="slider"></span></label>
  </div>
  <div class="row" id="rowEmgDet">
    <span class="row-label" id="iLblEmg">紧急车辆检测</span>
    <label class="toggle"><input type="checkbox" id="emergencyDet" checked onchange="setVal('emergencyDet',this.checked?1:0)"><span class="slider"></span></label>
  </div>
  <div class="row">
    <span class="row-label" id="iLblCN">强制激活</span>
    <label class="toggle"><input type="checkbox" id="forceActivate" onchange="setVal('forceActivate',this.checked?1:0)"><span class="slider"></span></label>
  </div>
  <!-- highbeam hidden: requires Vehicle CAN (X179 pin 9/10), not available on Party CAN -->
  <div class="row" id="rowOverrideSL" style="display:none">
    <span class="row-label" id="iLblOverrideSL">重写速度限制</span>
    <label class="toggle"><input type="checkbox" id="overrideSL" onchange="setVal('overrideSL',this.checked?1:0)"><span class="slider"></span></label>
  </div>
  <div class="row">
    <span class="row-label" id="iLblAPRestart">AP 自动恢复</span>
    <label class="toggle"><input type="checkbox" id="apRestart" onchange="setAPRestart(this.checked)"><span class="slider"></span></label>
  </div>
  <div class="row" id="rowHW3Offset" style="display:none">
    <span class="row-label" id="iLblHW3Off">速度偏移 %（HW3）</span>
    <select id="hw3Offset" onchange="setVal('hw3Offset',this.value)">
      <option value="-1" data-zh="自动" data-en="Auto">自动</option>
      <option value="0" data-zh="+0%" data-en="+0%">+0%</option>
      <option value="5" data-zh="+5%" data-en="+5%">+5%</option>
      <option value="10" data-zh="+10%" data-en="+10%">+10%</option>
      <option value="15" data-zh="+15%" data-en="+15%">+15%</option>
      <option value="20" data-zh="+20%" data-en="+20%">+20%</option>
      <option value="25" data-zh="+25%" data-en="+25%">+25%</option>
      <option value="30" data-zh="+30%" data-en="+30%">+30%</option>
      <option value="35" data-zh="+35%" data-en="+35%">+35%</option>
      <option value="40" data-zh="+40%" data-en="+40%">+40%</option>
      <option value="45" data-zh="+45%" data-en="+45%">+45%</option>
      <option value="50" data-zh="+50%" data-en="+50%">+50%</option>
    </select>
  </div>
  <div class="row" id="rowHW4Offset" style="display:none">
    <span class="row-label" id="iLblHW4Off" data-zh="速度偏移 km/h（HW4）" data-en="HW4 Speed Offset (km/h)">速度偏移 km/h（HW4）</span>
    <select id="hw4Offset" onchange="setVal('hw4Offset',this.value)">
      <option value="0" data-zh="关闭" data-en="Off">关闭</option>
      <option value="7" data-zh="+5 km/h" data-en="+5 km/h">+5 km/h</option>
      <option value="10" data-zh="+7 km/h" data-en="+7 km/h">+7 km/h</option>
      <option value="14" data-zh="+10 km/h" data-en="+10 km/h">+10 km/h</option>
      <option value="21" data-zh="+15 km/h" data-en="+15 km/h">+15 km/h</option>
    </select>
  </div>
  <div class="row" id="rowTrackMode" style="display:none">
    <span class="row-label" id="iLblTrackMode">赛道模式（实验性）</span>
    <label class="toggle"><input type="checkbox" id="trackMode" onchange="onTrackModeChange(this)"><span class="slider"></span></label>
  </div>
  <div class="row" id="rowHW3Smart" style="display:none">
    <span class="row-label" id="iLblHW3Smart">智能速度偏移</span>
    <label class="toggle"><input type="checkbox" id="hw3Smart" onchange="setHW3Smart(this.checked)"><span class="slider"></span></label>
  </div>
  <div id="rowHW3SmartRules" style="display:none">
    <div class="smart-rules">
      <div class="smart-rule">
        <span>限速 &lt; </span>
        <input type="number" id="hw3SmT1" min="20" max="195" value="40" style="width:50px" oninput="markSmartDirty();updateSmartLabels()">
        <span> kph &rarr; +</span>
        <input type="number" id="hw3SmO1" min="0" max="50" value="50" style="width:44px" oninput="markSmartDirty()">
        <span class="km">%</span>
      </div>
      <div class="smart-rule">
        <span><span id="sSmLbl2">40</span> ~ </span>
        <input type="number" id="hw3SmT2" min="20" max="200" value="60" style="width:50px" oninput="markSmartDirty();updateSmartLabels()">
        <span> kph &rarr; +</span>
        <input type="number" id="hw3SmO2" min="0" max="50" value="25" style="width:44px" oninput="markSmartDirty()">
        <span class="km">%</span>
      </div>
      <div class="smart-rule">
        <span><span id="sSmLbl3">60</span> ~ </span>
        <input type="number" id="hw3SmT3" min="20" max="200" value="80" style="width:50px" oninput="markSmartDirty();updateSmartLabels()">
        <span> kph &rarr; +</span>
        <input type="number" id="hw3SmO3" min="0" max="50" value="15" style="width:44px" oninput="markSmartDirty()">
        <span class="km">%</span>
      </div>
      <div class="smart-rule">
        <span><span id="sSmLbl4">80</span> ~ </span>
        <input type="number" id="hw3SmT4" min="20" max="200" value="100" style="width:50px" oninput="markSmartDirty();updateSmartLabels()">
        <span> kph &rarr; +</span>
        <input type="number" id="hw3SmO4" min="0" max="50" value="10" style="width:44px" oninput="markSmartDirty()">
        <span class="km">%</span>
      </div>
      <div class="smart-rule">
        <span>&ge; <span id="sSmLbl5">100</span> kph &rarr; +</span>
        <input type="number" id="hw3SmO5" min="0" max="50" value="8" style="width:44px" oninput="markSmartDirty()">
        <span class="km">%</span>
      </div>
      <div class="smart-rule" style="justify-content:flex-end;border-bottom:none;padding-top:10px;gap:8px">
        <button onclick="resetSmartRules()" style="background:#1e293b;color:#94a3b8;border:1px solid #334155;border-radius:6px;padding:5px 12px;font-size:12px;cursor:pointer" id="iSmResetBtn">恢复默认</button>
        <button onclick="saveSmartRules()" style="background:#2563eb;color:#fff;border:none;border-radius:6px;padding:5px 14px;font-size:12px;cursor:pointer" id="iSmSaveBtn">保存</button>
      </div>
    </div>
  </div>
</div>

<div class="card">
  <div class="card-title" id="iCardStat">状态</div>
  <div class="stats">
    <div class="stat"><div class="stat-val green" id="sModified">0</div><div class="stat-label" id="iLblMod">已修改</div></div>
    <div class="stat"><div class="stat-val" id="sRX">0</div><div class="stat-label" id="iLblRX">已接收</div></div>
    <div class="stat"><div class="stat-val amber" id="sErrors">0</div><div class="stat-label" id="iLblErr">错误</div></div>
    <div class="stat"><div class="stat-val" id="sUptime">0秒</div><div class="stat-label" id="iLblUp">运行时间</div></div>
  </div>
  <div id="rowSafeMode" style="display:none;background:#7f1d1d;border-radius:8px;padding:10px 12px;margin-bottom:8px;color:#fca5a5;font-size:13px;font-weight:600">
    ⚠️ 安全模式：设备连续崩溃，CAN 已禁用。请重新刷写固件。
  </div>
  <div class="status-row"><span id="iLblCAN">CAN 总线</span><span id="sCAN" class="status-no">--</span></div>
  <div id="rowDualCAN" style="display:none">
    <div class="status-row"><span id="iLblCANVH">整车 CAN</span><span id="sCANVH" class="status-no">--</span></div>
    <div class="status-row"><span id="iLblCANChassis">底盘 CAN</span><span id="sCANChassis" class="status-no">--</span></div>
  </div>
  <div class="status-row"><span id="iLblFSDTrig">FSD 已触发</span><span id="sFSD" class="status-no">--</span></div>
  <div class="status-row" id="rowTemp" style="display:none">
    <span id="iLblTemp">车内/外温度</span>
    <span id="sTemp" style="color:#38bdf8;font-weight:600;font-size:13px">--</span>
  </div>
  <div class="status-row" id="rowBMS" style="display:none">
    <span id="iLblBMS">电池 <span style="color:#64748b;font-size:11px">（数据未经车辆验证）</span></span>
    <span id="sBMS" style="color:#38bdf8;font-weight:600;font-size:13px">--</span>
  </div>
  <div class="status-row"><span id="iLblTimeSync">时间同步</span><span id="sTimeSync" class="status-no">--</span></div>
  <div class="status-row"><span id="iLblHeap">空闲内存</span><span id="sHeap" style="color:#64748b;font-weight:600">--</span></div>
  <div class="status-row"><span id="iLblVer">固件版本</span><span id="sVer" style="color:#64748b;font-weight:600">--</span></div>
  <div class="status-row" id="rowLanIP" style="display:none"><span id="iLblLanIP">内网 IP</span><span id="sLanIP" style="color:#22c55e;font-weight:600;font-family:monospace">--</span></div>
  <div class="status-row"><span id="iLblMdns">本地域名</span><span style="color:#38bdf8;font-weight:600;font-family:monospace">fsd.local</span></div>
</div>


<div class="card">
  <div class="card-title" id="iCardWifi">WiFi 设置</div>
  <div class="row" style="flex-direction:column;align-items:flex-start;gap:4px">
    <span class="row-label" id="iLblSSID">热点名称（SSID）</span>
    <input type="text" id="wifiSSID" class="text-input" maxlength="32" placeholder="FSD-Controller">
  </div>
  <div class="row" style="flex-direction:column;align-items:flex-start;gap:4px">
    <span class="row-label" id="iLblPass">新密码（留空保持不变）</span>
    <input type="password" id="wifiPass" class="text-input" maxlength="63" placeholder="≥ 8 位">
  </div>
  <button class="save-btn" id="wifiSaveBtn" onclick="doWifi()">保存并重启</button>
  <div class="msg" id="wifiMsg"></div>
  <div id="staSection" style="margin-top:18px;padding-top:18px;border-top:1px solid #1e293b">
    <div style="font-size:12px;font-weight:700;color:#64748b;letter-spacing:2px;margin-bottom:12px" id="iLblStaSection">连接路由器（内网访问）</div>
    <div style="color:#64748b;font-size:12px;margin-bottom:10px" id="iLblStaDesc">填写后模块将同时连接路由器，可通过内网 IP 访问，热点仍保留。留空则只用热点。</div>
    <div class="row" style="flex-direction:column;align-items:flex-start;gap:4px;margin-bottom:8px">
      <span class="row-label" id="iLblStaSSID">路由器 SSID</span>
      <div style="display:flex;gap:8px;width:100%">
        <input type="text" id="staSSID" class="text-input" maxlength="32" placeholder="路由器名称" style="flex:1">
        <button id="scanBtn" onclick="doScan()" style="flex:0 0 52px;height:40px;background:#1e40af;color:#fff;border:none;border-radius:8px;font-size:13px;cursor:pointer;white-space:nowrap">扫描</button>
      </div>
      <select id="scanList" style="display:none;width:100%;background:#1e293b;color:#e2e8f0;border:1px solid #334155;border-radius:8px;padding:8px;font-size:15px;margin-top:4px" onchange="scanPick(this)">
        <option value="" id="iScanPlaceholder">— 选择网络 —</option>
      </select>
    </div>
    <div class="row" style="flex-direction:column;align-items:flex-start;gap:4px;margin-bottom:8px">
      <span class="row-label" id="iLblStaPass">路由器密码</span>
      <input type="password" id="staPass" class="text-input" maxlength="63" placeholder="WiFi 密码">
    </div>
    <div style="display:flex;gap:8px;align-items:center;margin-bottom:8px">
      <span style="font-size:12px;color:#64748b" id="iLblStaStatus">状态：</span>
      <span style="font-size:12px;font-family:monospace" id="staStatus">--</span>
    </div>
    <div style="display:flex;gap:8px">
      <button class="save-btn" id="staSaveBtn" onclick="doSTA()" style="flex:3" id="staSaveBtn">保存并重启</button>
      <button class="save-btn" id="staClearBtn" onclick="doSTAClear()" style="flex:1;background:#334155">断开</button>
    </div>
    <div class="msg" id="staMsg"></div>
  </div>
  <div style="margin-top:18px;padding-top:18px;border-top:1px solid #1e293b">
    <div style="font-size:12px;font-weight:700;color:#64748b;letter-spacing:2px;margin-bottom:12px" id="iLblPinSection">访问密码</div>
    <div style="color:#64748b;font-size:12px;margin-bottom:10px" id="iLblPinDesc">设置后每次打开页面需要输入密码才能操作。留空则不设密码。</div>
    <input type="password" id="accessPin" class="text-input" maxlength="16" placeholder="4~16 位，留空=不设密码" style="margin-bottom:10px">
    <button class="save-btn" id="pinSaveBtn" onclick="doPin()">设置密码</button>
    <div class="msg" id="pinMsg"></div>
  </div>
</div>

<!-- WiFi bridge card (only visible when /api/wifi-bridge/status responds) -->
<div class="card" id="cardBridge" style="display:none">
  <div class="card-title">WiFi 桥接（上游转发）</div>
  <div style="background:#450a0a;border:2px solid #dc2626;border-radius:8px;padding:10px 12px;margin-bottom:12px;font-size:12px;line-height:1.6;color:#fecaca">
    <div style="font-weight:700;color:#fca5a5;margin-bottom:4px">⚠️ 重要提醒：先过滤再联网</div>
    <div>车机连接本 Wi-Fi 之前，请先<span style="color:#fef08a;font-weight:600">启用下方 DNS 过滤并配置好规则</span>。直接让车机裸连上网 = 特斯拉云端会收到非官方网络流量，存在<span style="color:#fca5a5;font-weight:600">账号被封禁或车辆被拉黑</span>的风险。</div>
    <div style="margin-top:4px;color:#fed7aa">建议顺序：① 先用手机加入上游热点 → ② 启用 DNS 过滤 + 选预设 → ③ 最后再让车机连本 AP</div>
  </div>
  <div class="row"><span class="row-label">启用</span>
    <label class="switch"><input type="checkbox" id="brEn" onchange="brSet()"><span class="slider"></span></label></div>
  <div class="row"><span class="row-label">上游状态</span>
    <span id="brStat" style="font-family:monospace;font-size:13px">--</span></div>
  <div class="row"><span class="row-label">上游 SSID</span>
    <span id="brSSID" style="font-family:monospace;font-size:13px">--</span></div>
  <div class="row"><span class="row-label">NAT</span>
    <span id="brNat" style="font-family:monospace;font-size:13px">--</span></div>
  <div class="row"><span class="row-label">芯片温度</span>
    <span id="brTemp" style="font-family:monospace;font-size:13px">--</span></div>

  <div style="margin-top:14px;padding-top:14px;border-top:1px solid #1e293b">
    <div style="font-size:12px;font-weight:700;color:#64748b;letter-spacing:2px;margin-bottom:10px">已保存热点</div>
    <div id="brList" style="font-size:13px;font-family:monospace"></div>
    <div style="margin-top:10px">
      <input type="text" id="brAddSsid" class="text-input" maxlength="32" placeholder="SSID（手动填写，大小写严格一致）" style="margin-bottom:6px">
      <input type="password" id="brAddPass" class="text-input" maxlength="63" placeholder="密码（开放热点留空）" style="margin-bottom:6px">
      <button class="save-btn" onclick="brAdd()">添加 / 更新</button>
      <div class="msg" id="brMsg"></div>
    </div>
  </div>

  <div style="margin-top:14px;padding-top:14px;border-top:1px solid #1e293b">
    <div style="font-size:12px;font-weight:700;color:#64748b;letter-spacing:2px;margin-bottom:10px">DNS 过滤</div>
    <div class="row"><span class="row-label">启用</span>
      <label class="switch"><input type="checkbox" id="brDnsEn" onchange="brSet()"><span class="slider"></span></label></div>
    <div style="color:#64748b;font-size:11px;margin:4px 0 4px 0">空格/逗号/分号分隔，支持根域（含子域匹配）</div>
    <div style="background:#0f172a;border:1px solid #1e293b;border-radius:6px;padding:8px;margin:6px 0 8px 0;font-size:11px;color:#94a3b8;line-height:1.6">
      <div style="color:#cbd5e1;font-weight:600;margin-bottom:4px">⚠️ 一键预设 · 载入后可自由修改</div>
      <div>• 预设只是帮你填好文本框，<span style="color:#34d399">载入后还可以手动增删</span>，改完点「保存 DNS 配置」才生效</div>
      <div>• 匹配规则：<span style="color:#cbd5e1">更长的规则优先</span>（如 tesla.cn 黑 + nav-prd-maps.tesla.cn 白 → nav-prd-maps 放行，其他 *.tesla.cn 拦）</div>
      <div>• 手机若开启「私人 DNS (DoT)」或 DoH 会<span style="color:#f87171">绕过</span>本过滤，需在系统设置关闭</div>
      <div style="margin-top:6px;padding-top:6px;border-top:1px dashed #334155;color:#fbbf24">• 预设内容<span style="color:#fde68a">来自网络收集</span>，仅供参考。Tesla 云域名可能因车型/区域/版本而异，请结合自己车的实际流量（参考"最近拦截"列表）自行增删调整。</div>
    </div>
    <div style="display:flex;flex-wrap:wrap;gap:6px;margin-bottom:8px">
      <button type="button" id="pstTesla" onclick="brApplyPreset('tesla_min')" style="height:28px;padding:0 10px;background:#065f46;color:#d1fae5;border:2px solid #34d399;border-radius:6px;font-size:11px;cursor:pointer;font-weight:600">⭐ Tesla 推荐（实测）</button>
      <button type="button" id="pstBl" onclick="brApplyPreset('bl_telemetry')" style="height:28px;padding:0 10px;background:#7c2d12;color:#fed7aa;border:none;border-radius:6px;font-size:11px;cursor:pointer">只屏蔽遥测</button>
      <button type="button" id="pstClr" onclick="brApplyPreset('clear')" style="height:28px;padding:0 10px;background:#334155;color:#cbd5e1;border:none;border-radius:6px;font-size:11px;cursor:pointer">清空</button>
    </div>
    <div class="row" style="flex-direction:column;align-items:flex-start;gap:4px">
      <span class="row-label">白名单（留空=允许全部未拉黑）</span>
      <textarea id="brDnsAllow" class="text-input" maxlength="1023" rows="3" style="font-family:monospace;font-size:12px"></textarea>
    </div>
    <div class="row" style="flex-direction:column;align-items:flex-start;gap:4px;margin-top:8px">
      <span class="row-label">黑名单（优先生效）</span>
      <textarea id="brDnsBlock" class="text-input" maxlength="1023" rows="3" style="font-family:monospace;font-size:12px"></textarea>
    </div>
    <button class="save-btn" onclick="brSet()" style="margin-top:8px">保存 DNS 配置</button>
    <div style="margin-top:10px">
      <div style="display:flex;justify-content:space-between;align-items:center;margin-bottom:6px">
        <span style="font-size:12px;color:#64748b">DNS 拦截 <span id="brBlkTot">0</span> 次 · IP 拦截 <span id="brIpDrops">0</span> 次 (cache <span id="brIpCache">0</span>/<span id="brIpCap">256</span> · peak <span id="brIpPeak">0</span>)</span>
        <button onclick="brBlkClear()" style="height:28px;padding:0 12px;background:#334155;color:#cbd5e1;border:none;border-radius:6px;font-size:12px;cursor:pointer">清空</button>
      </div>
      <div id="brBlkList" style="font-family:monospace;font-size:12px;max-height:120px;overflow-y:auto"></div>
    </div>
  </div>
</div>

<div class="card">
  <div class="card-title" id="iCardLog">诊断日志</div>
  <div style="font-size:11px;color:#64748b;padding:4px 0 8px" id="iLblLogNote">掉电清除 · 最近 80 条事件 · 遇到问题请复制发给我们</div>
  <textarea id="diagLog" readonly style="width:100%;height:160px;background:#0f172a;color:#94a3b8;border:1px solid #334155;border-radius:8px;padding:8px;font-size:11px;font-family:monospace;resize:vertical;box-sizing:border-box"></textarea>
  <div style="display:flex;gap:10px;margin-top:8px">
    <button class="save-btn" id="logCopyBtn" onclick="doLogCopy()" style="flex:3">复制日志</button>
    <button class="save-btn" id="logDlBtn" onclick="doLogDownload()" style="flex:2;background:#1e40af">下载持久日志</button>
    <button class="save-btn" id="logClrBtn" onclick="doLogClear()" style="flex:1;background:#334155">清除</button>
  </div>
  <div class="msg" id="logMsg"></div>
</div>

<div class="card">
  <div class="card-title" id="iCardOTA">固件更新</div>
  <div class="ota-row">
    <label class="file-btn" id="iLblFile" for="fwFile">选择文件</label>
    <input type="file" id="fwFile" accept=".bin" style="display:none" onchange="fileChosen(this)">
    <span class="file-name" id="fileName">未选择文件</span>
  </div>
  <button class="upload-btn" id="uploadBtn" disabled onclick="doOTA()">上传固件</button>
  <div class="progress" id="progWrap"><div class="progress-bar" id="progBar"></div></div>
  <div class="msg" id="otaMsg"></div>
  <button class="save-btn" id="rebootBtn" onclick="showRebootConfirm()" style="background:#b91c1c;margin-top:14px">重启设备</button>
  <div id="rebootConfirmBox" style="display:none;background:#1e293b;border:1px solid #b91c1c;border-radius:8px;padding:12px;margin-top:8px;font-size:13px;color:#fca5a5">
    <div id="iRebootConfirmMsg" style="margin-bottom:10px">确定要重启吗？</div>
    <div style="display:flex;gap:8px">
      <button onclick="doReboot()" style="flex:1;background:#b91c1c;color:#fff;border:none;border-radius:6px;padding:7px 0;font-size:13px;cursor:pointer" id="iRebootConfirmBtn">确认重启</button>
      <button onclick="hideRebootConfirm()" style="flex:1;background:#334155;color:#94a3b8;border:none;border-radius:6px;padding:7px 0;font-size:13px;cursor:pointer">取消</button>
    </div>
  </div>
  <div class="msg" id="rebootMsg"></div>
  <button class="save-btn" id="iResetAllBtn" onclick="showResetConfirm()" style="background:#1e293b;border:1px solid #b91c1c;color:#ef4444;margin-top:8px">恢复出厂设置</button>
  <div id="resetAllConfirmBox" style="display:none;background:#1e293b;border:1px solid #b91c1c;border-radius:8px;padding:12px;margin-top:8px;font-size:13px;color:#fca5a5">
    <div id="iResetAllConfirmMsg" style="margin-bottom:10px">将清除所有配置（WiFi、PIN、所有参数），确定吗？</div>
    <div style="display:flex;gap:8px">
      <button onclick="doResetAll()" style="flex:1;background:#b91c1c;color:#fff;border:none;border-radius:6px;padding:7px 0;font-size:13px;cursor:pointer" id="iResetAllConfirmBtn">确认重置</button>
      <button onclick="hideResetConfirm()" style="flex:1;background:#334155;color:#94a3b8;border:none;border-radius:6px;padding:7px 0;font-size:13px;cursor:pointer">取消</button>
    </div>
  </div>
  <div class="msg" id="resetAllMsg"></div>
</div>

<div style="text-align:center;padding:20px 0 8px;font-size:11px;color:#334155;line-height:1.8">
  本软件完全免费开源，如有人向你收费请勿上当<br>
  <a href="https://github.com/wjsall/tesla-fsd-controller" style="color:#334155;text-decoration:none">github.com/wjsall/tesla-fsd-controller</a>
</div>

<script>
var lang='zh';
var agreed=false;
var token='';
var pinRequired=false;
try{token=sessionStorage.getItem('fsd_tok')||'';}catch(e){}
var T={
  zh:{title:'特斯拉控制器',cardCtrl:'控制',cardStat:'状态',cardOTA:'固件更新',
    lblFsdEn:'FSD 开关',lblHW:'硬件版本',lblSpeed:'速度模式',lblPMode:'模式来源',
    lblISA:'限速提示音抑制',lblEmg:'紧急车辆检测',lblCN:'强制激活',
    lblMod:'已修改',lblRX:'已接收',lblErr:'错误',lblUp:'运行时间',
    lblCAN:'Party CAN',lblFSDTrig:'FSD 已触发',
    lblCANVH:'整车 CAN',lblCANChassis:'底盘 CAN',
    lblFile:'选择文件',noFile:'未选择文件',uploadBtn:'上传固件',
    lblVer:'固件版本',
    canOK:'正常',canErr:'异常',fsdYes:'是',fsdNo:'否',
    otaOK:'上传成功，正在重启...',otaFail:'上传失败: ',otaConn:'连接失败',rebootBtn:'重启设备',rebootConfirm:'确定要重启吗？',rebootConfirmBtn:'确认重启',rebootOK:'正在重启...',rebootFail:'重启失败',resetAllBtn:'恢复出厂设置',resetAllConfirm:'将清除所有配置（包括 WiFi、PIN、所有参数），确定吗？',resetAllConfirmBtn:'确认重置',resetAllOK:'已重置，正在重启...',resetAllFail:'重置失败',
    uptH:'时',uptM:'分',uptS:'秒',langBtn:'EN',
    hwHint:'HW4 硬件 + 固件 2026.8.x 或更旧（FSD V13）→ 请选 HW3',
    cardWifi:'WiFi 设置',lblSSID:'热点名称（SSID）',lblPass:'新密码（留空保持不变）',
    lblHW3Off:'速度偏移 km/h（HW3）',lblHW3Smart:'智能速度偏移',
    smR1L:'限速 <',smR2L:'限速 T1 ~',smR3L:'限速 ≥ T2 → +',lblBMS:'电池',
    wifiSave:'保存并重启',wifiOK:'已保存，正在重启...',wifiFail:'保存失败: ',
    wifiPassErr:'密码至少 8 位',wifiSSIDErr:'SSID 不能为空',
    lblLanIP:'内网 IP',lblMdns:'本地域名',
    lblStaSection:'连接路由器（内网访问）',lblStaDesc:'填写后模块将同时连接路由器，可通过内网 IP 访问，热点仍保留。留空则只用热点。',
    lblStaSSID:'路由器 SSID',lblStaPass:'路由器密码',lblStaStatus:'状态：',
    staConnected:'已连接',staDisconnected:'未连接',staSave:'保存并重启',staClear:'断开',
    staOK:'已保存，正在重启...',staFail:'保存失败',
    scanBtn:'扫描',scanPlaceholder:'— 选择网络 —',scanScanning:'扫描中...',scanFail:'扫描失败',
    cardLog:'诊断日志',lblLogNote:'RAM日志掉电清除 · 最近 80 条 · SPIFFS持久日志可下载',
    logCopyBtn:'复制日志',logDlBtn:'下载持久日志',logClrBtn:'清除',logCleared:'已清除',logCopied:'已复制',
    lblTimeSync:'时间同步',timeSyncOK:'已同步',timeSyncNo:'未同步',
    lblAPRestart:'AP 自动恢复',
    lblHW4Off:'速度偏移 km/h（HW4）',
    lblTrackMode:'赛道模式（实验性）',
    trackModeWarn:'⚠️ 实验性功能，效果未经完全验证。开启后将向总线持续注入赛道模式请求，可能影响车辆稳定控制。请知悉风险后开启。',
    lblLiveGWAP:'AP 类型',
    hwDetHW3:'CAN 检测到：HW3',hwDetHW4:'CAN 检测到：HW4',
    hwDetNone:'未自动检测到，请手动选择（2020+ 车型不支持）',
    cardLive:'实时摘要',lblLiveCAN:'CAN',lblLiveSpeed:'车速',lblLiveLimit:'限速',lblLiveOffset:'偏移',
    lblLiveFSD:'FSD 注入',lblLiveTier:'智能档位',lblLiveTemp:'车内/外温度',liveSpeedUnit:'km/h',liveLimitUnit:'km/h',liveOffUnit:'km/h',
    lblLiveTorque:'扭矩 Nm',lblLiveGear:'挡位',lblLiveAP:'AP',lblLiveBrake:'刹车'},
  en:{title:'Tesla Controller',cardCtrl:'CONTROL',cardStat:'STATUS',cardOTA:'OTA UPDATE',
    lblFsdEn:'FSD Enable',lblHW:'Hardware',lblSpeed:'Speed Profile',lblPMode:'Profile Source',
    lblISA:'ISA Chime Suppress',lblEmg:'Emergency Detection',lblCN:'Force Activate',
    lblHW3Off:'HW3 Speed Offset (km/h)',lblHW3Smart:'Smart Speed Offset',
    smR1L:'Limit <',smR2L:'Limit T1 ~',smR3L:'Limit ≥ T2 → +',lblBMS:'Battery',
    lblMod:'MODIFIED',lblRX:'RECEIVED',lblErr:'ERRORS',lblUp:'UPTIME',
    lblCAN:'Party CAN',lblFSDTrig:'FSD Triggered',
    lblCANVH:'Vehicle CAN',lblCANChassis:'Chassis CAN',
    lblFile:'Choose File',noFile:'No file chosen',uploadBtn:'Upload Firmware',
    lblVer:'Firmware Version',
    canOK:'OK',canErr:'ERROR',fsdYes:'Yes',fsdNo:'No',
    otaOK:'Upload success, rebooting...',otaFail:'Upload failed: ',otaConn:'Connection error',rebootBtn:'Restart Device',rebootConfirm:'Restart the device?',rebootConfirmBtn:'Confirm Restart',rebootOK:'Rebooting...',rebootFail:'Reboot failed',resetAllBtn:'Factory Reset',resetAllConfirm:'This will erase all settings (WiFi, PIN, all config). Continue?',resetAllConfirmBtn:'Confirm Reset',resetAllOK:'Reset done, rebooting...',resetAllFail:'Reset failed',
    uptH:'h',uptM:'m',uptS:'s',langBtn:'中文',
    hwHint:'HW4 hardware + firmware 2026.8.x or older (FSD V13) → select HW3',
    cardWifi:'WiFi Settings',lblSSID:'AP Name (SSID)',lblPass:'New Password (blank = keep current)',
    wifiSave:'Save & Restart',wifiOK:'Saved, rebooting...',wifiFail:'Save failed: ',
    wifiPassErr:'Password must be ≥ 8 chars',wifiSSIDErr:'SSID cannot be empty',
    lblLanIP:'LAN IP',lblMdns:'Local Domain',
    lblStaSection:'Connect to Router (LAN access)',lblStaDesc:'Module will also connect to your router — access via LAN IP. Hotspot remains as fallback. Leave blank for hotspot-only.',
    lblStaSSID:'Router SSID',lblStaPass:'Router Password',lblStaStatus:'Status:',
    staConnected:'Connected',staDisconnected:'Not connected',staSave:'Save & Restart',staClear:'Disconnect',
    staOK:'Saved, rebooting...',staFail:'Save failed',
    scanBtn:'Scan',scanPlaceholder:'— Select network —',scanScanning:'Scanning...',scanFail:'Scan failed',
    cardLog:'Diagnostic Log',lblLogNote:'RAM log clears on power-off · last 80 events · SPIFFS log persists across reboots',
    logCopyBtn:'Copy Log',logDlBtn:'Download Persistent Log',logClrBtn:'Clear',logCleared:'Cleared',logCopied:'Copied',
    lblTimeSync:'Time Sync',timeSyncOK:'Synced',timeSyncNo:'Not synced',
    lblAPRestart:'AP Auto-Resume',
    lblHW4Off:'HW4 Speed Offset (km/h)',
    lblTrackMode:'Track Mode (Experimental)',
    trackModeWarn:'⚠️ Experimental. Continuously injects Track Mode request on the CAN bus. May affect stability control. Enable only if you understand the risk.',
    lblLiveGWAP:'AP TYPE',
    hwDetHW3:'CAN detected: HW3',hwDetHW4:'CAN detected: HW4',
    hwDetNone:'Auto-detect failed — select manually (2020+ vehicles not supported)',
    cardLive:'LIVE STATUS',lblLiveCAN:'CAN',lblLiveSpeed:'SPEED',lblLiveLimit:'LIMIT',lblLiveOffset:'OFFSET',
    lblLiveFSD:'FSD INJECT',lblLiveTier:'SMART TIER',lblLiveTemp:'CABIN/AMBIENT TEMP',liveSpeedUnit:'km/h',liveLimitUnit:'km/h',liveOffUnit:'km/h',
    lblLiveTorque:'TORQUE Nm',lblLiveGear:'GEAR',lblLiveAP:'AP',lblLiveBrake:'BRAKE'}
};
function applyLang(){
  var t=T[lang];
  document.documentElement.lang=lang;
  document.getElementById('iTitle').textContent=t.title;
  document.getElementById('iLangBtn').textContent=t.langBtn;
  document.getElementById('iCardCtrl').textContent=t.cardCtrl;
  document.getElementById('iCardStat').textContent=t.cardStat;
  document.getElementById('iCardOTA').textContent=t.cardOTA;
  document.getElementById('iLblFsdEn').textContent=t.lblFsdEn;
  document.getElementById('iLblHW').textContent=t.lblHW;
  document.getElementById('iLblSpeed').textContent=t.lblSpeed;
  document.getElementById('iLblPMode').textContent=t.lblPMode;
  document.getElementById('iLblISA').textContent=t.lblISA;
  document.getElementById('iLblEmg').textContent=t.lblEmg;
  document.getElementById('iLblCN').textContent=t.lblCN;
  document.getElementById('iLblHW3Off').textContent=t.lblHW3Off;
  document.getElementById('iLblHW3Smart').textContent=t.lblHW3Smart;
  document.getElementById('iSmR1L').textContent=t.smR1L;
  document.getElementById('iSmR2L').textContent=t.smR2L;
  document.getElementById('iSmR3L').textContent=t.smR3L;
  document.getElementById('iLblBMS').textContent=t.lblBMS;
  document.getElementById('iLblMod').textContent=t.lblMod;
  document.getElementById('iLblRX').textContent=t.lblRX;
  document.getElementById('iLblErr').textContent=t.lblErr;
  document.getElementById('iLblUp').textContent=t.lblUp;
  document.getElementById('iLblCAN').textContent=t.lblCAN;
  document.getElementById('iLblFSDTrig').textContent=t.lblFSDTrig;
  document.getElementById('iLblVer').textContent=t.lblVer;
  document.getElementById('iHWHint').textContent=t.hwHint;
  document.getElementById('iLblLanIP').textContent=t.lblLanIP;
  document.getElementById('iLblMdns').textContent=t.lblMdns;
  document.getElementById('iCardWifi').textContent=t.cardWifi;
  document.getElementById('iLblSSID').textContent=t.lblSSID;
  document.getElementById('iLblPass').textContent=t.lblPass;
  document.getElementById('wifiSaveBtn').textContent=t.wifiSave;
  document.getElementById('iLblStaSection').textContent=t.lblStaSection;
  document.getElementById('iLblStaDesc').textContent=t.lblStaDesc;
  document.getElementById('iLblStaSSID').textContent=t.lblStaSSID;
  document.getElementById('iLblStaPass').textContent=t.lblStaPass;
  document.getElementById('iLblStaStatus').textContent=t.lblStaStatus;
  document.getElementById('staSaveBtn').textContent=t.staSave;
  document.getElementById('staClearBtn').textContent=t.staClear;
  document.getElementById('scanBtn').textContent=t.scanBtn;
  document.getElementById('iScanPlaceholder').textContent=t.scanPlaceholder;
  document.getElementById('iLblAPRestart').textContent=t.lblAPRestart;
  document.getElementById('iLblHW4Off').textContent=t.lblHW4Off;
  document.getElementById('iLblTrackMode').textContent=t.lblTrackMode;
  document.getElementById('iLblLiveGWAP').textContent=t.lblLiveGWAP;
  document.getElementById('iLblLiveTemp').textContent=t.lblLiveTemp;
  document.getElementById('iLblTimeSync').textContent=t.lblTimeSync;
  document.getElementById('iCardLog').textContent=t.cardLog;
  document.getElementById('iLblLogNote').textContent=t.lblLogNote;
  document.getElementById('logCopyBtn').textContent=t.logCopyBtn;
  document.getElementById('logDlBtn').textContent=t.logDlBtn;
  document.getElementById('logClrBtn').textContent=t.logClrBtn;
  document.getElementById('iLblFile').textContent=t.lblFile;
  document.getElementById('uploadBtn').textContent=t.uploadBtn;
  document.getElementById('rebootBtn').textContent=t.rebootBtn;
  document.getElementById('iRebootConfirmMsg').textContent=t.rebootConfirm;
  document.getElementById('iRebootConfirmBtn').textContent=t.rebootConfirmBtn;
  document.getElementById('iResetAllBtn').textContent=t.resetAllBtn;
  document.getElementById('iResetAllConfirmMsg').textContent=t.resetAllConfirm;
  document.getElementById('iResetAllConfirmBtn').textContent=t.resetAllConfirmBtn;
  ['speedProfile','profileMode'].forEach(function(id){
    Array.from(document.getElementById(id).options).forEach(function(o){
      if(o.dataset[lang])o.textContent=o.dataset[lang];
    });
  });
  var fn=document.getElementById('fileName');
  if(!fn.dataset.hasFile)fn.textContent=t.noFile;
  document.getElementById('iCardLive').textContent=t.cardLive;
  document.getElementById('iLblLiveCAN').textContent=t.lblLiveCAN;
  document.getElementById('iLblLiveSpeed').textContent=t.lblLiveSpeed;
  document.getElementById('iLblLiveLimit').textContent=t.lblLiveLimit;
  document.getElementById('iLblLiveOffset').textContent=t.lblLiveOffset;
  document.getElementById('iLblLiveFSD').textContent=t.lblLiveFSD;
  document.getElementById('iLblLiveTier').textContent=t.lblLiveTier;
  document.getElementById('iLblLiveTorque').textContent=t.lblLiveTorque;
  document.getElementById('iLblLiveGear').textContent=t.lblLiveGear;
  document.getElementById('iLblLiveAP').textContent=t.lblLiveAP;
  document.getElementById('iLblLiveBrake').textContent=t.lblLiveBrake;
}
function toggleLang(){lang=lang==='zh'?'en':'zh';applyLang();}
function poll(){
  fetch('/api/status').then(r=>r.json()).then(d=>{
    var t=T[lang];
    document.getElementById('sModified').textContent=d.modified;
    document.getElementById('sRX').textContent=d.rx;
    document.getElementById('sErrors').textContent=d.errors;
    var u=d.uptime,h=Math.floor(u/3600),m=Math.floor((u%3600)/60),s=u%60;
    document.getElementById('sUptime').textContent=h>0?h+t.uptH+m+t.uptM:m>0?m+t.uptM+s+t.uptS:s+t.uptS;
    var canEl=document.getElementById('sCAN');
    canEl.textContent=d.canOK?t.canOK:t.canErr;
    canEl.className=d.canOK?'status-ok':'status-err';
    document.getElementById('iLblCAN').textContent=t.lblCAN;
    if(typeof d.vhOK!=='undefined'){
      document.getElementById('rowDualCAN').style.display='';
      document.getElementById('iLblCANVH').textContent=t.lblCANVH;
      document.getElementById('iLblCANChassis').textContent=t.lblCANChassis;
      var vhEl=document.getElementById('sCANVH');
      vhEl.textContent=d.vhOK?t.canOK:t.canErr;
      vhEl.className=d.vhOK?'status-ok':'status-err';
      var prtyEl=document.getElementById('sCANChassis');
      prtyEl.textContent=d.prtyOK?t.canOK:t.canErr;
      prtyEl.className=d.prtyOK?'status-ok':'status-err';
    }
    var fsdEl=document.getElementById('sFSD');
    var fsdActive=d.fsdTriggered&&!!d.fsdEnable;
    fsdEl.textContent=fsdActive?t.fsdYes:t.fsdNo;
    fsdEl.className=fsdActive?'status-yes':'status-no';
    document.getElementById('fsdEnable').checked=!!d.fsdEnable;
    document.getElementById('hwMode').value=d.hwMode;
    var hwAutoEl=document.getElementById('iHWAuto');
    if(d.hwDetected===1){hwAutoEl.style.color='#22c55e';hwAutoEl.textContent=T[lang].hwDetHW3;hwAutoEl.style.display='';}
    else if(d.hwDetected===2){hwAutoEl.style.color='#22c55e';hwAutoEl.textContent=T[lang].hwDetHW4;hwAutoEl.style.display='';}
    else if(d.rx>0){hwAutoEl.style.color='#f59e0b';hwAutoEl.textContent=T[lang].hwDetNone;hwAutoEl.style.display='';}
    else{hwAutoEl.style.display='none';}
    updateSpeedOptions(d.hwMode);
    document.getElementById('speedProfile').value=d.speedProfile;
    document.getElementById('profileMode').value=d.profileMode?'1':'0';
    document.getElementById('speedProfile').disabled=!!d.profileMode;
    document.getElementById('isaChime').checked=!!d.isaChime;
    document.getElementById('emergencyDet').checked=!!d.emergencyDet;
    document.getElementById('forceActivate').checked=!!d.forceActivate;
    document.getElementById('overrideSL').checked=!!d.overrideSL;
    document.getElementById('rowOverrideSL').style.display=(d.hwMode===0)?'':'none';
    var hw3OffEl=document.getElementById('hw3Offset');
    if(hw3OffEl)hw3OffEl.value=String(d.hw3Offset!=null?d.hw3Offset:-1);
    if(d.tempSeen){
      document.getElementById('rowTemp').style.display='';
      var tIn=(d.tempInRaw*0.25).toFixed(1)+'°C';
      var tOut=(d.tempOutRaw*0.5-40).toFixed(1)+'°C';
      document.getElementById('sTemp').textContent=tIn+' / '+tOut;
    }
    if(d.bmsSeen){
      var bmsRow=document.getElementById('rowBMS');
      bmsRow.style.display='';
      var soc=d.bmsSoc!=null?(d.bmsSoc/10).toFixed(1)+'%':'--';
      var volt=d.bmsV!=null?(d.bmsV/100).toFixed(1)+'V':'--';
      var tmin=d.bmsMinT!=null?d.bmsMinT+'°C':'--';
      var tmax=d.bmsMaxT!=null?d.bmsMaxT+'°C':'--';
      document.getElementById('sBMS').textContent=soc+' '+volt+' '+tmin+'~'+tmax;
    }
    // Safe mode banner
    document.getElementById('rowSafeMode').style.display=d.safeMode?'':'none';
    // Free heap
    if(d.freeHeap!=null){
      var kb=(d.freeHeap/1024).toFixed(1);
      var heapColor=d.freeHeap<20000?'#f87171':d.freeHeap<50000?'#fbbf24':'#64748b';
      var heapEl=document.getElementById('sHeap');
      heapEl.textContent=kb+' KB';
      heapEl.style.color=heapColor;
    }
    if(d.apSSID&&!wifiSSIDLoaded){document.getElementById('wifiSSID').value=d.apSSID;wifiSSIDLoaded=true;}
    if(d.staSSID!=null&&!staSSIDLoaded){document.getElementById('staSSID').value=d.staSSID;staSSIDLoaded=true;}
    var staEl=document.getElementById('staStatus');
    if(d.staOK){staEl.textContent=T[lang].staConnected+' '+d.staIP;staEl.style.color='#22c55e';}
    else if(d.staSSID){staEl.textContent=T[lang].staDisconnected;staEl.style.color='#ef4444';}
    else{staEl.textContent='--';staEl.style.color='#64748b';}
    var lanRow=document.getElementById('rowLanIP');
    if(d.staOK&&d.staIP){document.getElementById('sLanIP').textContent=d.staIP;lanRow.style.display='';}
    else{lanRow.style.display='none';}
    if(d.version)document.getElementById('sVer').textContent=(d.variant?d.variant+' ':'')+'v'+d.version;
    // AP auto-restart
    var smartEl=document.getElementById('hw3Smart');
    if(smartEl){
      smartEl.checked=!!d.hw3Smart;
      // Sync smart rule inputs whenever user isn't actively editing — keeps phone/car UI in sync
      if(!smartDirty){
        if(d.hw3SmT1!=null)document.getElementById('hw3SmT1').value=d.hw3SmT1;
        if(d.hw3SmT2!=null)document.getElementById('hw3SmT2').value=d.hw3SmT2;
        if(d.hw3SmT3!=null)document.getElementById('hw3SmT3').value=d.hw3SmT3;
        if(d.hw3SmT4!=null)document.getElementById('hw3SmT4').value=d.hw3SmT4;
        if(d.hw3SmO1!=null)document.getElementById('hw3SmO1').value=d.hw3SmO1;
        if(d.hw3SmO2!=null)document.getElementById('hw3SmO2').value=d.hw3SmO2;
        if(d.hw3SmO3!=null)document.getElementById('hw3SmO3').value=d.hw3SmO3;
        if(d.hw3SmO4!=null)document.getElementById('hw3SmO4').value=d.hw3SmO4;
        if(d.hw3SmO5!=null)document.getElementById('hw3SmO5').value=d.hw3SmO5;
        updateSmartLabels();
      }
      var smartActive=(d.hwMode===1&&!!d.hw3Smart);
      document.getElementById('rowHW3Offset').style.display=(d.hwMode===1&&!smartActive)?'':'none';
      document.getElementById('rowHW3SmartRules').style.display=smartActive?'':'none';
      document.getElementById('rowHW4Offset').style.display=(d.hwMode===2)?'':'none';
      document.getElementById('rowTrackMode').style.display=(d.hwMode===1)?'':'none';
    }
    document.getElementById('apRestart').checked=!!d.apRestart;
    var hw4OffEl=document.getElementById('hw4Offset');
    if(hw4OffEl)hw4OffEl.value=String(d.hw4Offset!=null?d.hw4Offset:0);
    var trackModeEl=document.getElementById('trackMode');
    if(trackModeEl)trackModeEl.checked=!!d.trackMode;

    var tsEl=document.getElementById('sTimeSync');
    if(d.timeSynced){tsEl.textContent=T[lang].timeSyncOK;tsEl.className='status-ok';}
    else{tsEl.textContent=T[lang].timeSyncNo;tsEl.className='status-no';}
    if(d.dbg&&d.dbg.captured&&!document.getElementById('dbgCard')){
      var card=document.createElement('div');
      card.className='card';card.id='dbgCard';
      card.innerHTML='<div class="card-title">DEBUG — Frame 1021 Mux-0</div>'
        +'<div class="status-row"><span>原始字节</span><span id="dbgBytes" style="font-family:monospace;color:#38bdf8;font-size:13px">--</span></div>'
        +'<div class="status-row"><span>Bit 30 (UI_disableBackup)</span><span id="dbgBit30" style="font-weight:700">--</span></div>'
        +'<div style="font-size:11px;color:#64748b;padding:8px 0">Bit 30 = 1 中国限制倒车；= 0 无限制</div>';
      document.querySelector('.card').before(card);
    }
    if(d.dbg&&d.dbg.captured&&document.getElementById('dbgBytes')){
      document.getElementById('dbgBytes').textContent=d.dbg.bytes;
      var b30=document.getElementById('dbgBit30');
      b30.textContent=d.dbg.bit30===1?'1 — 限制':'0 — 无限制';
      b30.style.color=d.dbg.bit30===1?'#ef4444':'#22c55e';
    }
    // ── Live summary panel ───────────────────────────────────────────────
    var lCanEl=document.getElementById('liveCAN');
    lCanEl.textContent=d.canOK?t.canOK:t.canErr;
    lCanEl.className='stat-val '+(d.canOK?'green':'err');
    var spd=d.speedD?+(d.speedD/10).toFixed(0):0;
    document.getElementById('liveSpeed').textContent=spd||'--';
    var lim=(d.fusedLimit>0&&d.fusedLimit<31)?d.fusedLimit*5:(d.visionLimit>0&&d.visionLimit<31?d.visionLimit*5:0);
    document.getElementById('liveLimit').textContent=lim||'--';
    var lFsdEl=document.getElementById('liveFSD');
    var lFsdActive=d.fsdTriggered&&!!d.fsdEnable;
    lFsdEl.textContent=lFsdActive?t.fsdYes:t.fsdNo;
    lFsdEl.className=lFsdActive?'status-ok':'status-no';
    var tierRow=document.getElementById('rowLiveTier');
    if(d.hwMode===1&&d.hw3Smart&&d.smartTier>0){
      tierRow.style.display='';
      var tierLabels=['',' < '+d.hw3SmT1,d.hw3SmT1+'~'+d.hw3SmT2,d.hw3SmT2+'~'+d.hw3SmT3,d.hw3SmT3+'~'+d.hw3SmT4,' ≥ '+d.hw3SmT4];
      var tl=tierLabels[d.smartTier]||('T'+d.smartTier);
      var _sLim=d.fusedLimit>0?d.fusedLimit*5:(d.visionLimit>0?d.visionLimit*5:0);
      var smartKmh=(_sLim>0&&d.smartPct!=null)?Math.round(_sLim*d.smartPct/100):0;
      document.getElementById('liveTier').textContent=tl+' kph  +'+d.smartPct+'%  (+'+smartKmh+' km/h)';
    } else {
      tierRow.style.display='none';
    }
    var liveTempRow=document.getElementById('rowLiveTemp');
    if(d.tempSeen){
      liveTempRow.style.display='';
      document.getElementById('liveTemp').textContent=(d.tempInRaw*0.25).toFixed(1)+'°C / '+(d.tempOutRaw*0.5-40).toFixed(1)+'°C';
    } else {
      liveTempRow.style.display='none';
    }
    var spdLimKph=d.fusedLimit>0?d.fusedLimit*5:(d.visionLimit>0?d.visionLimit*5:0);
    // Prefer manual hw3Offset; fall back to live hw3AutoOffset (matches car UI)
    var hw3Pct=(d.hw3Offset>=0)?d.hw3Offset:(d.hw3AutoOffset>=0?d.hw3AutoOffset:-1);
    var offKmh=(hw3Pct>=0&&spdLimKph>0)?Math.round(spdLimKph*hw3Pct/100):null;
    var smartKmh=(d.smartPct>0&&spdLimKph>0)?Math.round(spdLimKph*d.smartPct/100):0;
    var hw4OffKmh={0:0,7:5,10:7,14:10,21:15}[d.hw4Offset]||0;
    // HW3 auto mode (smart off, manual=-1): passthrough Tesla's mux-2 bytes, we do not override.
    var offVal=d.hwMode===1?(d.hw3Smart?smartKmh:(offKmh!=null?offKmh:null)):
               (d.hwMode===2&&hw4OffKmh>0?hw4OffKmh:null);
    document.getElementById('liveOffset').textContent=offVal!=null?'+'+offVal:'--';
    // ── GTW_autopilot type ────────────────────────────────────────────────
    var gwApNames=['NONE','HWY','EAP','FSD','BASIC'];
    var gwApRow=document.getElementById('rowLiveGWAP');
    if(d.gwAutopilot>=0){
      gwApRow.style.display='';
      document.getElementById('liveGWAP').textContent=gwApNames[d.gwAutopilot]||('AP'+d.gwAutopilot);
    } else {
      gwApRow.style.display='none';
    }
    // ── Torque / Gear / AP / Brake ───────────────────────────────────────
    var tq=(d.torqueR||0)*2;
    document.getElementById('liveTorque').textContent=tq?tq:'--';
    var gearMap=['--','P','R','N','D','--','--','--'];
    var gearEl=document.getElementById('liveGear');
    var g=d.gearRaw||0;
    gearEl.textContent=gearMap[g]||'--';
    gearEl.className='stat-val'+(g===2?' red':g===4?' green':'');
    var apEl=document.getElementById('liveAP');
    if(d.accState>0){apEl.textContent='ON';apEl.className='stat-val green';}
    else{apEl.textContent='--';apEl.className='stat-val';}
    var brakeEl=document.getElementById('liveBrake');
    if(d.brake){brakeEl.textContent='ON';brakeEl.className='stat-val red';}
    else{brakeEl.textContent='--';brakeEl.className='stat-val';}
    // ────────────────────────────────────────────────────────────────────
  }).catch(()=>{});
  logPollCount++;
  if(logPollCount%5===1)refreshDiagLog();
}
function setAPRestart(want){
  fetch('/api/aprestart?en='+(want?'1':'0')+(token?'&token='+token:''))
    .then(function(r){return r.json();})
    .then(function(res){if(res.ok)poll();});
}
var FW_VER=')rawliteral" FIRMWARE_VERSION R"rawliteral(';
var appStarted=false;
var logPollCount=0;
function refreshDiagLog(){
  var url='/api/log'+(token?'?token='+token:'');
  fetch(url).then(function(r){return r.text();}).then(function(txt){
    var el=document.getElementById('diagLog');
    el.value=txt;
    el.scrollTop=el.scrollHeight;
  }).catch(function(){});
}
function syncTime(){
  var ts=Math.floor(Date.now()/1000);
  fetch('/api/time'+(token?'?token='+token:''),{method:'POST',
    headers:{'Content-Type':'application/x-www-form-urlencoded'},
    body:'ts='+ts}).catch(function(){});
}
function startApp(){
  agreed=true;
  document.getElementById('disclaimer').style.display='none';
  if(appStarted)return;  // poll already running; only skip setInterval, not UI reset
  appStarted=true;
  setInterval(poll,1000);poll();
  syncTime();  // push browser clock to ESP32 for real timestamps in CSV
  brInit();    // probe for WiFi bridge endpoint (esp32s3-waveshare-wifi only)
}

// ── WiFi bridge card (shown only if backend supports it) ───────────
var brDirty=false;  // true while user editing DNS lists → skip overwrite
var brPollTimer=null;
function brInit(){
  fetch('/api/wifi-bridge/status'+(token?'?token='+token:'')).then(function(r){
    if(!r.ok)return;
    document.getElementById('cardBridge').style.display='';
    // Bridge supersedes the old single-SSID STA field; hide it to avoid two-config confusion
    var staSec=document.getElementById('staSection');
    if(staSec)staSec.style.display='none';
    brPoll();
    brPollTimer=setInterval(brPoll,3000);
    // mark dirty on any edit so polling doesn't overwrite user input
    ['brDnsAllow','brDnsBlock','brAddSsid','brAddPass'].forEach(function(id){
      var el=document.getElementById(id);
      if(el)el.addEventListener('input',function(){brDirty=true;});
    });
  }).catch(function(){});
}
function brPoll(){
  fetch('/api/wifi-bridge/status'+(token?'?token='+token:'')).then(function(r){return r.ok?r.json():null;}).then(function(d){
    if(!d)return;
    document.getElementById('brEn').checked=!!d.upstreamEnable;
    document.getElementById('brDnsEn').checked=!!d.dnsEnable;
    document.getElementById('brStat').textContent=d.upstreamStatus+(d.upstreamSignal?' · '+d.upstreamSignal:'')+(d.upstreamRSSI!==null?' · '+d.upstreamRSSI+' dBm':'');
    document.getElementById('brSSID').textContent=d.upstreamSSID||'--';
    document.getElementById('brNat').textContent=d.natStatus;
    var tempStr='--';
    if(d.chipTempC!==null){
      tempStr=d.chipTempC.toFixed(1)+'°C';
      if(d.chipTempAvgC!==null)tempStr+=' (avg '+d.chipTempAvgC.toFixed(1)+')';
      tempStr+=' · '+d.thermalStatus;
    }
    document.getElementById('brTemp').textContent=tempStr;
    // hotspot list
    var list=document.getElementById('brList');
    if(!d.upstreamNetworks.length){list.innerHTML='<span style="color:#64748b">无已保存热点</span>';}
    else{
      list.innerHTML=d.upstreamNetworks.map(function(n){
        var dot=n.connected?'🟢':(n.active?'🟡':'⚪');
        return '<div style="display:flex;justify-content:space-between;align-items:center;padding:4px 0;border-bottom:1px solid #1e293b">'+
          '<span>'+dot+' '+escHtml(n.ssid)+(n.hasPass?'':' <span style="color:#fbbf24">(开放)</span>')+'</span>'+
          '<button onclick="brDel('+JSON.stringify(n.ssid).replace(/"/g,'&quot;')+')" style="height:26px;padding:0 10px;background:#991b1b;color:#fee2e2;border:none;border-radius:6px;font-size:11px;cursor:pointer">删除</button>'+
          '</div>';
      }).join('');
    }
    if(!brDirty){
      document.getElementById('brDnsAllow').value=d.dnsAllow||'';
      document.getElementById('brDnsBlock').value=d.dnsBlock||'';
    }
    brHighlightPreset(d.dnsAllow,d.dnsBlock);
    document.getElementById('brBlkTot').textContent=d.dnsBlockedTotal;
    document.getElementById('brIpDrops').textContent=(d.ipBlockDrops!=null?d.ipBlockDrops:0);
    document.getElementById('brIpCache').textContent=(d.ipCacheCount!=null?d.ipCacheCount:0);
    document.getElementById('brIpPeak').textContent=(d.ipCachePeak!=null?d.ipCachePeak:0);
    var blk=document.getElementById('brBlkList');
    if(!d.dnsBlockedRecent.length){blk.innerHTML='<span style="color:#64748b">暂无拦截</span>';}
    else{
      blk.innerHTML=d.dnsBlockedRecent.map(function(e){
        return '<div style="display:flex;justify-content:space-between;padding:2px 0">'+
          '<span>'+escHtml(e.domain)+'</span><span style="color:#94a3b8">×'+e.count+'</span></div>';
      }).join('');
    }
  }).catch(function(){});
}
function escHtml(s){var d=document.createElement('div');d.textContent=s;return d.innerHTML;}
function brSet(){
  var qs='upstreamEnable='+(document.getElementById('brEn').checked?1:0)+
         '&dnsEnable='+(document.getElementById('brDnsEn').checked?1:0)+
         '&dnsAllow='+encodeURIComponent(document.getElementById('brDnsAllow').value)+
         '&dnsBlock='+encodeURIComponent(document.getElementById('brDnsBlock').value);
  fetch('/api/wifi-bridge/set?'+qs+(token?'&token='+token:''))
    .then(function(r){brDirty=false;msg('brMsg',r.ok?'已保存':'保存失败',r.ok);});
}
var BR_PRESETS={
  tesla_min:{name:'⭐ Tesla 推荐方案（实测）',
    allow:'connman.vn.cloud.tesla.cn nav-prd-maps.tesla.cn hermes-prd.vn.cloud.tesla.cn signaling.vn.cloud.tesla.cn media-server-me.tesla.cn www.tesla.cn maps-cn-prd.go.tesla.services volcengine.com volces.com volcengineapi.com volccdn.com api.map.baidu.com lc.map.baidu.com newvector.map.baidu.com route.map.baidu.com newclient.map.baidu.com tracknavi.baidu.com itsmap3.baidu.com app.navi.baidu.com mapapip0.bdimg.com mapapisp0.bdimg.com automap0.bdimg.com baidunavi.cdn.bcebos.com lbsnavi.cdn.bcebos.com enlargeroad-view.su.bcebos.com',
    block:'tesla.cn tesla.com teslamotors.com tesla.services',
    note:'屏蔽全部 Tesla 云域名，仅放行核心子域：\n• Wi-Fi 检测 connman + www.tesla.cn\n• Tesla 地图导航 nav-prd-maps + maps-cn-prd\n• 手机 APP 控车 hermes + signaling\n• Intel 车机语音 media-server-me\n• AMD 车机语音 volcengine/volces/volcengineapi/volccdn（字节火山引擎）\n• 百度地图/导航 *.map.baidu.com + *.bdimg.com + *.bcebos.com（车机内置地图需要）\n\n✅ 已实测 2 天无异常（APP 控车/导航/语音/Wi-Fi 正常工作）。推荐优先使用此方案。'},
  bl_telemetry:{name:'屏蔽遥测（中风险）',
    allow:'',
    block:'hermes-stream-prd.vn.cloud.tesla.cn vehicle-files.prd.cnn1.vn.cloud.tesla.cn vehicle-files.prd.cn1.vn.cloud.tesla.cn firmware.tesla.cn',
    note:'拦 4 条 Tesla 遥测/上传/OTA 端点：\n• hermes-stream-prd（车辆数据流）\n• vehicle-files prd.cnn1/cn1（车辆日志/文件上传）\n• firmware.tesla.cn（OTA 固件下载）\n\nhermes-stream 风险略高，可能影响部分推送/远程指令。屏蔽 firmware.tesla.cn 会阻止 OTA 升级。'},
  clear:{name:'清空',allow:'',block:'',note:'清空所有过滤规则'}
};
var PRESET_IDS_PH={tesla_min:'pstTesla',bl_telemetry:'pstBl',clear:'pstClr'};
function normListPh(s){return String(s||'').trim().split(/\s+/).filter(Boolean).sort().join(' ');}
function brHighlightPreset(allow,block){
  var a=normListPh(allow),b=normListPh(block);
  var active=null;
  for(var k in BR_PRESETS){
    if(normListPh(BR_PRESETS[k].allow)===a && normListPh(BR_PRESETS[k].block)===b){active=k;break;}
  }
  for(var kk in PRESET_IDS_PH){
    var el=document.getElementById(PRESET_IDS_PH[kk]);
    if(el) el.style.boxShadow=(kk===active)?'0 0 0 3px #fbbf24':'none';
  }
}
function brApplyPreset(key){
  var p=BR_PRESETS[key];if(!p)return;
  if(!confirm('应用预设「'+p.name+'」？\n\n' + p.note + '\n\n⚠ 预设内容来自网络收集，仅供参考，准确性请自行验证。\n注意：会覆盖当前白/黑名单文本框内容。')){return;}
  document.getElementById('brDnsAllow').value=p.allow;
  document.getElementById('brDnsBlock').value=p.block;
  brDirty=true;
  msg('brMsg','已载入「'+p.name+'」，请点保存 DNS 配置',true);
}
function brAdd(){
  var ssid=document.getElementById('brAddSsid').value.trim();
  var pass=document.getElementById('brAddPass').value;
  if(!ssid){msg('brMsg','请填写 SSID',false);return;}
  var qs='ssid='+encodeURIComponent(ssid)+'&pass='+encodeURIComponent(pass);
  fetch('/api/wifi-bridge/add?'+qs+(token?'&token='+token:''))
    .then(function(r){
      if(r.ok){
        document.getElementById('brAddSsid').value='';
        document.getElementById('brAddPass').value='';
        document.getElementById('brScanList').style.display='none';
        brDirty=false;
        msg('brMsg','已保存',true);
      }else{r.text().then(function(t){msg('brMsg','失败：'+t,false);});}
    });
}
function brDel(ssid){
  if(!confirm('删除 '+ssid+' ?'))return;
  fetch('/api/wifi-bridge/delete?ssid='+encodeURIComponent(ssid)+(token?'&token='+token:''))
    .then(function(r){msg('brMsg',r.ok?'已删除':'删除失败',r.ok);});
}
function brBlkClear(){
  fetch('/api/wifi-bridge/blocked-clear'+(token?'?token='+token:''))
    .then(function(r){msg('brMsg',r.ok?'已清空':'清空失败',r.ok);});
}
function msg(id,text,ok){
  var el=document.getElementById(id);
  el.textContent=text;
  el.style.color=ok?'#34d399':'#f87171';
  setTimeout(function(){el.textContent='';},2500);
}
function showPinStep(){
  document.getElementById('disclaimerContent').style.display='none';
  document.getElementById('pinStep').style.display='';
  document.getElementById('disclaimer').style.display='flex';
  setTimeout(function(){document.getElementById('pinInput').focus();},100);
}
function submitPin(){
  var pin=document.getElementById('pinInput').value;
  document.getElementById('pinError').style.display='none';
  fetch('/api/auth',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'pin='+encodeURIComponent(pin)})
  .then(function(r){if(r.ok)return r.text();throw new Error('wrong');})
  .then(function(tok){
    token=tok;
    try{sessionStorage.setItem('fsd_tok',tok);}catch(e){}
    startApp();
  })
  .catch(function(){
    document.getElementById('pinError').style.display='block';
    document.getElementById('pinInput').value='';
    document.getElementById('pinInput').focus();
  });
}
function confirmDisclaimer(){
  try{localStorage.setItem('disclaimed',FW_VER);}catch(e){}
  if(pinRequired&&!token){showPinStep();}else{startApp();}
}
function rejectDisclaimer(){
  document.getElementById('disclaimerBtns').style.display='none';
  document.getElementById('disclaimerRejected').style.display='block';
}
// On load: fetch status to get pinRequired, then decide whether to skip disclaimer.
// Skip only if user already agreed to this exact firmware version.
// Factory reset clears localStorage.disclaimed so the disclaimer reappears after reset.
fetch('/api/status').then(function(r){return r.json();}).then(function(d){
  pinRequired=!!d.pinRequired;
  var disclaimed=false;
  try{disclaimed=localStorage.getItem('disclaimed')===FW_VER;}catch(e){}
  if(disclaimed){
    if(!pinRequired){startApp();}
    else if(token){startApp();}
    else{showPinStep();}
  }else{
    document.getElementById('disclaimer').style.display='flex';
  }
}).catch(function(){
  document.getElementById('disclaimer').style.display='flex';
});
var wifiSSIDLoaded=false;
var staSSIDLoaded=false;
function updateSpeedOptions(hwMode){
  // HW3/Legacy: 3 profiles (0-2) with different labels than HW4
  // HW4: 5 profiles (0-4) — Sloth/Chill/Standard/Hurry/Mad Max
  var hw3Labels=[
    {zh:'轻松',en:'Chill'},
    {zh:'标准',en:'Standard'},
    {zh:'迅捷',en:'Sport'}
  ];
  var hw4Labels=[
    {zh:'平缓',en:'Sloth'},
    {zh:'舒适',en:'Chill'},
    {zh:'标准',en:'Standard'},
    {zh:'迅捷',en:'Hurry'},
    {zh:'狂飙',en:'Mad Max'}
  ];
  var isHW4=(hwMode===2);
  var labels=isHW4?hw4Labels:hw3Labels;
  var sel=document.getElementById('speedProfile');
  Array.from(sel.options).forEach(function(o){
    var v=parseInt(o.value);
    var hide=(!isHW4&&v>2);
    o.disabled=hide;
    o.style.display=hide?'none':'';
    if(!hide&&labels[v]){
      o.setAttribute('data-zh',labels[v].zh);
      o.setAttribute('data-en',labels[v].en);
      o.textContent=(lang==='en')?labels[v].en:labels[v].zh;
    }
  });
  // If current selection is now invalid, clamp to 2
  if(!isHW4&&parseInt(sel.value)>2){sel.value='2';setVal('speedProfile',2);}
  // Show HW3 rows only for HW3 mode
  var isHW3=(hwMode===1);
  document.getElementById('rowHW3Smart').style.display=isHW3?'':'none';
  document.getElementById('rowTrackMode').style.display=isHW3?'':'none';
  var smartOn=isHW3&&document.getElementById('hw3Smart').checked;
  // 智能开启时隐藏手动偏移下拉，避免用户误以为手动值在生效
  document.getElementById('rowHW3Offset').style.display=(isHW3&&!smartOn)?'':'none';
  document.getElementById('rowHW3SmartRules').style.display=smartOn?'':'none';
  // HW4-only rows — hide and reset on other modes
  document.getElementById('rowHW4Offset').style.display=isHW4?'':'none';
  document.getElementById('rowEmgDet').style.display=isHW4?'':'none';
  // Legacy-only rows
  document.getElementById('rowOverrideSL').style.display=(hwMode===0)?'':'none';
  if(!isHW4){
    var emgEl=document.getElementById('emergencyDet');
    if(emgEl&&emgEl.checked){emgEl.checked=false;setVal('emergencyDet',0);}
  }
}
function setVal(key,val){
  if(!agreed)return;
  var url='/api/set?'+key+'='+val+(token?'&token='+token:'');
  var ctrl=document.getElementById(key);
  var prevVal=ctrl?(ctrl.type==='checkbox'?ctrl.checked:(ctrl.tagName==='SELECT'?ctrl.value:null)):null;
  var ctrl2=ctrl;
  var ac=new AbortController();
  var tid=setTimeout(function(){ac.abort();},3000);
  fetch(url,{signal:ac.signal})
    .then(function(r){
      clearTimeout(tid);
      if(r.status===403){token='';try{sessionStorage.removeItem('fsd_tok');}catch(e){}showPinStep();}
      else if(!r.ok && prevVal!==null && ctrl2){
        // Revert UI on non-200
        if(ctrl2.type==='checkbox')ctrl2.checked=prevVal;
        else if(ctrl2.tagName==='SELECT')ctrl2.value=prevVal;
      }
    })
    .catch(function(){
      clearTimeout(tid);
      // Revert UI on network error / timeout
      if(prevVal!==null && ctrl2){
        if(ctrl2.type==='checkbox')ctrl2.checked=prevVal;
        else if(ctrl2.tagName==='SELECT')ctrl2.value=prevVal;
      }
    });
}
function onTrackModeChange(el){
  if(el.checked){
    if(!confirm(T[lang].trackModeWarn)){el.checked=false;return;}
  }
  setVal('trackMode',el.checked?1:0);
}
function setHW3Smart(enabled){
  setVal('hw3Smart',enabled?1:0);
  document.getElementById('rowHW3Offset').style.display=enabled?'none':'';
  document.getElementById('rowHW3SmartRules').style.display=enabled?'':'none';
}
var smartDirty=false;
function markSmartDirty(){ smartDirty=true; }
function updateSmartLabels(){
  var t1=parseInt(document.getElementById('hw3SmT1').value)||40;
  var t2=parseInt(document.getElementById('hw3SmT2').value)||60;
  var t3=parseInt(document.getElementById('hw3SmT3').value)||80;
  var t4=parseInt(document.getElementById('hw3SmT4').value)||100;
  document.getElementById('sSmLbl2').textContent=t1;
  document.getElementById('sSmLbl3').textContent=t2;
  document.getElementById('sSmLbl4').textContent=t3;
  document.getElementById('sSmLbl5').textContent=t4;
}
function saveSmartRules(){
  if(!agreed)return;
  var t1=parseInt(document.getElementById('hw3SmT1').value)||40;
  var t2=parseInt(document.getElementById('hw3SmT2').value)||60;
  var t3=parseInt(document.getElementById('hw3SmT3').value)||80;
  var t4=parseInt(document.getElementById('hw3SmT4').value)||100;
  var o1=parseInt(document.getElementById('hw3SmO1').value);if(isNaN(o1))o1=50;
  var o2=parseInt(document.getElementById('hw3SmO2').value);if(isNaN(o2))o2=25;
  var o3=parseInt(document.getElementById('hw3SmO3').value);if(isNaN(o3))o3=15;
  var o4=parseInt(document.getElementById('hw3SmO4').value);if(isNaN(o4))o4=10;
  var o5=parseInt(document.getElementById('hw3SmO5').value);if(isNaN(o5))o5=8;
  // Clamp: T1<T2<T3<T4, offsets 0-50 (matches backend + car UI)
  t1=Math.max(20,Math.min(t1,195));
  t2=Math.max(t1+1,Math.min(t2,200));
  t3=Math.max(t2+1,Math.min(t3,200));
  t4=Math.max(t3+1,Math.min(t4,200));
  o1=Math.max(0,Math.min(o1,50));o2=Math.max(0,Math.min(o2,50));o3=Math.max(0,Math.min(o3,50));
  o4=Math.max(0,Math.min(o4,50));o5=Math.max(0,Math.min(o5,50));
  // Update inputs after clamping
  document.getElementById('hw3SmT1').value=t1;document.getElementById('hw3SmT2').value=t2;
  document.getElementById('hw3SmT3').value=t3;document.getElementById('hw3SmT4').value=t4;
  updateSmartLabels();
  var url='/api/set?hw3SmT1='+t1+'&hw3SmT2='+t2+'&hw3SmT3='+t3+'&hw3SmT4='+t4
        +'&hw3SmO1='+o1+'&hw3SmO2='+o2+'&hw3SmO3='+o3+'&hw3SmO4='+o4+'&hw3SmO5='+o5+(token?'&token='+token:'');
  var btn=document.getElementById('iSmSaveBtn');
  fetch(url).then(function(r){
    if(r.status===403){token='';try{sessionStorage.removeItem('fsd_tok');}catch(e){}showPinStep();}
    else if(r.ok){smartDirty=false; if(btn){btn.textContent='✓';setTimeout(function(){btn.textContent='保存';},1500);}}
    else {
      // Backend rejected (e.g. 400 validation) — clear dirty so poll can re-sync, flag to user.
      smartDirty=false;
      if(btn){btn.textContent='✗';setTimeout(function(){btn.textContent='保存';},1500);}
    }
  }).catch(function(){
    // Network error — clear dirty so poll re-syncs; user can retry.
    smartDirty=false;
    if(btn){btn.textContent='✗';setTimeout(function(){btn.textContent='保存';},1500);}
  });
}
function resetSmartRules(){
  if(!agreed)return;
  document.getElementById('hw3SmT1').value=40;
  document.getElementById('hw3SmT2').value=60;
  document.getElementById('hw3SmT3').value=80;
  document.getElementById('hw3SmT4').value=100;
  document.getElementById('hw3SmO1').value=50;
  document.getElementById('hw3SmO2').value=25;
  document.getElementById('hw3SmO3').value=15;
  document.getElementById('hw3SmO4').value=10;
  document.getElementById('hw3SmO5').value=8;
  saveSmartRules();
}
function doWifi(){
  var t=T[lang];
  var ssid=document.getElementById('wifiSSID').value.trim();
  var pass=document.getElementById('wifiPass').value;
  var msg=document.getElementById('wifiMsg');
  if(!ssid){msg.textContent=t.wifiSSIDErr;msg.className='msg err';return;}
  if(pass.length>0&&pass.length<8){msg.textContent=t.wifiPassErr;msg.className='msg err';return;}
  var btn=document.getElementById('wifiSaveBtn');
  btn.disabled=true;
  var body='ssid='+encodeURIComponent(ssid)+(pass.length>=8?'&pass='+encodeURIComponent(pass):'');
  fetch('/api/wifi'+(token?'?token='+token:''),{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:body})
    .then(r=>r.text()).then(txt=>{
      if(txt==='OK'){msg.textContent=t.wifiOK;msg.className='msg ok';}
      else{msg.textContent=t.wifiFail+txt;msg.className='msg err';btn.disabled=false;}
    }).catch(()=>{msg.textContent=t.wifiFail+'connection error';msg.className='msg err';btn.disabled=false;});
}
function doScan(){
  var t=T[lang];
  var btn=document.getElementById('scanBtn');
  var list=document.getElementById('scanList');
  btn.disabled=true;
  btn.textContent=t.scanScanning;
  list.style.display='none';
  // Kick off async scan — returns immediately, no server blocking
  fetch('/api/scan'+(token?'?token='+token:''))
    .then(function(){pollScanResult(0);})
    .catch(function(){
      var msg=document.getElementById('staMsg');
      msg.textContent=t.scanFail;msg.className='msg err';
      setTimeout(function(){msg.textContent='';},2000);
      btn.disabled=false;btn.textContent=t.scanBtn;
    });
}
function pollScanResult(attempt){
  var t=T[lang];
  var btn=document.getElementById('scanBtn');
  var list=document.getElementById('scanList');
  if(attempt>20){// 20×500ms = 10s timeout
    btn.disabled=false;btn.textContent=t.scanBtn;
    var msg=document.getElementById('staMsg');
    msg.textContent=t.scanFail;msg.className='msg err';
    setTimeout(function(){msg.textContent='';},2000);
    return;
  }
  fetch('/api/scan/result'+(token?'?token='+token:''))
    .then(function(r){return r.json();})
    .then(function(data){
      if(data.scanning){setTimeout(function(){pollScanResult(attempt+1);},500);return;}
      // Got results array
      var nets=data;
      nets.sort(function(a,b){return b.rssi-a.rssi;});
      while(list.options.length>1)list.remove(1);
      nets.forEach(function(n){
        var o=document.createElement('option');
        var bars=n.rssi>=-60?'▂▄▆█':n.rssi>=-75?'▂▄▆_':n.rssi>=-85?'▂▄__':'▂___';
        o.value=n.ssid;o.textContent=bars+' '+n.ssid;
        list.appendChild(o);
      });
      list.value='';
      list.style.display=nets.length?'':'none';
      btn.disabled=false;btn.textContent=t.scanBtn;
    })
    .catch(function(){
      btn.disabled=false;btn.textContent=t.scanBtn;
    });
}
function scanPick(sel){
  if(sel.value)document.getElementById('staSSID').value=sel.value;
  sel.style.display='none';
}
function doSTA(){
  var t=T[lang];
  var ssid=document.getElementById('staSSID').value.trim();
  var pass=document.getElementById('staPass').value;
  var msg=document.getElementById('staMsg');
  var btn=document.getElementById('staSaveBtn');
  btn.disabled=true;
  var body='ssid='+encodeURIComponent(ssid)+'&pass='+encodeURIComponent(pass);
  fetch('/api/sta'+(token?'?token='+token:''),{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:body})
    .then(r=>r.text()).then(txt=>{
      if(txt==='OK'){msg.textContent=t.staOK;msg.className='msg ok';}
      else{msg.textContent=t.staFail+': '+txt;msg.className='msg err';btn.disabled=false;}
    }).catch(()=>{msg.textContent=t.staFail;msg.className='msg err';btn.disabled=false;});
}
function doSTAClear(){
  var t=T[lang];
  var msg=document.getElementById('staMsg');
  var body='ssid=&pass=';
  fetch('/api/sta'+(token?'?token='+token:''),{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:body})
    .then(r=>r.text()).then(txt=>{
      if(txt==='OK'){
        document.getElementById('staSSID').value='';
        document.getElementById('staPass').value='';
        staSSIDLoaded=false;
        msg.textContent=t.staOK;msg.className='msg ok';
      }
    }).catch(()=>{});
}
function fileChosen(inp){
  var fn=document.getElementById('fileName');
  if(inp.files[0]){fn.textContent=inp.files[0].name;fn.dataset.hasFile='1';}
  else{delete fn.dataset.hasFile;fn.textContent=T[lang].noFile;}
  document.getElementById('uploadBtn').disabled=!inp.files[0];
}
function doPin(){
  var pin=document.getElementById('accessPin').value;
  var msg=document.getElementById('pinMsg');
  if(pin.length>0&&pin.length<4){msg.textContent='密码至少 4 位';msg.className='msg err';return;}
  fetch('/api/pin'+(token?'?token='+token:''),{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'pin='+encodeURIComponent(pin)})
  .then(function(r){
    if(r.ok){
      pinRequired=pin.length>0;
      msg.textContent=pin?'密码已设置，下次访问需要输入密码':'密码已清除';
      msg.className='msg ok';
      document.getElementById('accessPin').value='';
      token='';try{sessionStorage.removeItem('fsd_tok');}catch(e){}
    }else{msg.textContent='设置失败（请确认当前已验证）';msg.className='msg err';}
  }).catch(function(){msg.textContent='连接失败';msg.className='msg err';});
}
function doLogCopy(){
  var t=T[lang];
  var txt=document.getElementById('diagLog').value;
  var msg=document.getElementById('logMsg');
  if(navigator.clipboard){
    navigator.clipboard.writeText(txt).then(function(){
      msg.textContent=t.logCopied;msg.className='msg ok';
      setTimeout(function(){msg.textContent='';},2000);
    }).catch(function(){
      msg.textContent='复制失败';msg.className='msg err';
      setTimeout(function(){msg.textContent='';},2000);
    });
  } else {
    var ta=document.createElement('textarea');
    ta.value=txt;ta.style.position='fixed';ta.style.opacity='0';
    document.body.appendChild(ta);ta.focus();ta.select();
    try{document.execCommand('copy');msg.textContent=t.logCopied;msg.className='msg ok';}
    catch(e){msg.textContent='复制失败';msg.className='msg err';}
    document.body.removeChild(ta);
    setTimeout(function(){msg.textContent='';},2000);
  }
}
function doLogClear(){
  var t=T[lang];
  fetch('/api/log/clear'+(token?'?token='+token:''),{method:'POST'})
  .then(function(r){
    if(r.ok){
      document.getElementById('diagLog').value='';
      var msg=document.getElementById('logMsg');
      msg.textContent=t.logCleared;msg.className='msg ok';
      setTimeout(function(){msg.textContent='';},2000);
    }
  }).catch(function(){});
}
function doLogDownload(){
  var url='/api/log/download'+(token?'?token='+token:'');
  var a=document.createElement('a');
  a.href=url;a.download='diag.log';
  document.body.appendChild(a);a.click();document.body.removeChild(a);
}
function doOTA(){
  var file=document.getElementById('fwFile').files[0];
  if(!file)return;
  var t=T[lang];
  var xhr=new XMLHttpRequest();
  var prog=document.getElementById('progWrap');
  var bar=document.getElementById('progBar');
  var msg=document.getElementById('otaMsg');
  prog.style.display='block';bar.style.width='0%';msg.textContent='';msg.className='msg';
  document.getElementById('uploadBtn').disabled=true;
  xhr.upload.addEventListener('progress',e=>{if(e.lengthComputable)bar.style.width=Math.round(e.loaded/e.total*100)+'%';});
  xhr.onload=function(){
    if(xhr.status===200){msg.textContent=t.otaOK;msg.className='msg ok';}
    else{msg.textContent=t.otaFail+xhr.statusText;msg.className='msg err';document.getElementById('uploadBtn').disabled=false;}
  };
  xhr.onerror=function(){msg.textContent=t.otaConn;msg.className='msg err';document.getElementById('uploadBtn').disabled=false;};
  var form=new FormData();form.append('firmware',file);
  xhr.open('POST','/api/ota'+(token?'?token='+token:''));xhr.send(form);
}
function showRebootConfirm(){
  document.getElementById('rebootConfirmBox').style.display='';
  document.getElementById('rebootBtn').style.display='none';
}
function hideRebootConfirm(){
  document.getElementById('rebootConfirmBox').style.display='none';
  document.getElementById('rebootBtn').style.display='';
}
function doReboot(){
  var t=T[lang];
  var msg=document.getElementById('rebootMsg');
  document.getElementById('iRebootConfirmBtn').disabled=true;
  msg.textContent='';msg.className='msg';
  fetch('/api/reboot'+(token?'?token='+token:''))
    .then(function(r){
      document.getElementById('rebootConfirmBox').style.display='none';
      if(r.ok){msg.textContent=t.rebootOK;msg.className='msg ok';}
      else{msg.textContent=t.rebootFail;msg.className='msg err';hideRebootConfirm();}
    })
    .catch(function(){
      document.getElementById('rebootConfirmBox').style.display='none';
      msg.textContent=t.rebootOK;msg.className='msg ok';
    });
}
function showResetConfirm(){
  document.getElementById('resetAllConfirmBox').style.display='';
  document.getElementById('iResetAllBtn').style.display='none';
}
function hideResetConfirm(){
  document.getElementById('resetAllConfirmBox').style.display='none';
  document.getElementById('iResetAllBtn').style.display='';
}
function doResetAll(){
  var t=T[lang];
  var msg=document.getElementById('resetAllMsg');
  document.getElementById('iResetAllConfirmBtn').disabled=true;
  msg.textContent='';msg.className='msg';
  fetch('/api/reset'+(token?'?token='+token:''),{method:'POST'})
    .then(function(r){
      document.getElementById('resetAllConfirmBox').style.display='none';
      if(r.ok){
        try{localStorage.removeItem('disclaimed');}catch(e){}
        msg.textContent=t.resetAllOK;msg.className='msg ok';
      }else{msg.textContent=t.resetAllFail;msg.className='msg err';hideResetConfirm();}
    })
    .catch(function(){
      document.getElementById('resetAllConfirmBox').style.display='none';
      try{localStorage.removeItem('disclaimed');}catch(e){}
      msg.textContent=t.resetAllOK;msg.className='msg ok';
    });
}

// 键盘避让：输入框获得焦点时滚动到视口中央，避免被软键盘挡住
document.addEventListener('focusin',function(e){
  var t=e.target;
  if(!t||!(t.matches&&t.matches('input,textarea')))return;
  setTimeout(function(){try{t.scrollIntoView({block:'center',behavior:'smooth'});}catch(_){}} ,300);
});
</script>
</body>
</html>
)rawliteral";
