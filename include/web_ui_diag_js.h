#pragma once
// ── Shared JS for the diagnostic-upload UI + carSwVer save ──────────────────
// Phone (web_ui.h) and car (web_ui_car.h) HTML embed this same JS via macro
// substitution inside their R"rawliteral(...)rawliteral" string. The two UIs
// previously carried identical copies of this state machine — making any tweak
// risky to keep in sync. Three adapters bridge the styling / i18n / variable
// differences:
//
//   _DIAG_LITERALS()  → object with the user-visible labels (T[lang] for phone,
//                        Chinese literals for car).
//   _DIAG_STYLE(el, kind)
//                      → apply 'ok' / 'err' / 'pending' styling. Phone toggles
//                        className "msg ok" etc; car sets inline style.color.
//   _AUTH_TOKEN()     → returns the session token string (phone uses `token`,
//                        car uses `tok`).
//
// Each HTML page must define those three functions in a <script> block before
// emitting DIAG_SHARED_JS.

#define DIAG_SHARED_JS R"rawjs(
// ───── carSwVer save ─────
var carVerDirty=false;
(function(){
  var inp=document.getElementById('carVerInput');
  if(!inp) return;
  // Tesla MCU 版本格式 YYYY.WW.PP[.SUB]（年.周.补丁[.子号]）。社区/媒体常省略 SUB。
  // 仅做软提示，不阻止保存：worker 端用允许字符集再过滤一次，不合规归 (invalid) 桶。
  var RE=/^\d{4}\.\d{1,2}\.\d{1,3}(\.\d{1,3})?$/;
  inp.addEventListener('input',function(){
    carVerDirty=true;
    var msg=document.getElementById('carVerMsg');
    if(!msg) return;
    var v=inp.value.trim();
    var L=_DIAG_LITERALS();
    if(v.length>0 && !RE.test(v)){
      if(L.carVerBadFmt){ msg.textContent=L.carVerBadFmt; _DIAG_STYLE(msg,'err'); }
    } else {
      msg.textContent=''; _DIAG_STYLE(msg,'');
    }
  });
})();
function saveCarVer(){
  var L=_DIAG_LITERALS();
  var inp=document.getElementById('carVerInput');
  var msg=document.getElementById('carVerMsg');
  if(!inp||!msg) return;
  var v=inp.value.trim();
  if(L.carVerSaving){ msg.textContent=L.carVerSaving; _DIAG_STYLE(msg,'pending'); }
  var t=_AUTH_TOKEN();
  fetch('/api/carver'+(t?'?token='+encodeURIComponent(t)+'&v=':'?v=')+encodeURIComponent(v))
    .then(function(r){
      if(r.ok){ msg.textContent=L.carVerOK; _DIAG_STYLE(msg,'ok'); carVerDirty=false; }
      else{ msg.textContent=L.carVerFail; _DIAG_STYLE(msg,'err'); }
      setTimeout(function(){ msg.textContent=''; _DIAG_STYLE(msg,''); },2500);
    })
    .catch(function(){
      msg.textContent=L.carVerNetErr||L.carVerFail; _DIAG_STYLE(msg,'err');
      setTimeout(function(){ msg.textContent=''; _DIAG_STYLE(msg,''); },2500);
    });
}
// ───── 诊断包上传 ─────
var __diagPoll=null;
function diagSetUI(state, id, msgText){
  var L=_DIAG_LITERALS();
  var btn=document.getElementById('diagUpBtn');
  var idEl=document.getElementById('diagUpId');
  var copy=document.getElementById('diagUpCopy');
  var msgEl=document.getElementById('diagUpMsg');
  if(!btn||!msgEl) return;
  btn.setAttribute('data-state', String(state));
  function setBtn(on){ btn.disabled=!on; btn.style.opacity=on?1:.5; }
  if(state===1){
    setBtn(false); btn.textContent=L.uploading;
    if(idEl) idEl.style.display='none';
    if(copy) copy.style.display='none';
    msgEl.textContent=L.inProg; _DIAG_STYLE(msgEl,'pending');
  } else if(state===2){
    setBtn(true); btn.textContent=L.retryAgain;
    if(idEl){ idEl.textContent=id; idEl.style.display=''; }
    if(copy) copy.style.display='';
    msgEl.textContent=L.done; _DIAG_STYLE(msgEl,'ok');
  } else if(state===3){
    setBtn(true); btn.textContent=L.retry;
    if(idEl) idEl.style.display='none';
    if(copy) copy.style.display='none';
    msgEl.textContent=msgText||L.reqFail; _DIAG_STYLE(msgEl,'err');
  } else {
    setBtn(true); btn.textContent=L.upload;
    if(idEl) idEl.style.display='none';
    if(copy) copy.style.display='none';
    msgEl.textContent=''; _DIAG_STYLE(msgEl,'');
  }
}
function diagPollOnce(){
  var t=_AUTH_TOKEN();
  fetch('/api/diag/status'+(t?'?token='+encodeURIComponent(t):''))
    .then(function(r){
      if(r.status===404){ diagSetUI(3,'', _DIAG_LITERALS().notSupported); return null; }
      // 403 = ESP32 local checkToken failed (PIN expired). Stop polling
      // silently; the next user interaction will surface the PIN prompt.
      if(r.status===403){ if(__diagPoll){ clearInterval(__diagPoll); __diagPoll=null; } return null; }
      return r.ok ? r.json() : null;
    })
    .then(function(d){
      if(!d) return;
      diagSetUI(d.state, d.id, d.msg);
      if(d.state===1){ if(!__diagPoll) __diagPoll=setInterval(diagPollOnce, 1500); }
      else { if(__diagPoll){ clearInterval(__diagPoll); __diagPoll=null; } }
    })
    .catch(function(){});
}
function diagUpload(){
  diagSetUI(1,'','');
  var L=_DIAG_LITERALS();
  var t=_AUTH_TOKEN();
  fetch('/api/diag/upload'+(t?'?token='+encodeURIComponent(t):''),{method:'POST'})
    .then(function(r){
      if(r.status===404){ diagSetUI(3,'', L.notSupported); return null; }
      // ESP32 local 403 = PIN required / expired. Show clear hint instead
      // of leaving UI stuck on "uploading…".
      if(r.status===403){ diagSetUI(3,'', L.unauthorized||'未授权（请先验证 PIN）'); return null; }
      if(!r.ok){ diagSetUI(3,'', 'HTTP '+r.status); return null; }
      return r.json();
    })
    .then(function(d){ if(d) diagPollOnce(); })
    .catch(function(){ diagSetUI(3,'', L.netErr); });
}
function diagCopyId(){
  var L=_DIAG_LITERALS();
  var idEl=document.getElementById('diagUpId');
  if(!idEl||!idEl.textContent) return;
  var v=idEl.textContent;
  if(navigator.clipboard&&navigator.clipboard.writeText){
    navigator.clipboard.writeText(v).then(function(){
      var msg=document.getElementById('diagUpMsg');
      if(msg){ msg.textContent=L.copied+v; _DIAG_STYLE(msg,'ok'); }
    });
  }
}
)rawjs"
