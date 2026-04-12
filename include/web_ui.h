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
.toggle{position:relative;width:50px;height:28px}
.toggle input{opacity:0;width:0;height:0}
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
</style>
</head>
<body>

<div id="disclaimer" style="position:fixed;top:0;left:0;right:0;bottom:0;background:rgba(0,0,0,0.88);z-index:999;display:flex;align-items:center;justify-content:center;padding:20px">
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
  display:block;width:100%;margin-bottom:16px;padding:14px;
  background:linear-gradient(135deg,#0d4f4f,#0a7a6a);
  color:#00d4aa;border:1px solid #00d4aa44;border-radius:14px;
  font-size:15px;font-weight:700;cursor:pointer;letter-spacing:1px;
  text-align:center;
" id="iDashBtn">⬤ 仪表盘 · DASHBOARD</button>

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
  <div class="row">
    <span class="row-label" id="iLblEmg">紧急车辆检测</span>
    <label class="toggle"><input type="checkbox" id="emergencyDet" checked onchange="setVal('emergencyDet',this.checked?1:0)"><span class="slider"></span></label>
  </div>
  <div class="row">
    <span class="row-label" id="iLblCN">强制激活</span>
    <label class="toggle"><input type="checkbox" id="forceActivate" onchange="setVal('forceActivate',this.checked?1:0)"><span class="slider"></span></label>
  </div>
  <div class="row">
    <span class="row-label" id="iLblPrecond">电池预热</span>
    <label class="toggle"><input type="checkbox" id="precond" onchange="setVal('precond',this.checked?1:0)"><span class="slider"></span></label>
  </div>
  <div class="row hb-row">
    <span class="row-label" id="iLblHB">强制远光<br><span class="hb-hint" id="iHBHint">需开启自适应灯光</span></span>
    <label class="toggle"><input type="checkbox" id="hbForce" disabled onchange="toggleHighBeam(this.checked)"><span class="slider"></span></label>
  </div>
  <div class="row">
    <span class="row-label" id="iLblAPRestart">AP 自动恢复</span>
    <label class="toggle"><input type="checkbox" id="apRestart" onchange="setAPRestart(this.checked)"><span class="slider"></span></label>
  </div>
  <div class="row" id="rowHW3Cap" style="display:none">
    <span class="row-label" id="iLblHW3Cap">限速保护（×1.2）</span>
    <label class="toggle"><input type="checkbox" id="hw3Cap" onchange="setVal('hw3Cap',this.checked?1:0)"><span class="slider"></span></label>
  </div>
  <div class="row" id="rowHW3Offset" style="display:none">
    <span class="row-label" id="iLblHW3Off">速度偏移（HW3）</span>
    <select id="hw3Offset" onchange="setVal('hw3Offset',this.value)">
      <option value="-1" data-zh="自动" data-en="Auto">自动</option>
      <option value="0" data-zh="0%" data-en="0%">0%</option>
      <option value="5" data-zh="5%" data-en="5%">5%</option>
      <option value="10" data-zh="10%" data-en="10%">10%</option>
      <option value="15" data-zh="15%" data-en="15%">15%</option>
      <option value="20" data-zh="20%" data-en="20%">20%</option>
      <option value="25" data-zh="25%" data-en="25%">25%</option>
      <option value="30" data-zh="30%" data-en="30%">30%</option>
      <option value="35" data-zh="35%" data-en="35%">35%</option>
      <option value="40" data-zh="40%" data-en="40%">40%</option>
      <option value="45" data-zh="45%" data-en="45%">45%</option>
      <option value="50" data-zh="50%" data-en="50%">50%</option>
    </select>
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
  <div class="status-row"><span id="iLblFSDTrig">FSD 已触发</span><span id="sFSD" class="status-no">--</span></div>
  <div class="status-row" id="rowBMS" style="display:none">
    <span id="iLblBMS">电池 <span style="color:#64748b;font-size:11px">（数据未经车辆验证）</span></span>
    <span id="sBMS" style="color:#38bdf8;font-weight:600;font-size:13px">--</span>
  </div>
  <div class="status-row"><span id="iLblTimeSync">时间同步</span><span id="sTimeSync" class="status-no">--</span></div>
  <div class="status-row"><span id="iLblHeap">空闲内存</span><span id="sHeap" style="color:#64748b;font-weight:600">--</span></div>
  <div class="status-row"><span id="iLblVer">固件版本</span><span id="sVer" style="color:#64748b;font-weight:600">--</span></div>
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
  <div style="margin-top:18px;padding-top:18px;border-top:1px solid #1e293b">
    <div style="font-size:12px;font-weight:700;color:#64748b;letter-spacing:2px;margin-bottom:12px" id="iLblStaSection">连接路由器（内网访问）</div>
    <div style="color:#64748b;font-size:12px;margin-bottom:10px" id="iLblStaDesc">填写后模块将同时连接路由器，可通过内网 IP 访问，热点仍保留。留空则只用热点。</div>
    <div class="row" style="flex-direction:column;align-items:flex-start;gap:4px;margin-bottom:8px">
      <span class="row-label" id="iLblStaSSID">路由器 SSID</span>
      <input type="text" id="staSSID" class="text-input" maxlength="32" placeholder="路由器名称">
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

<div class="card">
  <div class="card-title" id="iCardLog">诊断日志</div>
  <div style="font-size:11px;color:#64748b;padding:4px 0 8px" id="iLblLogNote">掉电清除 · 最近 80 条事件 · 遇到问题请复制发给我们</div>
  <textarea id="diagLog" readonly style="width:100%;height:160px;background:#0f172a;color:#94a3b8;border:1px solid #334155;border-radius:8px;padding:8px;font-size:11px;font-family:monospace;resize:vertical;box-sizing:border-box"></textarea>
  <div style="display:flex;gap:10px;margin-top:8px">
    <button class="save-btn" id="logCopyBtn" onclick="doLogCopy()" style="flex:3">复制日志</button>
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
    lblCAN:'CAN 总线',lblFSDTrig:'FSD 已触发',
    lblFile:'选择文件',noFile:'未选择文件',uploadBtn:'上传固件',
    lblVer:'固件版本',
    canOK:'正常',canErr:'异常',fsdYes:'是',fsdNo:'否',
    otaOK:'上传成功，正在重启...',otaFail:'上传失败: ',otaConn:'连接失败',
    uptH:'时',uptM:'分',uptS:'秒',langBtn:'EN',
    hwHint:'HW4 硬件 + 固件 2026.8.x 或更旧（FSD V13）→ 请选 HW3',
    cardWifi:'WiFi 设置',lblSSID:'热点名称（SSID）',lblPass:'新密码（留空保持不变）',
    lblHW3Cap:'限速保护（×1.2）',lblHW3Off:'速度偏移（HW3）',lblPrecond:'电池预热',lblBMS:'电池',
    wifiSave:'保存并重启',wifiOK:'已保存，正在重启...',wifiFail:'保存失败: ',
    wifiPassErr:'密码至少 8 位',wifiSSIDErr:'SSID 不能为空',
    lblStaSection:'连接路由器（内网访问）',lblStaDesc:'填写后模块将同时连接路由器，可通过内网 IP 访问，热点仍保留。留空则只用热点。',
    lblStaSSID:'路由器 SSID',lblStaPass:'路由器密码',lblStaStatus:'状态：',
    staConnected:'已连接',staDisconnected:'未连接',staSave:'保存并重启',staClear:'断开',
    staOK:'已保存，正在重启...',staFail:'保存失败',
    cardLog:'诊断日志',lblLogNote:'掉电清除 · 最近 80 条事件 · 遇到问题请复制发给我们',
    logCopyBtn:'复制日志',logClrBtn:'清除',logCleared:'已清除',logCopied:'已复制',
    lblTimeSync:'时间同步',timeSyncOK:'已同步',timeSyncNo:'未同步',
    lblHB:'强制远光',hbHintOff:'需开启自适应灯光',hbHintOn:'自适应灯光已就绪',
    lblAPRestart:'AP 自动恢复',
    hwDetHW3:'CAN 检测到：HW3',hwDetHW4:'CAN 检测到：HW4',
    hwDetNone:'未自动检测到，请手动选择（2020+ 车型不支持）'},
  en:{title:'Tesla Controller',cardCtrl:'CONTROL',cardStat:'STATUS',cardOTA:'OTA UPDATE',
    lblFsdEn:'FSD Enable',lblHW:'Hardware',lblSpeed:'Speed Profile',lblPMode:'Profile Source',
    lblISA:'ISA Chime Suppress',lblEmg:'Emergency Detection',lblCN:'Force Activate',
    lblHW3Cap:'Speed Limit Guard (×1.2)',lblHW3Off:'HW3 Speed Offset',lblPrecond:'Battery Preheat',lblBMS:'Battery',
    lblMod:'MODIFIED',lblRX:'RECEIVED',lblErr:'ERRORS',lblUp:'UPTIME',
    lblCAN:'CAN Bus',lblFSDTrig:'FSD Triggered',
    lblFile:'Choose File',noFile:'No file chosen',uploadBtn:'Upload Firmware',
    lblVer:'Firmware Version',
    canOK:'OK',canErr:'ERROR',fsdYes:'Yes',fsdNo:'No',
    otaOK:'Upload success, rebooting...',otaFail:'Upload failed: ',otaConn:'Connection error',
    uptH:'h',uptM:'m',uptS:'s',langBtn:'中文',
    hwHint:'HW4 hardware + firmware 2026.8.x or older (FSD V13) → select HW3',
    cardWifi:'WiFi Settings',lblSSID:'AP Name (SSID)',lblPass:'New Password (blank = keep current)',
    wifiSave:'Save & Restart',wifiOK:'Saved, rebooting...',wifiFail:'Save failed: ',
    wifiPassErr:'Password must be ≥ 8 chars',wifiSSIDErr:'SSID cannot be empty',
    lblStaSection:'Connect to Router (LAN access)',lblStaDesc:'Module will also connect to your router — access via LAN IP. Hotspot remains as fallback. Leave blank for hotspot-only.',
    lblStaSSID:'Router SSID',lblStaPass:'Router Password',lblStaStatus:'Status:',
    staConnected:'Connected',staDisconnected:'Not connected',staSave:'Save & Restart',staClear:'Disconnect',
    staOK:'Saved, rebooting...',staFail:'Save failed',
    cardLog:'Diagnostic Log',lblLogNote:'Cleared on power-off · last 80 events · copy and send if issues occur',
    logCopyBtn:'Copy Log',logClrBtn:'Clear',logCleared:'Cleared',logCopied:'Copied',
    lblTimeSync:'Time Sync',timeSyncOK:'Synced',timeSyncNo:'Not synced',
    lblHB:'Force High Beam',hbHintOff:'Requires adaptive lighting',hbHintOn:'Adaptive lighting ready',
    lblAPRestart:'AP Auto-Resume',
    hwDetHW3:'CAN detected: HW3',hwDetHW4:'CAN detected: HW4',
    hwDetNone:'Auto-detect failed — select manually (2020+ vehicles not supported)'}
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
  document.getElementById('iLblHW3Cap').textContent=t.lblHW3Cap;
  document.getElementById('iLblHW3Off').textContent=t.lblHW3Off;
  document.getElementById('iLblPrecond').textContent=t.lblPrecond;
  document.getElementById('iLblBMS').textContent=t.lblBMS;
  document.getElementById('iLblMod').textContent=t.lblMod;
  document.getElementById('iLblRX').textContent=t.lblRX;
  document.getElementById('iLblErr').textContent=t.lblErr;
  document.getElementById('iLblUp').textContent=t.lblUp;
  document.getElementById('iLblCAN').textContent=t.lblCAN;
  document.getElementById('iLblFSDTrig').textContent=t.lblFSDTrig;
  document.getElementById('iLblVer').textContent=t.lblVer;
  document.getElementById('iHWHint').textContent=t.hwHint;
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
  document.getElementById('iLblHB').textContent=t.lblHB;
  document.getElementById('iLblAPRestart').textContent=t.lblAPRestart;
  document.getElementById('iLblTimeSync').textContent=t.lblTimeSync;
  document.getElementById('iCardLog').textContent=t.cardLog;
  document.getElementById('iLblLogNote').textContent=t.lblLogNote;
  document.getElementById('logCopyBtn').textContent=t.logCopyBtn;
  document.getElementById('logClrBtn').textContent=t.logClrBtn;
  document.getElementById('iLblFile').textContent=t.lblFile;
  document.getElementById('uploadBtn').textContent=t.uploadBtn;
  ['speedProfile','profileMode'].forEach(function(id){
    Array.from(document.getElementById(id).options).forEach(function(o){
      if(o.dataset[lang])o.textContent=o.dataset[lang];
    });
  });
  var fn=document.getElementById('fileName');
  if(!fn.dataset.hasFile)fn.textContent=t.noFile;
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
    var fsdEl=document.getElementById('sFSD');
    var fsdActive=d.fsdTriggered&&!!d.fsdEnable;
    fsdEl.textContent=fsdActive?t.fsdYes:t.fsdNo;
    fsdEl.className=fsdActive?'status-yes':'status-no';
    document.getElementById('fsdEnable').checked=!!d.fsdEnable;
    document.getElementById('hwMode').value=d.hwMode;
    var hwAutoEl=document.getElementById('iHWAuto');
    if(d.hwDetected===1){hwAutoEl.style.color='#22c55e';hwAutoEl.textContent=T[lang].hwDetHW3;hwAutoEl.style.display='';}
    else if(d.hwDetected===2){hwAutoEl.style.color='#22c55e';hwAutoEl.textContent=T[lang].hwDetHW4;hwAutoEl.style.display='';}
    else if(d.rxCount>0){hwAutoEl.style.color='#f59e0b';hwAutoEl.textContent=T[lang].hwDetNone;hwAutoEl.style.display='';}
    else{hwAutoEl.style.display='none';}
    updateSpeedOptions(d.hwMode);
    document.getElementById('speedProfile').value=d.speedProfile;
    document.getElementById('profileMode').value=d.profileMode?'1':'0';
    document.getElementById('speedProfile').disabled=!!d.profileMode;
    document.getElementById('isaChime').checked=!!d.isaChime;
    document.getElementById('emergencyDet').checked=!!d.emergencyDet;
    document.getElementById('forceActivate').checked=!!d.forceActivate;
    var hw3OffEl=document.getElementById('hw3Offset');
    if(hw3OffEl)hw3OffEl.value=String(d.hw3Offset!=null?d.hw3Offset:-1);
    document.getElementById('precond').checked=!!d.precond;
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
    if(d.version)document.getElementById('sVer').textContent='v'+d.version;
    // AP auto-restart
    document.getElementById('hw3Cap').checked=!!d.hw3Cap;
    document.getElementById('apRestart').checked=!!d.apRestart;
    // High beam control
    var hbChk=document.getElementById('hbForce');
    var hbHint=document.getElementById('iHBHint');
    var adaptAvail=!!d.adaptLighting;
    hbChk.disabled=!adaptAvail;
    hbChk.checked=!!d.hbForce;
    hbHint.textContent=adaptAvail?T[lang].hbHintOn:T[lang].hbHintOff;
    hbHint.className=adaptAvail?'hb-hint avail':'hb-hint';
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
  }).catch(()=>{});
  logPollCount++;
  if(logPollCount%5===1)refreshDiagLog();
}
function setAPRestart(want){
  fetch('/api/aprestart?en='+(want?'1':'0')+(token?'&token='+token:''))
    .then(function(r){return r.json();})
    .then(function(res){if(res.ok)poll();});
}
function toggleHighBeam(want){
  fetch('/api/highbeam?en='+(want?'1':'0')+(token?'&token='+token:''))
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
// On load: pre-fetch status to get pinRequired, then decide flow
fetch('/api/status').then(function(r){return r.json();}).then(function(d){
  pinRequired=!!d.pinRequired;
  var disclaimed=false;
  try{disclaimed=localStorage.getItem('disclaimed')===FW_VER;}catch(e){}
  if(disclaimed){
    if(!pinRequired){startApp();}
    else if(token){startApp();}  // poll() will catch 403 if token expired
    else{showPinStep();}
  }
}).catch(function(){});
try{if(localStorage.getItem('disclaimed')===FW_VER){startApp();}}catch(e){}
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
  document.getElementById('rowHW3Cap').style.display=(hwMode===1)?'':'none';
  document.getElementById('rowHW3Offset').style.display=(hwMode===1)?'':'none';
}
function setVal(key,val){
  if(!agreed)return;
  var url='/api/set?'+key+'='+val+(token?'&token='+token:'');
  fetch(url).then(function(r){if(r.status===403){token='';try{sessionStorage.removeItem('fsd_tok');}catch(e){}showPinStep();}}).catch(function(){});
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
</script>
</body>
</html>
)rawliteral";
