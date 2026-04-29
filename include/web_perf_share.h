#pragma once

const char PERF_SHARE_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="zh">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1,user-scalable=no">
<title>性能成绩</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
html,body{width:100%;height:100%}
body{font-family:-apple-system,"PingFang SC","Microsoft YaHei",system-ui,sans-serif;background:#000;color:#fff;overflow:hidden;display:flex;flex-direction:column;align-items:center;justify-content:center;padding:24px 0}
.stage{position:relative;width:min(96vw,calc((100vh - 140px) * 0.5625));aspect-ratio:9/16;border-radius:28px;overflow:hidden;box-shadow:0 30px 70px rgba(0,0,0,.55),0 0 0 1px rgba(255,255,255,.04)}
.stage svg{display:block;width:100%;height:100%}
.actions{margin-top:22px;display:flex;gap:12px;justify-content:center}
.btn{background:#fff;color:#000;border:0;border-radius:999px;padding:13px 26px;font-size:14px;font-weight:600;cursor:pointer;font-family:inherit;-webkit-tap-highlight-color:transparent;letter-spacing:.5px}
.btn:active{transform:scale(.96)}
.btn.ghost{background:rgba(255,255,255,.08);color:#fff;border:1px solid rgba(255,255,255,.18)}
.toast{position:fixed;top:24px;left:50%;transform:translateX(-50%);background:rgba(0,0,0,.88);color:#fff;padding:10px 22px;border-radius:999px;font-size:13px;opacity:0;transition:opacity .3s;pointer-events:none;z-index:100;border:1px solid rgba(255,255,255,.12)}
.toast.show{opacity:1}
</style>
</head>
<body>

<div class="stage">
  <svg id="cardSvg" viewBox="0 0 1080 1920" preserveAspectRatio="xMidYMid meet" xmlns="http://www.w3.org/2000/svg">
    <defs>
      <radialGradient id="bgAccel" cx="50%" cy="20%" r="78%">
        <stop offset="0%" stop-color="#dc2626" stop-opacity=".88"/>
        <stop offset="42%" stop-color="#7f1d1d" stop-opacity=".55"/>
        <stop offset="100%" stop-color="#050505"/>
      </radialGradient>
      <radialGradient id="bgBrake" cx="50%" cy="20%" r="78%">
        <stop offset="0%" stop-color="#06b6d4" stop-opacity=".82"/>
        <stop offset="42%" stop-color="#0e7490" stop-opacity=".5"/>
        <stop offset="100%" stop-color="#050505"/>
      </radialGradient>
      <linearGradient id="numAccel" x1="0%" y1="0%" x2="0%" y2="100%">
        <stop offset="0%" stop-color="#fff7ed"/>
        <stop offset="100%" stop-color="#fdba74"/>
      </linearGradient>
      <linearGradient id="numBrake" x1="0%" y1="0%" x2="0%" y2="100%">
        <stop offset="0%" stop-color="#f0f9ff"/>
        <stop offset="100%" stop-color="#7dd3fc"/>
      </linearGradient>

      <!-- Speed-line decoration: stylized motion graphic, not a literal car silhouette.
           Three thin horizontal lines of decreasing length — minimalist racing-poster aesthetic. -->
      <symbol id="speed-lines" viewBox="0 0 500 80">
        <line x1="20" y1="20" x2="480" y2="20" stroke="currentColor" stroke-width="2" stroke-linecap="round" opacity="0.35"/>
        <line x1="100" y1="40" x2="400" y2="40" stroke="currentColor" stroke-width="3" stroke-linecap="round" opacity="0.55"/>
        <line x1="180" y1="60" x2="320" y2="60" stroke="currentColor" stroke-width="2" stroke-linecap="round" opacity="0.35"/>
      </symbol>
    </defs>

    <rect id="bgRect" width="1080" height="1920" fill="url(#bgAccel)"/>

    <!-- Top brand block -->
    <text x="540" y="220" text-anchor="middle" fill="white" font-size="60" letter-spacing="28" font-weight="200" font-family="-apple-system,system-ui,sans-serif">TESLA</text>
    <line x1="430" y1="270" x2="650" y2="270" stroke="rgba(255,255,255,.35)" stroke-width="1.5"/>
    <text x="540" y="316" text-anchor="middle" fill="rgba(255,255,255,.55)" font-size="22" letter-spacing="10" font-weight="500" font-family="-apple-system,system-ui,sans-serif">PERFORMANCE TIMER</text>

    <!-- Decorative dial circle (faint, behind hero) -->
    <circle cx="540" cy="1080" r="430" fill="none" stroke="rgba(255,255,255,.05)" stroke-width="1"/>
    <circle cx="540" cy="1080" r="370" fill="none" stroke="rgba(255,255,255,.04)" stroke-width="1"/>

    <!-- Hero number + unit (single text, tspans share baseline → naturally centered) -->
    <text x="540" y="1140" text-anchor="middle" font-family="-apple-system,system-ui,sans-serif"><tspan id="heroNum" font-size="360" font-weight="800" fill="url(#numAccel)" letter-spacing="-18">4.32</tspan><tspan dx="24" font-size="80" font-weight="300" fill="rgba(255,255,255,.7)" letter-spacing="4">秒</tspan></text>

    <!-- Accent tick under number -->
    <rect x="500" y="1200" width="80" height="3" rx="1.5" fill="rgba(255,255,255,.5)"/>

    <!-- Hero subtitle -->
    <text id="heroSub" x="540" y="1300" text-anchor="middle" fill="white" font-size="48" font-weight="400" letter-spacing="10" font-family="-apple-system,system-ui,sans-serif">0  →  100 km/h</text>

    <!-- Badge group rendered by JS -->
    <g id="badgeGroup"></g>

    <!-- Speed-line decoration -->
    <use href="#speed-lines" x="290" y="1560" width="500" height="80" color="rgba(255,255,255,0.9)"/>

    <!-- Bottom decoration -->
    <line x1="380" y1="1740" x2="700" y2="1740" stroke="rgba(255,255,255,.18)" stroke-width="1"/>

    <!-- Footer -->
    <text id="footTime" x="540" y="1798" text-anchor="middle" fill="rgba(255,255,255,.5)" font-size="28" font-family="-apple-system,system-ui,sans-serif">2026.04.26  ·  14:23</text>
    <text id="footMark" x="540" y="1858" text-anchor="middle" fill="rgba(255,255,255,.32)" font-size="22" letter-spacing="6" font-weight="500" font-family="-apple-system,system-ui,sans-serif">TESLA-CAN-WEB</text>
  </svg>
</div>

<div class="actions">
  <button class="btn" onclick="saveCard()">📥 保存图片</button>
  <button class="btn ghost" onclick="shareCard()">🔗 分享</button>
</div>

<div class="toast" id="toast"></div>

<script>
function q(name,def){
  var m=location.search.match(new RegExp('[?&]'+name+'=([^&]*)'));
  return m?decodeURIComponent(m[1].replace(/\+/g,' ')):def;
}
var type=q('type','accel');
var ms=parseInt(q('ms','0'),10);
var entry=parseInt(q('v','0'),10);
var ts=parseInt(q('t',String(Date.now())),10);
var model=q('model','');
var ver=q('ver','');

(function init(){
  if(type==='brake'){
    document.getElementById('bgRect').setAttribute('fill','url(#bgBrake)');
    document.getElementById('heroNum').setAttribute('fill','url(#numBrake)');
  }
  if(ms>0){
    document.getElementById('heroNum').textContent=(ms/1000).toFixed(2);
  }else{
    var hn=document.getElementById('heroNum');
    hn.textContent='--';
    hn.setAttribute('font-size','220');
  }
  document.getElementById('heroSub').textContent=(type==='accel')?'0  →  100 km/h':'100  →  0 km/h';
  if(ver) document.getElementById('footMark').textContent='TESLA-CAN-WEB  '+ver.toUpperCase();

  var labels=[];
  if(model) labels.push(model.toUpperCase());
  if(type==='brake'&&entry>0) labels.push('进入 '+entry+' km/h');
  renderBadges(labels);

  var d=new Date(ts);
  var pad=function(n){return n<10?'0'+n:''+n;};
  document.getElementById('footTime').textContent=
    d.getFullYear()+'.'+pad(d.getMonth()+1)+'.'+pad(d.getDate())+'  ·  '+
    pad(d.getHours())+':'+pad(d.getMinutes());
})();

function approxTextWidth(s,fontSize){
  var w=0;
  for(var i=0;i<s.length;i++){
    var c=s.charCodeAt(i);
    if(c>0x7f) w+=fontSize*1.0;
    else if(c>=48&&c<=57) w+=fontSize*.6;
    else if(c>=65&&c<=90) w+=fontSize*.72;
    else if(c===32) w+=fontSize*.32;
    else w+=fontSize*.55;
  }
  return w;
}

function renderBadges(labels){
  var NS='http://www.w3.org/2000/svg';
  var g=document.getElementById('badgeGroup');
  while(g.firstChild) g.removeChild(g.firstChild);
  if(labels.length===0) return;

  var fs=30, padH=34, padV=18, gap=18;
  var rectH=fs+padV*2;
  var widths=[], total=0;
  for(var i=0;i<labels.length;i++){
    var w=approxTextWidth(labels[i],fs)+padH*2;
    widths.push(w); total+=w;
  }
  total+=gap*(labels.length-1);

  var x=540-total/2;
  var y=1400;
  for(var i=0;i<labels.length;i++){
    var w=widths[i];
    var rect=document.createElementNS(NS,'rect');
    rect.setAttribute('x',x);
    rect.setAttribute('y',y);
    rect.setAttribute('width',w);
    rect.setAttribute('height',rectH);
    rect.setAttribute('rx',rectH/2);
    rect.setAttribute('fill','rgba(255,255,255,.13)');
    rect.setAttribute('stroke','rgba(255,255,255,.38)');
    rect.setAttribute('stroke-width','1.5');
    g.appendChild(rect);
    var t=document.createElementNS(NS,'text');
    t.setAttribute('x',x+w/2);
    t.setAttribute('y',y+rectH/2+fs*.36);
    t.setAttribute('text-anchor','middle');
    t.setAttribute('fill','rgba(255,255,255,.92)');
    t.setAttribute('font-size',fs);
    t.setAttribute('font-weight','500');
    t.setAttribute('font-family','-apple-system,system-ui,sans-serif');
    t.setAttribute('letter-spacing','2');
    t.textContent=labels[i];
    g.appendChild(t);
    x+=w+gap;
  }
}

function saveCard(){
  var svg=document.getElementById('cardSvg');
  var s=new XMLSerializer().serializeToString(svg);
  if(!/xmlns=/.test(s)) s=s.replace(/<svg /,'<svg xmlns="http://www.w3.org/2000/svg" ');
  var blob=new Blob([s],{type:'image/svg+xml;charset=utf-8'});
  var url=URL.createObjectURL(blob);
  var img=new Image();
  img.onload=function(){
    var c=document.createElement('canvas');
    c.width=1080; c.height=1920;
    var ctx=c.getContext('2d');
    ctx.fillStyle='#050505'; ctx.fillRect(0,0,1080,1920);
    ctx.drawImage(img,0,0,1080,1920);
    URL.revokeObjectURL(url);
    try{
      c.toBlob(function(b){
        if(!b){ toast('生成失败，长按卡片保存'); return; }
        var u=URL.createObjectURL(b);
        var a=document.createElement('a');
        a.href=u;
        a.download='tesla-perf-'+Date.now()+'.png';
        document.body.appendChild(a);
        a.click();
        a.remove();
        setTimeout(function(){ URL.revokeObjectURL(u); toast('已保存到下载'); },200);
      },'image/png');
    }catch(e){ toast('生成失败，长按卡片保存'); }
  };
  img.onerror=function(){ toast('生成失败，长按卡片保存'); };
  img.src=url;
}

function shareCard(){
  var sec=(ms/1000).toFixed(2);
  var head=(type==='accel'?'0 → 100':'100 → 0');
  if(navigator.share){
    navigator.share({title:'特斯拉性能成绩',text:head+' 用时 '+sec+' 秒',url:location.href}).catch(function(){});
  }else if(navigator.clipboard){
    navigator.clipboard.writeText(location.href).then(function(){toast('链接已复制');},function(){toast('请手动复制地址栏');});
  }else{
    toast('请手动复制地址栏链接');
  }
}

function toast(msg){
  var t=document.getElementById('toast');
  t.textContent=msg;
  t.classList.add('show');
  setTimeout(function(){ t.classList.remove('show'); },1800);
}
</script>
</body>
</html>
)rawliteral";
