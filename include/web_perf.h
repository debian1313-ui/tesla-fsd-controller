#pragma once

const char PERF_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="zh">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1,user-scalable=no">
<title>性能测试</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:-apple-system,system-ui,"PingFang SC","Microsoft YaHei",sans-serif;background:#0b1120;color:#e2e8f0;min-height:100vh;padding:16px}
.header{display:flex;align-items:center;gap:12px;margin-bottom:20px}
.back-btn{background:#1e293b;color:#94a3b8;border:1px solid #334155;border-radius:10px;padding:8px 14px;font-size:13px;font-weight:600;cursor:pointer;transition:border-color .2s,color .2s}
.back-btn:hover{border-color:#38bdf8;color:#38bdf8}
.header h1{font-size:18px;font-weight:700;color:#38bdf8;letter-spacing:1px}

/* ── Speed display ── */
.speed-card{background:#131d32;border-radius:16px;padding:28px 20px 20px;margin-bottom:16px;text-align:center;position:relative;overflow:hidden}
.speed-glow{position:absolute;top:50%;left:50%;transform:translate(-50%,-50%);width:200px;height:200px;border-radius:50%;opacity:.08;filter:blur(40px);transition:background 0.5s}
.speed-val{font-size:88px;font-weight:900;line-height:1;letter-spacing:-3px;position:relative;transition:color 0.3s}
.speed-label{font-size:13px;color:#64748b;margin-top:6px;letter-spacing:3px;position:relative}
.speed-blue{color:#38bdf8}
.speed-amber{color:#f59e0b}
.speed-red{color:#ef4444}

/* ── Test card ── */
.card{background:#131d32;border-radius:14px;padding:20px;margin-bottom:16px;border:1px solid transparent;transition:border-color .4s,box-shadow .4s}
.card.flash-done{border-color:#22c55e;box-shadow:0 0 20px #22c55e33}
.test-header{display:flex;align-items:center;justify-content:space-between;margin-bottom:14px}
.test-title{font-size:16px;font-weight:700;color:#e2e8f0}

/* ── State badge ── */
.badge{font-size:11px;font-weight:700;padding:4px 10px;border-radius:20px;letter-spacing:1px;transition:all .3s}
.badge-idle{background:#1e293b;color:#64748b}
.badge-armed{background:#3d2f00;color:#f59e0b}
.badge-running{background:#0d2d1a;color:#22c55e}
.badge-done{background:#0d2d4f;color:#38bdf8}
@keyframes blink{0%,100%{opacity:1}50%{opacity:.4}}
.badge-running{animation:blink .7s ease-in-out infinite}

/* ── Progress bar ── */
.progress-wrap{height:4px;background:#1e293b;border-radius:2px;margin-bottom:18px;overflow:hidden}
.progress-bar{height:100%;border-radius:2px;width:0%;transition:width .15s linear,background .5s}
.bar-accel{background:linear-gradient(90deg,#38bdf8,#22c55e)}
.bar-brake{background:linear-gradient(90deg,#f59e0b,#ef4444)}

/* ── Result ── */
.result-row{display:flex;align-items:baseline;gap:8px;margin-bottom:18px;min-height:52px}
.result-val{font-size:48px;font-weight:800;color:#f8fafc;transition:color .3s}
.result-unit{font-size:15px;color:#64748b;font-weight:500}
.entry-tag{font-size:11px;color:#64748b;background:#1e293b;border-radius:6px;padding:3px 8px;margin-left:4px;align-self:center}

/* ── Buttons ── */
.btn-row{display:flex;gap:10px}
.btn-arm{flex:1;background:#c41530;color:#fff;border:none;border-radius:10px;padding:14px;font-size:15px;font-weight:700;cursor:pointer;letter-spacing:1px;transition:background .2s,transform .1s,opacity .2s;position:relative;overflow:hidden}
.btn-arm:not(:disabled):active{transform:scale(.97)}
.btn-arm:hover:not(:disabled){background:#e31937}
.btn-arm:disabled{opacity:.35;cursor:not-allowed}
.btn-reset{flex:0 0 76px;background:#1e293b;color:#94a3b8;border:1px solid #334155;border-radius:10px;padding:14px;font-size:13px;font-weight:600;cursor:pointer;transition:border-color .2s,color .2s}
.btn-reset:hover{border-color:#64748b;color:#e2e8f0}
.btn-share{flex:0 0 76px;background:#1e293b;color:#38bdf8;border:1px solid #1e3a5f;border-radius:10px;padding:14px;font-size:13px;font-weight:600;cursor:pointer;transition:border-color .2s,color .2s,background .2s;display:none}
.btn-share:hover{background:#0d2d4f;border-color:#38bdf8}
.btn-share.show{display:block}

/* ── Vehicle (model) row ── */
.vrow{display:flex;align-items:center;gap:10px;background:#131d32;border-radius:12px;padding:12px 14px;margin-bottom:14px;border:1px solid transparent;transition:border-color .2s}
.vrow.dirty{border-color:#f59e0b}
.vrow-label{font-size:12px;color:#64748b;letter-spacing:1px;flex:0 0 auto}
.vrow-input{flex:1;background:#0b1120;color:#e2e8f0;border:1px solid #334155;border-radius:8px;padding:8px 10px;font-size:14px;font-family:inherit;letter-spacing:.5px;outline:none;transition:border-color .2s}
.vrow-input:focus{border-color:#38bdf8}
.vrow-input::placeholder{color:#475569}
.vrow-save{background:#0d2d4f;color:#38bdf8;border:1px solid #1e3a5f;border-radius:8px;padding:8px 12px;font-size:12px;font-weight:600;cursor:pointer;display:none;letter-spacing:.5px}
.vrow-save:hover{background:#0f3a66}
.vrow-save.show{display:block}
.vrow-save.saved{color:#22c55e;border-color:#22c55e}
.vrow-save.failed{color:#ef4444;border-color:#ef4444}

/* ── Ripple ── */
@keyframes ripple{to{transform:scale(4);opacity:0}}
.ripple{position:absolute;border-radius:50%;background:rgba(255,255,255,.25);transform:scale(0);animation:ripple .5s linear;pointer-events:none}

.note{text-align:center;font-size:11px;color:#334155;padding:8px 0 4px}

/* ── Disclaimer overlay ── */
#perfDisclaimer{position:fixed;top:0;left:0;right:0;bottom:0;background:rgba(0,0,0,.92);z-index:999;display:flex;align-items:center;justify-content:center;padding:20px}
.disc-box{background:#131d32;border-radius:16px;padding:24px;max-width:360px;width:100%;border:1px solid #f59e0b}
.disc-icon{font-size:32px;text-align:center;margin-bottom:12px}
.disc-title{color:#f59e0b;font-weight:700;font-size:16px;margin-bottom:14px;text-align:center}
.disc-body{font-size:13px;color:#94a3b8;line-height:1.9;margin-bottom:22px}
.disc-body b{color:#e2e8f0}
.disc-confirm{background:#f59e0b;color:#0b1120;border:none;border-radius:10px;padding:14px;width:100%;font-size:14px;font-weight:700;cursor:pointer;margin-bottom:10px;letter-spacing:1px}
.disc-confirm:hover{background:#d97706}
.disc-cancel{background:#1e293b;color:#64748b;border:1px solid #334155;border-radius:10px;padding:12px;width:100%;font-size:13px;cursor:pointer}
.disc-cancel:hover{color:#94a3b8;border-color:#475569}

/* ── History ── */
.hist-section{margin-bottom:16px}
.hist-header{display:flex;align-items:center;justify-content:space-between;margin-bottom:8px;padding:0 2px}
.hist-title{font-size:12px;color:#475569;font-weight:600;letter-spacing:1px}
.hist-clear-btn{font-size:11px;color:#334155;cursor:pointer;border:none;background:none;padding:2px 6px}
.hist-clear-btn:hover{color:#64748b}
.hist-row{display:flex;align-items:center;justify-content:space-between;padding:9px 14px;background:#131d32;border-radius:10px;margin-bottom:6px;border-left:3px solid transparent}
.hist-accel{border-left-color:#38bdf8}
.hist-brake{border-left-color:#f59e0b}
.hist-ts{font-size:11px;color:#475569}
.hist-val{font-size:18px;font-weight:800;color:#f8fafc}
.hist-sec{font-size:12px;color:#64748b;margin-left:3px}
.hist-entry{font-size:11px;color:#64748b;background:#1e293b;border-radius:4px;padding:2px 7px;margin-left:8px}
.hist-empty{font-size:12px;color:#334155;text-align:center;padding:12px}
</style>
</head>
<body>

<!-- Disclaimer overlay -->
<div id="perfDisclaimer">
  <div class="disc-box">
    <div class="disc-icon">⚠️</div>
    <div class="disc-title">使用前须知</div>
    <div class="disc-body">
      · 本测试<b>仅限封闭道路或专用场地</b>使用<br>
      · <b>严禁在公共道路、交通区域进行</b><br>
      · 测试结果受路面、坡度、温度等影响，<b>数据仅供参考</b><br>
      · 请确保测试区域安全，无行人及障碍物<br>
      · 所有风险由使用者自行承担
    </div>
    <button class="disc-confirm" onclick="confirmPerf()">我已了解，在封闭场地使用</button>
    <button class="disc-cancel" onclick="history.back()">返回</button>
  </div>
</div>

<div class="header">
  <button class="back-btn" onclick="history.back()">← 返回</button>
  <h1>性能测试</h1>
</div>

<!-- Speed display -->
<div class="speed-card">
  <div class="speed-glow" id="speedGlow"></div>
  <div class="speed-val speed-blue" id="speed">--</div>
  <div class="speed-label">km/h</div>
</div>

<!-- Vehicle (model) row -->
<div class="vrow" id="vrowModel">
  <span class="vrow-label">车型</span>
  <input class="vrow-input" id="perfModelInput" placeholder="例如 Model Y Performance" maxlength="32" autocomplete="off">
  <button class="vrow-save" id="perfModelSave" onclick="saveModel()">保存</button>
</div>

<!-- 0→100 card -->
<div class="card" id="cardAccel">
  <div class="test-header">
    <div class="test-title">0 → 100 km/h</div>
    <span class="badge badge-idle" id="accelBadge">待命</span>
  </div>
  <div class="progress-wrap"><div class="progress-bar bar-accel" id="accelBar"></div></div>
  <div class="result-row">
    <span class="result-val" id="accelResult">--</span>
    <span class="result-unit" id="accelUnit"></span>
  </div>
  <div class="btn-row">
    <button class="btn-arm" id="btnArm0" onclick="arm('accel',this)">预备</button>
    <button class="btn-reset" onclick="resetTest('accel')">重置</button>
    <button class="btn-share" id="btnShareAccel" onclick="shareResult('accel')">分享</button>
  </div>
</div>

<!-- 100→0 card -->
<div class="card" id="cardBrake">
  <div class="test-header">
    <div class="test-title">100 → 0 km/h</div>
    <span class="badge badge-idle" id="brakeBadge">待命</span>
  </div>
  <div class="progress-wrap"><div class="progress-bar bar-brake" id="brakeBar"></div></div>
  <div class="result-row">
    <span class="result-val" id="brakeResult">--</span>
    <span class="result-unit" id="brakeUnit"></span>
    <span class="entry-tag" id="brakeEntry" style="display:none"></span>
  </div>
  <div class="btn-row">
    <button class="btn-arm" id="btnArm1" onclick="arm('brake',this)">预备</button>
    <button class="btn-reset" onclick="resetTest('brake')">重置</button>
    <button class="btn-share" id="btnShareBrake" onclick="shareResult('brake')">分享</button>
  </div>
</div>

<div class="note">精度约 ±0.1 秒（受 CAN 帧率限制）· 仅供参考 · 请在安全场地使用</div>

<!-- Accel history -->
<div class="hist-section">
  <div class="hist-header">
    <span class="hist-title">0→100 历史记录</span>
    <button class="hist-clear-btn" onclick="clearHistory('accel')">清除</button>
  </div>
  <div id="histAccelList"></div>
</div>

<!-- Brake history -->
<div class="hist-section">
  <div class="hist-header">
    <span class="hist-title">100→0 历史记录</span>
    <button class="hist-clear-btn" onclick="clearHistory('brake')">清除</button>
  </div>
  <div id="histBrakeList"></div>
</div>

<script>
var token='';
try{token=sessionStorage.getItem('fsd_tok')||'';}catch(e){}

// ── Disclaimer ───────────────────────────────────────────────────────────────
function confirmPerf(){
  document.getElementById('perfDisclaimer').style.display='none';
}

var BADGE=['badge-idle','badge-armed','badge-running','badge-done'];
var BADGE_TEXT=['待命','已备战','计时中','完成'];
var prevAccel=0,prevBrake=0;
var accelAnimDone=false,brakeAnimDone=false;
var brakeEntryKph=0;
// Latest perfAccelMs/perfBrakeMs from poll — used by share button (NOT previous-frame state).
var lastAccelMs=0,lastBrakeMs=0,fwVer='';
var perfModelDirty=false;

// ── Count-up animation ──────────────────────────────────────────────────────
function countUp(elId,targetSec,duration){
  var el=document.getElementById(elId);
  var start=performance.now();
  function step(now){
    var p=Math.min((now-start)/duration,1);
    var e=1-Math.pow(1-p,3);  // ease-out cubic
    el.textContent=(targetSec*e).toFixed(2);
    if(p<1)requestAnimationFrame(step);
    else el.textContent=targetSec.toFixed(2);
  }
  requestAnimationFrame(step);
}

// ── Card flash on completion ────────────────────────────────────────────────
function flashCard(id){
  var c=document.getElementById(id);
  c.classList.add('flash-done');
  setTimeout(function(){c.classList.remove('flash-done');},1500);
}

// ── Ripple effect on ARM button ──────────────────────────────────────────────
function addRipple(btn,e){
  var r=document.createElement('span');
  var rect=btn.getBoundingClientRect();
  var size=Math.max(rect.width,rect.height);
  r.className='ripple';
  r.style.width=r.style.height=size+'px';
  r.style.left=(e.clientX-rect.left-size/2)+'px';
  r.style.top=(e.clientY-rect.top-size/2)+'px';
  btn.appendChild(r);
  setTimeout(function(){r.remove();},500);
}

// ── Apply state to a test card ───────────────────────────────────────────────
function applyState(prefix,state,ms,spd){
  var badge=document.getElementById(prefix+'Badge');
  var res=document.getElementById(prefix+'Result');
  var unit=document.getElementById(prefix+'Unit');
  var bar=document.getElementById(prefix+'Bar');
  var armBtn=document.getElementById('btnArm'+(prefix==='accel'?'0':'1'));

  badge.className='badge '+BADGE[state]||BADGE[0];
  badge.textContent=BADGE_TEXT[state]||'--';

  // Progress bar
  if(state===2){
    var pct=0;
    if(prefix==='accel') pct=Math.min(spd,100);
    else pct=Math.min(100,Math.max(0,brakeEntryKph>0?(1-spd/brakeEntryKph)*100:0));
    bar.style.width=pct+'%';
  } else if(state===3){
    bar.style.width='100%';
  } else {
    bar.style.width='0%';
  }

  // ARM button
  armBtn.disabled=(state===2||state===3);

  // Share button — visible only when DONE with a real time
  var shareBtn=document.getElementById('btnShare'+(prefix==='accel'?'Accel':'Brake'));
  if(shareBtn){
    if(state===3&&ms>0)shareBtn.classList.add('show');
    else shareBtn.classList.remove('show');
  }

  // Result
  if(state===3&&ms>0){
    unit.textContent='秒';
    // Animate only once per result
    var flag=(prefix==='accel')?accelAnimDone:brakeAnimDone;
    if(!flag){
      countUp(prefix+'Result',ms/1000,900);
      flashCard('card'+(prefix==='accel'?'Accel':'Brake'));
      if(prefix==='accel')accelAnimDone=true;
      else{
        brakeAnimDone=true;
        // Show entry speed for braking test
        var et=document.getElementById('brakeEntry');
        if(brakeEntryKph>0){et.textContent='进入 '+brakeEntryKph+' km/h';et.style.display='';}
      }
    }
  } else if(state===2){
    res.textContent='...';
    unit.textContent='';
  } else if(state!==3){
    res.textContent='--';
    unit.textContent='';
    if(prefix==='accel')accelAnimDone=false;
    else{brakeAnimDone=false;document.getElementById('brakeEntry').style.display='none';}
  }
}

// ── Speed colour ─────────────────────────────────────────────────────────────
function updateSpeed(spd){
  var el=document.getElementById('speed');
  var glow=document.getElementById('speedGlow');
  el.textContent=spd||'--';
  if(!spd){el.className='speed-val speed-blue';glow.style.background='#38bdf8';return;}
  if(spd<60){el.className='speed-val speed-blue';glow.style.background='#38bdf8';}
  else if(spd<100){el.className='speed-val speed-amber';glow.style.background='#f59e0b';}
  else{el.className='speed-val speed-red';glow.style.background='#ef4444';}
}

// ── History ───────────────────────────────────────────────────────────────────
var HIST_MAX=10;
function saveHistory(type,ms,entryKph){
  var key='perf_hist_'+type;
  var arr=[];
  try{arr=JSON.parse(localStorage.getItem(key)||'[]');}catch(e){}
  arr.unshift({ts:Date.now(),ms:ms,entry:entryKph||0});
  if(arr.length>HIST_MAX)arr=arr.slice(0,HIST_MAX);
  try{localStorage.setItem(key,JSON.stringify(arr));}catch(e){}
  renderHistory(type);
}
function clearHistory(type){
  try{localStorage.removeItem('perf_hist_'+type);}catch(e){}
  renderHistory(type);
}
function fmtTs(ts){
  var d=new Date(ts);
  var p=function(n){return n<10?'0'+n:''+n;};
  return p(d.getMonth()+1)+'/'+p(d.getDate())+' '+p(d.getHours())+':'+p(d.getMinutes())+':'+p(d.getSeconds());
}
function renderHistory(type){
  var listEl=document.getElementById('hist'+(type==='accel'?'Accel':'Brake')+'List');
  var arr=[];
  try{arr=JSON.parse(localStorage.getItem('perf_hist_'+type)||'[]');}catch(e){}
  if(arr.length===0){listEl.innerHTML='<div class="hist-empty">暂无记录</div>';return;}
  var html='';
  for(var i=0;i<arr.length;i++){
    var r=arr[i];
    var cls=type==='accel'?'hist-accel':'hist-brake';
    html+='<div class="hist-row '+cls+'">';
    html+='<span class="hist-ts">'+fmtTs(r.ts)+'</span>';
    html+='<span><span class="hist-val">'+(r.ms/1000).toFixed(2)+'</span><span class="hist-sec">秒</span>';
    if(type==='brake'&&r.entry>0)html+='<span class="hist-entry">进入 '+r.entry+' km/h</span>';
    html+='</span></div>';
  }
  listEl.innerHTML=html;
}

// ── Poll ─────────────────────────────────────────────────────────────────────
function poll(){
  fetch('/api/status'+(token?'?token='+token:'')).then(function(r){return r.json();}).then(function(d){
    var spd=d.speedD?Math.round(d.speedD/10):0;
    updateSpeed(spd);
    brakeEntryKph=d.brakeEntryKph||0;
    if(d.version)fwVer=d.version;
    lastAccelMs=d.perfAccelMs||0;
    lastBrakeMs=d.perfBrakeMs||0;

    // Sync model input (skip if user is editing)
    var inp=document.getElementById('perfModelInput');
    if(inp&&!perfModelDirty&&typeof d.perfModel==='string'&&inp.value!==d.perfModel){
      inp.value=d.perfModel;
    }

    var na=d.perfAccel||0,nb=d.perfBrake||0;
    // Detect DONE transition → save to history
    if(prevAccel!==3&&na===3&&d.perfAccelMs>0)saveHistory('accel',d.perfAccelMs,0);
    if(prevBrake!==3&&nb===3&&d.perfBrakeMs>0)saveHistory('brake',d.perfBrakeMs,d.brakeEntryKph||0);
    applyState('accel',na,d.perfAccelMs||0,spd);
    applyState('brake',nb,d.perfBrakeMs||0,spd);
    prevAccel=na;
    prevBrake=nb;
  }).catch(function(){});
}

// ── API calls ────────────────────────────────────────────────────────────────
function arm(which,btn){
  addRipple(btn,event);
  fetch('/api/perf?cmd=arm_'+which+(token?'&token='+token:'')).then(function(r){return r.json();}).then(poll);
}
function resetTest(which){
  if(which==='accel')accelAnimDone=false;
  else{brakeAnimDone=false;document.getElementById('brakeEntry').style.display='none';}
  fetch('/api/perf?cmd=reset_'+which+(token?'&token='+token:'')).then(function(r){return r.json();}).then(poll);
}

// ── Model save ───────────────────────────────────────────────────────────────
function saveModel(){
  var inp=document.getElementById('perfModelInput');
  var btn=document.getElementById('perfModelSave');
  var row=document.getElementById('vrowModel');
  var v=(inp.value||'').trim();
  var url='/api/perf?cmd=set_model&v='+encodeURIComponent(v)+(token?'&token='+token:'');
  function onFail(){
    btn.classList.add('failed');
    btn.textContent='保存失败';
    setTimeout(function(){
      btn.classList.remove('failed');
      btn.textContent='保存';
    },1500);
  }
  fetch(url).then(function(r){
    if(!r.ok)throw new Error('http '+r.status);
    return r.json();
  }).then(function(){
    perfModelDirty=false;
    row.classList.remove('dirty');
    btn.classList.add('saved');
    btn.textContent='已保存';
    setTimeout(function(){
      btn.classList.remove('saved');
      btn.classList.remove('show');
      btn.textContent='保存';
    },1200);
  }).catch(onFail);
}

(function(){
  var inp=document.getElementById('perfModelInput');
  var btn=document.getElementById('perfModelSave');
  var row=document.getElementById('vrowModel');
  if(!inp)return;
  inp.addEventListener('input',function(){
    perfModelDirty=true;
    row.classList.add('dirty');
    btn.classList.add('show');
    btn.classList.remove('saved');
    btn.textContent='保存';
  });
  inp.addEventListener('keydown',function(e){
    if(e.key==='Enter'){e.preventDefault();saveModel();}
  });
})();

// ── Share ────────────────────────────────────────────────────────────────────
function shareResult(type){
  var ms=type==='accel'?lastAccelMs:lastBrakeMs;
  if(!ms||ms<=0)return;
  var inp=document.getElementById('perfModelInput');
  var model=(inp&&inp.value)?inp.value.trim():'';
  var url='/perf-share?type='+type
    +'&ms='+ms
    +'&model='+encodeURIComponent(model)
    +'&ver='+encodeURIComponent(fwVer||'')
    +'&t='+Date.now();
  if(type==='brake'&&brakeEntryKph>0)url+='&v='+brakeEntryKph;
  window.open(url,'_blank');
}

renderHistory('accel');
renderHistory('brake');
var __perfPollT=setInterval(poll,400);
document.addEventListener('visibilitychange',function(){
  if(document.hidden){
    if(__perfPollT){ clearInterval(__perfPollT); __perfPollT=null; }
  } else if(!__perfPollT){
    poll();
    __perfPollT=setInterval(poll,400);
  }
});
poll();
</script>
</body>
</html>
)rawliteral";
