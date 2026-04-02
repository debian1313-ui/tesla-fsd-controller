#pragma once

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="zh">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1,user-scalable=no">
<title>FSD Controller</title>
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
</style>
</head>
<body>

<div class="title-wrap">
  <h1 id="iTitle">FSD 控制器</h1>
  <button class="lang-btn" id="iLangBtn" onclick="toggleLang()">EN</button>
</div>

<div class="card">
  <div class="card-title" id="iCardCtrl">控制</div>
  <div class="row">
    <span class="row-label" id="iLblFsdEn">FSD 开关</span>
    <label class="toggle"><input type="checkbox" id="fsdEnable" checked onchange="setVal('fsdEnable',this.checked?1:0)"><span class="slider"></span></label>
  </div>
  <div class="row" style="flex-wrap:wrap;gap:4px">
    <span class="row-label" id="iLblHW">硬件版本</span>
    <select id="hwMode" onchange="setVal('hwMode',this.value)">
      <option value="0">LEGACY</option>
      <option value="1">HW3</option>
      <option value="2" selected>HW4</option>
    </select>
    <div id="iHWHint" style="width:100%;font-size:11px;color:#64748b;padding:2px 0">HW4 硬件 + 固件 2026.8.x 或更旧（FSD V13）→ 请选 HW3</div>
  </div>
  <div class="row">
    <span class="row-label" id="iLblSpeed">速度模式</span>
    <select id="speedProfile" onchange="setVal('speedProfile',this.value)">
      <option value="0" data-zh="保守" data-en="Chill">保守</option>
      <option value="1" data-zh="默认" data-en="Default" selected>默认</option>
      <option value="2" data-zh="适中" data-en="Moderate">适中</option>
      <option value="3" data-zh="激进" data-en="Aggressive">激进</option>
      <option value="4" data-zh="最大" data-en="Maximum">最大</option>
    </select>
  </div>
  <div class="row">
    <span class="row-label" id="iLblPMode">模式来源</span>
    <select id="profileMode" onchange="setVal('profileMode',this.value)">
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
    <span class="row-label" id="iLblCN">中国模式 🇨🇳</span>
    <label class="toggle"><input type="checkbox" id="chinaMode" onchange="setVal('chinaMode',this.checked?1:0)"><span class="slider"></span></label>
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
  <div class="status-row"><span id="iLblCAN">CAN 总线</span><span id="sCAN" class="status-no">--</span></div>
  <div class="status-row"><span id="iLblFSDTrig">FSD 已触发</span><span id="sFSD" class="status-no">--</span></div>
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

<script>
var lang='zh';
var T={
  zh:{title:'FSD 控制器',cardCtrl:'控制',cardStat:'状态',cardOTA:'固件更新',
    lblFsdEn:'FSD 开关',lblHW:'硬件版本',lblSpeed:'速度模式',lblPMode:'模式来源',
    lblISA:'限速提示音抑制',lblEmg:'紧急车辆检测',lblCN:'中国模式 🇨🇳',
    lblMod:'已修改',lblRX:'已接收',lblErr:'错误',lblUp:'运行时间',
    lblCAN:'CAN 总线',lblFSDTrig:'FSD 已触发',
    lblFile:'选择文件',noFile:'未选择文件',uploadBtn:'上传固件',
    canOK:'正常',canErr:'异常',fsdYes:'是',fsdNo:'否',
    otaOK:'上传成功，正在重启...',otaFail:'上传失败: ',otaConn:'连接失败',
    uptH:'时',uptM:'分',uptS:'秒',langBtn:'EN',
    hwHint:'HW4 hardware + firmware 2026.8.x or older (FSD V13) → select HW3'},
  en:{title:'FSD Controller',cardCtrl:'CONTROL',cardStat:'STATUS',cardOTA:'OTA UPDATE',
    lblFsdEn:'FSD Enable',lblHW:'Hardware',lblSpeed:'Speed Profile',lblPMode:'Profile Source',
    lblISA:'ISA Chime Suppress',lblEmg:'Emergency Detection',lblCN:'China Mode 🇨🇳',
    lblMod:'MODIFIED',lblRX:'RECEIVED',lblErr:'ERRORS',lblUp:'UPTIME',
    lblCAN:'CAN Bus',lblFSDTrig:'FSD Triggered',
    lblFile:'Choose File',noFile:'No file chosen',uploadBtn:'Upload Firmware',
    canOK:'OK',canErr:'ERROR',fsdYes:'Yes',fsdNo:'No',
    otaOK:'Upload success, rebooting...',otaFail:'Upload failed: ',otaConn:'Connection error',
    uptH:'h',uptM:'m',uptS:'s',langBtn:'中文',
    hwHint:'HW4 hardware + firmware 2026.8.x or older (FSD V13) → select HW3'}
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
  document.getElementById('iLblMod').textContent=t.lblMod;
  document.getElementById('iLblRX').textContent=t.lblRX;
  document.getElementById('iLblErr').textContent=t.lblErr;
  document.getElementById('iLblUp').textContent=t.lblUp;
  document.getElementById('iLblCAN').textContent=t.lblCAN;
  document.getElementById('iLblFSDTrig').textContent=t.lblFSDTrig;
  document.getElementById('iHWHint').textContent=t.hwHint;
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
    fsdEl.textContent=d.fsdTriggered?t.fsdYes:t.fsdNo;
    fsdEl.className=d.fsdTriggered?'status-yes':'status-no';
    document.getElementById('fsdEnable').checked=!!d.fsdEnable;
    document.getElementById('hwMode').value=d.hwMode;
    document.getElementById('speedProfile').value=d.speedProfile;
    document.getElementById('profileMode').value=d.profileMode?'1':'0';
    document.getElementById('isaChime').checked=!!d.isaChime;
    document.getElementById('emergencyDet').checked=!!d.emergencyDet;
    document.getElementById('chinaMode').checked=!!d.chinaMode;
  }).catch(()=>{});
}
setInterval(poll,1000);poll();
function setVal(key,val){fetch('/api/set?'+key+'='+val).catch(()=>{});}
function fileChosen(inp){
  var fn=document.getElementById('fileName');
  if(inp.files[0]){fn.textContent=inp.files[0].name;fn.dataset.hasFile='1';}
  else{delete fn.dataset.hasFile;fn.textContent=T[lang].noFile;}
  document.getElementById('uploadBtn').disabled=!inp.files[0];
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
  xhr.open('POST','/api/ota');xhr.send(form);
}
</script>
</body>
</html>
)rawliteral";
