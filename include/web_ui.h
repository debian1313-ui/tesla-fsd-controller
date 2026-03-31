#pragma once

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="zh">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1,user-scalable=no">
<title>FSD 控制器</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:-apple-system,system-ui,"PingFang SC","Microsoft YaHei",sans-serif;background:#0b1120;color:#e2e8f0;min-height:100vh;padding:16px}
h1{text-align:center;font-size:22px;color:#38bdf8;padding:20px 0 24px;font-weight:700;letter-spacing:1px}
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

<h1>FSD 控制器</h1>

<div class="card">
  <div class="card-title">控制</div>
  <div class="row">
    <span class="row-label">FSD 开关</span>
    <label class="toggle"><input type="checkbox" id="fsdEnable" checked onchange="setVal('fsdEnable',this.checked?1:0)"><span class="slider"></span></label>
  </div>
  <div class="row">
    <span class="row-label">硬件版本</span>
    <select id="hwMode" onchange="setVal('hwMode',this.value)">
      <option value="0">LEGACY</option>
      <option value="1">HW3</option>
      <option value="2" selected>HW4</option>
    </select>
  </div>
  <div class="row">
    <span class="row-label">速度模式</span>
    <select id="speedProfile" onchange="setVal('speedProfile',this.value)">
      <option value="0">保守</option>
      <option value="1" selected>默认</option>
      <option value="2">适中</option>
      <option value="3">激进</option>
      <option value="4">最大</option>
    </select>
  </div>
  <div class="row">
    <span class="row-label">模式来源</span>
    <select id="profileMode" onchange="setVal('profileMode',this.value)">
      <option value="1" selected>自动（拨杆）</option>
      <option value="0">手动</option>
    </select>
  </div>
  <div class="row">
    <span class="row-label">限速提示音抑制</span>
    <label class="toggle"><input type="checkbox" id="isaChime" onchange="setVal('isaChime',this.checked?1:0)"><span class="slider"></span></label>
  </div>
  <div class="row">
    <span class="row-label">紧急车辆检测</span>
    <label class="toggle"><input type="checkbox" id="emergencyDet" checked onchange="setVal('emergencyDet',this.checked?1:0)"><span class="slider"></span></label>
  </div>
  <div class="row">
    <span class="row-label">中国模式 🇨🇳</span>
    <label class="toggle"><input type="checkbox" id="chinaMode" onchange="setVal('chinaMode',this.checked?1:0)"><span class="slider"></span></label>
  </div>
</div>

<div class="card">
  <div class="card-title">状态</div>
  <div class="stats">
    <div class="stat"><div class="stat-val green" id="sModified">0</div><div class="stat-label">已修改</div></div>
    <div class="stat"><div class="stat-val" id="sRX">0</div><div class="stat-label">已接收</div></div>
    <div class="stat"><div class="stat-val amber" id="sErrors">0</div><div class="stat-label">错误</div></div>
    <div class="stat"><div class="stat-val" id="sUptime">0秒</div><div class="stat-label">运行时间</div></div>
  </div>
  <div class="status-row"><span>CAN 总线</span><span id="sCAN" class="status-no">--</span></div>
  <div class="status-row"><span>FSD 已触发</span><span id="sFSD" class="status-no">--</span></div>
</div>

<div class="card">
  <div class="card-title">固件更新</div>
  <div class="ota-row">
    <label class="file-btn" for="fwFile">选择文件</label>
    <input type="file" id="fwFile" accept=".bin" style="display:none" onchange="fileChosen(this)">
    <span class="file-name" id="fileName">未选择文件</span>
  </div>
  <button class="upload-btn" id="uploadBtn" disabled onclick="doOTA()">上传固件</button>
  <div class="progress" id="progWrap"><div class="progress-bar" id="progBar"></div></div>
  <div class="msg" id="otaMsg"></div>
</div>

<script>
function poll(){
  fetch('/api/status').then(r=>r.json()).then(d=>{
    document.getElementById('sModified').textContent=d.modified;
    document.getElementById('sRX').textContent=d.rx;
    document.getElementById('sErrors').textContent=d.errors;
    let u=d.uptime;
    let h=Math.floor(u/3600),m=Math.floor((u%3600)/60),s=u%60;
    document.getElementById('sUptime').textContent=h>0?h+'时'+m+'分':m>0?m+'分'+s+'秒':s+'秒';
    let canEl=document.getElementById('sCAN');
    canEl.textContent=d.canOK?'正常':'异常';
    canEl.className=d.canOK?'status-ok':'status-err';
    let fsdEl=document.getElementById('sFSD');
    fsdEl.textContent=d.fsdTriggered?'是':'否';
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
setInterval(poll,1000);
poll();
function setVal(key,val){fetch('/api/set?'+key+'='+val).catch(()=>{});}
function fileChosen(inp){
  document.getElementById('fileName').textContent=inp.files[0]?inp.files[0].name:'未选择文件';
  document.getElementById('uploadBtn').disabled=!inp.files[0];
}
function doOTA(){
  let file=document.getElementById('fwFile').files[0];
  if(!file)return;
  let xhr=new XMLHttpRequest();
  let prog=document.getElementById('progWrap');
  let bar=document.getElementById('progBar');
  let msg=document.getElementById('otaMsg');
  prog.style.display='block';bar.style.width='0%';msg.textContent='';msg.className='msg';
  document.getElementById('uploadBtn').disabled=true;
  xhr.upload.addEventListener('progress',e=>{if(e.lengthComputable)bar.style.width=Math.round(e.loaded/e.total*100)+'%';});
  xhr.onload=function(){
    if(xhr.status===200){msg.textContent='上传成功，正在重启...';msg.className='msg ok';}
    else{msg.textContent='上传失败: '+xhr.statusText;msg.className='msg err';document.getElementById('uploadBtn').disabled=false;}
  };
  xhr.onerror=function(){msg.textContent='连接失败';msg.className='msg err';document.getElementById('uploadBtn').disabled=false;};
  let form=new FormData();form.append('firmware',file);
  xhr.open('POST','/api/ota');xhr.send(form);
}
</script>
</body>
</html>
)rawliteral";
