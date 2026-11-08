import {Trackball,settings,legacySettings} from './trackball.mjs';
const host=window.__arcadeHost;
if(!host||!document.documentElement.hasAttribute('data-backyard-arcade'))throw new Error('Arcade controls require the standalone Cornhole build.');
const {shotPowerColor}=await import('/yard-games/power-arrow.js');
let config;
try{
 const saved=localStorage.getItem('backyard-arcade-v5');
 const legacy=localStorage.getItem('backyard-arcade-v4')||localStorage.getItem('backyard-arcade-v3')||localStorage.getItem('backyard-arcade-v2')||localStorage.getItem('backyard-arcade-v1');
 config=saved?settings(JSON.parse(saved)):legacy?legacySettings(JSON.parse(legacy)):settings();
}catch{config=settings();}

const boardViewEnabled=true;
let tournament=false,placementTarget=null;
const machine=new Trackball(config),$=id=>document.getElementById(id);
let menuOpen=true,settingsOpen=false,navIndex=0,lastTurn='',lastShot=null,calibration=null,binding=null,lastFrame=performance.now(),fps=60;
const diag={x:0,y:0,events:0,lastX:0,lastY:0,locked:false,lastShot:null};
const root=document.createElement('div');root.id='arcade-ui';root.innerHTML=`
<div id="arcade-scoreboard" aria-label="Cornhole scoreboard"><div class="arcade-team arcade-red" id="arcade-red-team"><span id="arcade-red-name">RED</span><div class="arcade-score-values"><div><small id="arcade-red-total-label">TOTAL</small><strong id="arcade-red-score">0</strong></div><div class="arcade-round-metric" id="arcade-red-round-metric"><small>ROUND</small><strong id="arcade-red-round">0</strong></div></div><div id="arcade-red-pips" class="arcade-pips" aria-hidden="true"></div></div><div class="arcade-score-center"><span id="arcade-score-round">ROUND 1</span><b id="arcade-score-target">TO 21</b><small id="arcade-round-edge">EVEN</small></div><div class="arcade-team arcade-blue" id="arcade-blue-team"><span id="arcade-blue-name">BLUE</span><div class="arcade-score-values"><div><small id="arcade-blue-total-label">TOTAL</small><strong id="arcade-blue-score">0</strong></div><div class="arcade-round-metric" id="arcade-blue-round-metric"><small>ROUND</small><strong id="arcade-blue-round">0</strong></div></div><div id="arcade-blue-pips" class="arcade-pips" aria-hidden="true"></div></div></div>
<aside id="arcade-board-view" aria-label="Live board close-up" hidden><span>BOARD · 27 FT</span><div id="arcade-board-viewport"></div></aside>
<section id="arcade-menu" class="arcade-card"><p class="arcade-kicker">ARCADE · 0.2.14</p><h1>BACKYARD<br><span>CORNHOLE</span></h1><div class="arcade-actions"><button id="arcade-resume" hidden>RESUME</button><button id="arcade-two">2 PLAYER</button><button id="arcade-practice">PRACTICE</button><button id="arcade-tournament">TOURNAMENT</button><button id="arcade-audio-open" class="secondary">MUSIC</button><button id="arcade-config" class="secondary">CABINET SETUP</button><button id="arcade-exit" class="secondary">EXIT GAME</button></div></section>
<section id="arcade-audio" class="arcade-card" hidden><h2>MUSIC</h2><p id="music-now"></p><div class="arcade-actions"><button id="music-toggle">PLAY MUSIC</button><button id="music-next">NEXT TRACK</button><button id="music-folder" class="secondary">CHOOSE FOLDER</button><button id="music-refresh" class="secondary">REFRESH</button></div><div class="music-volume"><button data-music-volume="-.1" aria-label="Lower music volume">−</button><output id="music-volume"></output><button data-music-volume=".1" aria-label="Raise music volume">+</button></div><button id="arcade-audio-back" class="secondary">BACK</button></section>
<section id="arcade-settings" class="arcade-card" hidden><h2>CABINET SETUP</h2>
<div class="arcade-options">
<label>Sensitivity</label><div><button data-adjust="sensitivity" data-by="-.25">−</button><output id="sensitivity-value"></output><button data-adjust="sensitivity" data-by=".25">+</button></div>
<label>Full-power speed</label><div><button data-adjust="powerSpeed" data-by="-200">−</button><output id="range-value"></output><button data-adjust="powerSpeed" data-by="200">+</button></div>
<label>Stroke length</label><div><button data-adjust="releaseGate" data-by="-20">−</button><output id="gate-value"></output><button data-adjust="releaseGate" data-by="20">+</button></div>
<label>Finish rotation</label><div><button data-adjust="spin" data-by="-.15">−</button><output id="spin-value"></output><button data-adjust="spin" data-by=".15">+</button></div>
<label>Wind</label><div><button id="arcade-wind">CALM</button></div>
<label>Throw style</label><button data-bind="shotKey"></button><label>Bag surface</label><button data-bind="surfaceKey"></button><label>Reset bag</label><button data-bind="resetKey"></button><label>Confirm</label><button data-bind="confirmKey"></button><label>Pause / menu</label><button data-bind="menuKey"></button></div>
<div class="arcade-actions"><button id="arcade-calibrate">CALIBRATE TRACKBALL</button><button id="arcade-reset" class="secondary">RESET SETTINGS</button><button id="arcade-settings-back" class="secondary">BACK</button></div>
<pre id="arcade-diagnostics" aria-live="off"></pre></section>
<div id="arcade-power" hidden><span id="arcade-power-label">POWER</span><div role="meter" aria-label="Throw power" aria-valuemin="0" aria-valuemax="100" aria-valuenow="0" id="arcade-power-track"><i id="arcade-power-fill"></i></div></div>
<div id="arcade-prompt" hidden><span id="arcade-state">↓</span><span id="arcade-height">NORMAL</span><span id="arcade-surface">SLICK</span><button id="arcade-reset-bag">RESET BAG</button></div>
<div id="arcade-calibration" class="arcade-card" hidden><h2>↑</h2><p>Roll forward firmly.</p><button id="arcade-calibration-cancel" class="secondary">CANCEL</button></div>
<div id="arcade-error" class="arcade-card" hidden><h2>TRACKBALL NOT CAPTURED</h2><p id="arcade-error-copy"></p><button id="arcade-error-back">BACK</button></div>`;
document.body.append(root);
let windy=false,capturing=false,nativeInput=false,nativeStatus=null;
function save(){config=settings(config);machine.config=config;localStorage.setItem('backyard-arcade-v5',JSON.stringify(config));renderSettings();}
function renderSettings(){
 $('gate-value').textContent=Math.round(config.releaseGate);
 $('sensitivity-value').textContent=config.sensitivity.toFixed(2)+'×';$('range-value').textContent=Math.round(config.powerSpeed);$('spin-value').textContent=config.spin===0?'OFF':config.spin.toFixed(2);
 for(const b of root.querySelectorAll('[data-bind]'))b.textContent=binding===b.dataset.bind?'PRESS BUTTON…':config[b.dataset.bind];
 $('arcade-wind').textContent=windy?'WIND':'CALM';
}
function inMenu(){return menuOpen||tournament&&!$('panel').hidden;}
function buttons(){return [...document.querySelectorAll('#arcade-ui button,#panel button')].filter(b=>!b.disabled&&b.getClientRects().length);}
function focusFirst(){navIndex=0;buttons()[0]?.focus();}
function inputReady(){return nativeInput||!!document.pointerLockElement;}
function unlock(){if(document.pointerLockElement)document.exitPointerLock();}
function showMenu(){
 $('panel').hidden=true;placementTarget=null;tournament=false;
 menuOpen=true;settingsOpen=false;calibration=null;machine.reset(performance.now());host.pause(true);host.hideArrow();unlock();
 $('arcade-audio').hidden=true;$('arcade-menu').hidden=false;$('arcade-settings').hidden=true;$('arcade-calibration').hidden=true;$('arcade-error').hidden=true;
 $('arcade-resume').hidden=['menu','intro','champion'].includes(host.state.phase);focusFirst();
}
function showSettings(){settingsOpen=true;menuOpen=true;$('arcade-menu').hidden=true;$('arcade-settings').hidden=false;binding=null;renderSettings();focusFirst();}
async function capture(){
 if(nativeInput){diag.locked=true;machine.reset(performance.now());return true;}
 capturing=true;
 try{
  try{await document.getElementById('game').requestPointerLock({unadjustedMovement:true});}
  catch{await document.getElementById('game').requestPointerLock();}
  // Older Chromium returns void; wait for the asynchronous lock result.
  for(let i=0;i<20&&!document.pointerLockElement;i++)await new Promise(r=>setTimeout(r,25));
  if(!document.pointerLockElement)throw new Error('Select Resume to capture the trackball.');
  return true;
 }catch(error){showError(error.message);return false;}
 finally{capturing=false;}
}
function showError(message){host.pause(true);menuOpen=true;$('arcade-error-copy').textContent=message||'Select Resume to try again.';$('arcade-menu').hidden=true;$('arcade-settings').hidden=true;$('arcade-calibration').hidden=true;$('arcade-error').hidden=false;focusFirst();}
async function resume(){
 tournament=host.state.career;
 settingsOpen=false;menuOpen=false;root.querySelectorAll('.arcade-card').forEach(el=>el.hidden=true);host.pause(false);machine.reset(performance.now());await capture();
}
function start(mode,lock=true){tournament=false;host.start(mode);host.wind(windy);lastTurn='';lastShot=null;machine.reset(performance.now());if(lock)resume();else{menuOpen=false;settingsOpen=false;root.querySelectorAll('.arcade-card').forEach(el=>el.hidden=true);host.pause(false);}}
$('arcade-audio-open').onclick=()=>{$('arcade-menu').hidden=true;$('arcade-audio').hidden=false;focusFirst();};$('arcade-audio-back').onclick=showMenu;
$('arcade-tournament').onclick=()=>{tournament=true;menuOpen=false;settingsOpen=false;root.querySelectorAll('.arcade-card').forEach(el=>el.hidden=true);host.pause(false);host.tournament();focusFirst();};
$('panel').addEventListener('click',e=>{if(e.target.closest('#fight')&&!nativeInput)capture();});
$('arcade-two').onclick=()=>start('two');$('arcade-practice').onclick=()=>start('practice');$('arcade-resume').onclick=resume;
$('arcade-config').onclick=showSettings;$('arcade-settings-back').onclick=showMenu;$('arcade-error-back').onclick=showMenu;
$('arcade-exit').onclick=()=>window.close();$('pause-button').onclick=showMenu;
$('arcade-wind').onclick=()=>{windy=!windy;host.wind(windy);renderSettings();};
$('arcade-reset').onclick=()=>{config=settings();save();};
for(const b of root.querySelectorAll('[data-adjust]'))b.onclick=()=>{config[b.dataset.adjust]+=Number(b.dataset.by);save();};
for(const b of root.querySelectorAll('[data-bind]'))b.onclick=()=>{binding=b.dataset.bind;renderSettings();};
function resetBag(){if(menuOpen||host.state.phase!=='aim'||host.state.vs==='cpu'&&host.state.thrower===1)return;machine.reset(performance.now());placementTarget=0;host.resetBag();}
$('arcade-reset-bag').onclick=resetBag;
$('arcade-calibrate').onclick=()=>{
 calibration=new Trackball(config);calibration.reset(performance.now());settingsOpen=false;menuOpen=false;$('arcade-settings').hidden=true;$('arcade-calibration').hidden=false;capture();
};
$('arcade-calibration-cancel').onclick=()=>{calibration=null;unlock();showSettings();};
function feed(dx,dy,now=performance.now()){
 diag.x+=dx;diag.y+=dy;diag.lastX=dx;diag.lastY=dy;diag.events++;
 if(calibration){
  const stroke=calibration.feed(dx,dy,now);
  if(stroke?.kind==='throw'){
   config.powerSpeed=stroke.speed/.7;
   save();
   calibration=null;
   menuOpen=true;
   unlock();
   $('arcade-calibration').hidden=true;
   showSettings();
  }
  return;
 }
 if(menuOpen||host.state.paused||host.state.phase!=='aim')return;
 const wasForward=machine.phase==='forward';
 const result=machine.feed(dx,dy,now);
 if(wasForward&&machine.phase==='ready')host.hideArrow();
 if(!result)return;
 if(result.kind==='position'){if(placementTarget===null)placementTarget=host.state.position[host.state.thrower];placementTarget=Math.max(-.45,Math.min(.45,placementTarget+result.dx*.0008));}
 if(result.kind==='preview')host.preview(result);
 if(result.kind==='throw'){
  lastShot={...result,power:host.power(result.power),at:now};diag.lastShot=lastShot;
  if(!host.throw(result))machine.reset(now);
 }
}
document.addEventListener('mousemove',e=>{if(!nativeInput&&document.pointerLockElement)feed(e.movementX,e.movementY,performance.now());});
// Keep hover navigation usable with a shared trackball and a keyboard-encoded cabinet button.
for(const area of [root,$('panel')])area.addEventListener('pointermove',e=>{const b=e.target.closest('button');if(b&&b!==document.activeElement){b.focus({preventScroll:true});navIndex=buttons().indexOf(b);}});
document.addEventListener('pointerlockchange',()=>{
 diag.locked=inputReady();
 if(!diag.locked&&!menuOpen&&!capturing){if(calibration){calibration=null;$('arcade-calibration').hidden=true;showSettings();}else showMenu();}
 if(diag.locked)machine.reset(performance.now());
});
document.addEventListener('pointerlockerror',()=>{if(!nativeInput&&!capturing)showError('Select Resume to capture the trackball.');});
window.addEventListener('blur',()=>{if(!menuOpen)showMenu();});
window.addEventListener('keydown',e=>{
 if(e.altKey&&e.code==='F4')return;
 const handled=['F2','Digit1','Digit2','ArrowUp','ArrowDown','ArrowLeft','ArrowRight','Tab',config.shotKey,config.surfaceKey,config.menuKey,config.confirmKey,config.resetKey,'Escape','Space','Enter'];
 if(!handled.includes(e.code)&&!binding)return;e.preventDefault();e.stopImmediatePropagation();if(e.repeat)return;
 if(binding){config[binding]=e.code;binding=null;save();return;}
 if(e.code==='F2'){showMenu();showSettings();return;}
 if(!menuOpen&&tournament&&!$('panel').hidden&&[config.menuKey,config.surfaceKey,'Escape'].includes(e.code)){($('panel').querySelector('#back')||$('panel').querySelector('button:last-of-type'))?.click();return;}
 if(e.code===config.menuKey||e.code==='Escape'){if(calibration){calibration=null;unlock();$('arcade-calibration').hidden=true;showSettings();}else if(settingsOpen)showMenu();else if(menuOpen&&!$('arcade-resume').hidden)resume();else showMenu();return;}
 if(!menuOpen&&tournament&&!$('panel').hidden){const list=buttons();if(['ArrowUp','ArrowLeft','ArrowDown','ArrowRight','Tab'].includes(e.code)){navIndex=(navIndex+(['ArrowUp','ArrowLeft'].includes(e.code)?-1:1)+list.length)%list.length;list[navIndex]?.focus();}else if([config.confirmKey,config.shotKey,'Enter','Space'].includes(e.code)){(document.activeElement?.matches('#panel button')?document.activeElement:list[0])?.click();machine.reset(performance.now());}return;}
 if(menuOpen){
  if(e.code==='Digit1'){start('practice');return;}if(e.code==='Digit2'){start('two');return;}
  const list=buttons();if(['ArrowUp','ArrowLeft','ArrowDown','ArrowRight','Tab'].includes(e.code)){navIndex=(navIndex+(['ArrowUp','ArrowLeft'].includes(e.code)||e.shiftKey?-1:1)+list.length)%list.length;list[navIndex]?.focus();}
  else if([config.shotKey,config.confirmKey,'Space','Enter'].includes(e.code))(document.activeElement?.matches('#arcade-ui button')?document.activeElement:list[0])?.click();return;
 }
 if(['summary','over'].includes(host.state.phase)){if([config.shotKey,config.confirmKey,'Space','Enter'].includes(e.code)){host.advance();machine.reset(performance.now());}return;}
 if(e.code===config.resetKey){resetBag();return;}
 if(host.state.phase==='aim'&&[config.shotKey,config.surfaceKey].includes(e.code)){if(e.code===config.shotKey)host.height();else host.surface();host.hideArrow();placementTarget=host.state.position[host.state.thrower];machine.reset(performance.now());}
},true);
const padState=new Map(),padArmAt=performance.now()+700;
function pollPads(){
 if(nativeStatus?.pads.length)return;
 let pads;try{pads=navigator.getGamepads?.()||[];}catch{return;}
 const connected=new Set();
 for(const pad of pads){if(!pad)continue;for(let i=0;i<pad.buttons.length;i++){
  const code=`Pad${pad.index}Button${i}`,pressed=pad.buttons[i].pressed;connected.add(code);
  // A held launch button must be released before it can select anything.
  if(pressed&&padState.get(code)===false&&performance.now()>=padArmAt){
   let action=code;
   if(!binding&&![config.shotKey,config.surfaceKey,config.menuKey,config.confirmKey,config.resetKey].includes(code)){
    action=({0:inMenu()?config.confirmKey:config.shotKey,1:inMenu()?config.menuKey:config.surfaceKey,2:config.resetKey,5:config.resetKey,8:config.menuKey,9:inMenu()?config.confirmKey:config.menuKey,12:'ArrowUp',13:'ArrowDown',14:'ArrowLeft',15:'ArrowRight'})[i];
   }
   if(action)window.dispatchEvent(new KeyboardEvent('keydown',{code:action,bubbles:true}));
  }
  padState.set(code,pressed);
 }}
 for(const key of padState.keys())if(!connected.has(key))padState.delete(key);
}
function nativeEvent(data){
 if(data.kind==='status'){
  nativeStatus=data;nativeInput=data.mice.length>0;diag.locked=inputReady();
  console.info('Cabinet devices',JSON.stringify(data));
  if(!nativeInput&&!menuOpen)showMenu();
  return;
 }
 if(data.kind==='error'){nativeInput=false;diag.locked=false;showMenu();return;}
 if(data.kind==='reset'){machine.reset(performance.now());return;}
 if(data.kind==='motion'&&nativeInput){feed(data.x,data.y,Number.isFinite(data.at)?Math.min(performance.now(),data.at-performance.timeOrigin):performance.now());return;}
 // Golden Tee accepts trackball mouse buttons as well as the RetroPad pair.
 // Keep ordinary left-click menu selection; in play use the pair for bag options.
 if(data.kind==='mouse-button'&&data.pressed){
  if(calibration||binding)return;
  if(data.button===0&&menuOpen)return;
  const code=data.button===0?config.shotKey:(inMenu()?config.menuKey:config.surfaceKey);
  window.dispatchEvent(new KeyboardEvent('keydown',{code,bubbles:true}));return;
 }
 if(data.kind==='nav'&&(menuOpen||tournament&&!$('panel').hidden)){window.dispatchEvent(new KeyboardEvent('keydown',{code:data.code,bubbles:true}));return;}
 if(data.kind!=='button'||!data.pressed)return;
 const code=`Pad${data.pad}Button${data.button}`;
 let action=code;
 if(!binding&&![config.shotKey,config.surfaceKey,config.menuKey,config.confirmKey,config.resetKey].includes(code)){
  action=({0:inMenu()?config.confirmKey:config.shotKey,3:inMenu()?config.confirmKey:config.shotKey,1:inMenu()?config.menuKey:config.surfaceKey,4:inMenu()?config.menuKey:config.surfaceKey,2:config.resetKey,5:config.resetKey,8:config.menuKey,9:inMenu()?config.confirmKey:config.menuKey})[data.button];
 }
 if(action)window.dispatchEvent(new KeyboardEvent('keydown',{code:action,bubbles:true}));
}
for(const type of ['pointerdown','pointermove','pointerup'])window.addEventListener(type,e=>{if(nativeInput&&!menuOpen&&!e.target.closest('#arcade-ui,#panel')){e.preventDefault();e.stopImmediatePropagation();}},true);
window.addEventListener('click',e=>{if(nativeInput&&!menuOpen&&!e.target.closest('#arcade-ui,#panel')){e.preventDefault();e.stopImmediatePropagation();}},true);
document.addEventListener('contextmenu',e=>{if(nativeInput)e.preventDefault();});
window.cabinetInput?.subscribe(nativeEvent);
let liveRound={pts:[0,0]};
function renderScoreboard(s){
 // Keep settled totals visible through a throw; recount every bag after all motion ends.
 if(!s.bags.length)liveRound={pts:[0,0]};
 else if(s.bags.every(b=>['board','in','grass','removed'].includes(b.state)&&!(b.state==='grass'&&b.dead)))liveRound=host.round();
 const round=liveRound;
 $('arcade-blue-name').textContent=s.career?s.rival:'BLUE';
 for(const [t,name] of ['red','blue'].entries()){
  const thrown=s.bags.filter(b=>b.team===t).length;
  const score=s.practice?round.pts[t]:s.scores[t];
  $(`arcade-${name}-score`).textContent=score;
  $(`arcade-${name}-round`).textContent=round.pts[t];
  $(`arcade-${name}-round-metric`).hidden=s.practice;
  $(`arcade-${name}-total-label`).textContent=s.practice?'POINTS':'TOTAL';
  $(`arcade-${name}-team`).classList.toggle('current',!menuOpen&&s.phase==='aim'&&s.thrower===t);
  $(`arcade-${name}-team`).classList.toggle('inactive',s.practice&&t===1);
  const pips=s.practice?'':Array.from({length:4},(_,i)=>`<i class="${i<thrown?'used':''}"></i>`).join('');
  if($(`arcade-${name}-pips`).innerHTML!==pips)$(`arcade-${name}-pips`).innerHTML=pips;
 }
 $('arcade-score-round').textContent=s.practice?'PRACTICE':`ROUND ${s.inning}`;
 $('arcade-score-target').textContent=s.practice?'POINTS':s.phase==='over'?'FINAL':'TO 21';
 const difference=round.pts[0]-round.pts[1],edge=$('arcade-round-edge');
 edge.hidden=s.practice||['menu','intro','champion'].includes(s.phase);
 edge.textContent=difference===0?'EVEN':`${difference>0?'RED':'BLUE'} +${Math.abs(difference)}`;
 edge.dataset.team=difference>0?'red':difference<0?'blue':'even';
}
function frame(now){
 pollPads();
 const frameDt=Math.min(.1,(now-lastFrame)/1000);fps=fps*.95+1000/Math.max(1,now-lastFrame)*.05;lastFrame=now;
 const s=host.state;renderScoreboard(s);const key=`${s.phase}:${s.inning}:${s.k}:${s.thrower}`;
 if(key!==lastTurn){lastTurn=key;machine.reset(now);placementTarget=null;if(s.phase==='wait'&&!s.career)host.wind(windy);}
 if(!menuOpen&&inputReady()){
  const wasForward=machine.phase==='forward';
  machine.tick(now);
  if(wasForward&&machine.phase!=='forward')host.hideArrow();
 }
 if(!menuOpen&&s.phase==='aim'&&placementTarget!==null){const delta=(placementTarget-s.position[s.thrower])*(1-Math.exp(-frameDt/0.07));if(Math.abs(delta)>1e-6)host.position(delta);}
 const live=['forward'].includes(machine.phase)&&s.phase==='aim'&&machine.preview;const power=live?host.power(machine.preview.power):(lastShot?.power??0);$('arcade-power').hidden=menuOpen||!live&&(!lastShot||now-lastShot.at>5000);$('arcade-power-label').textContent=live?'POWER':'LAST SHOT';$('arcade-power-fill').style.width=(power*100)+'%';$('arcade-power-fill').style.background=shotPowerColor(power);$('arcade-power-track').setAttribute('aria-valuenow',Math.round(power*100));
 document.body.classList.toggle('arcade-native-playing',nativeInput&&!menuOpen);
 $('arcade-board-view').hidden=!boardViewEnabled||menuOpen||!!calibration||['menu','intro','champion'].includes(s.phase);
 $('arcade-prompt').hidden=menuOpen||s.phase!=='aim'||!!calibration;
 $('arcade-state').textContent=machine.phase==='settling'?'·':machine.phase==='ready'?'↑':'↗';
 $('arcade-height').textContent=['SLIDER','NORMAL','KOBE'][s.arc];
 $('arcade-surface').textContent=s.side[s.thrower].toUpperCase();
 if(settingsOpen)$('arcade-diagnostics').textContent=`${nativeStatus?`TRACKBALL: ${nativeStatus.mice.join(", ")||"NOT FOUND"} · ${nativeStatus.pads.length} controllers\n`:""}${Math.round(fps)} FPS · ${diag.events} events\nΔ ${diag.lastX}, ${diag.lastY} · total ${Math.round(diag.x)}, ${Math.round(diag.y)}\n${lastShot?`power ${lastShot.power.toFixed(3)} · aim ${lastShot.aim.toFixed(3)} · spin ${lastShot.spin.toFixed(2)}`:'No throw recorded'}\nF2 setup · 1 practice · 2 players · Alt+F4 exit`;
 // Existing score/round panels remain functional; only replace the main lobby.
 const panel=$('panel');if(s.phase==='menu'&&!tournament){panel.hidden=true;if(!menuOpen)showMenu();}
 requestAnimationFrame(frame);
}
renderSettings();showMenu();requestAnimationFrame(frame);
const {setupMusic}=await import('./music-player.mjs');const music=setupMusic(root);
window.__arcade={music,start,showMenu,showSettings,feed,get config(){return {...config};},setConfig(value){config=settings({...config,...value});save();machine.reset(performance.now());},get diagnostics(){return {...diag};},resetBag,get machine(){return machine;}};
