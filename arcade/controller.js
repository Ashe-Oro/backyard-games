import {Trackball,settings} from './trackball.mjs';
const host=window.__arcadeHost;
let config;try{const saved=localStorage.getItem('backyard-arcade-v2');config=settings(saved?JSON.parse(saved):{...JSON.parse(localStorage.getItem('backyard-arcade-v1')||'{}'),spin:0});}catch{config=settings();}
const machine=new Trackball(config),$=id=>document.getElementById(id);
let menuOpen=true,settingsOpen=false,navIndex=0,lastTurn='',lastShot=null,calibration=null,binding=null,lastFrame=performance.now(),fps=60;
const diag={x:0,y:0,events:0,lastX:0,lastY:0,locked:false,lastShot:null};
const root=document.createElement('div');root.id='arcade-ui';root.innerHTML=`
<section id="arcade-menu" class="arcade-card"><p class="arcade-kicker">TRACKBALL PROTOTYPE · 0.2.2</p><h1>BACKYARD<br><span>CORNHOLE</span></h1><div class="arcade-actions"><button id="arcade-resume" hidden>RESUME</button><button id="arcade-two">2 PLAYER</button><button id="arcade-practice">PRACTICE</button><button id="arcade-config" class="secondary">CABINET SETUP</button><button id="arcade-exit" class="secondary">EXIT GAME</button></div></section>
<section id="arcade-settings" class="arcade-card" hidden><h2>CABINET SETUP</h2>
<div class="arcade-options"><label>Power</label><div><button data-power="hybrid">STROKE</button><button data-power="distance">DISTANCE</button></div>
<label>Sensitivity</label><div><button data-adjust="sensitivity" data-by="-.25">−</button><output id="sensitivity-value"></output><button data-adjust="sensitivity" data-by=".25">+</button></div>
<label>Pull range</label><div><button data-adjust="pullRange" data-by="-40">−</button><output id="range-value"></output><button data-adjust="pullRange" data-by="40">+</button></div>
<label>Finish rotation</label><div><button data-adjust="spin" data-by="-.15">−</button><output id="spin-value"></output><button data-adjust="spin" data-by=".15">+</button></div>
<label>Wind</label><div><button id="arcade-wind">CALM</button></div>
<label>Throw style</label><button data-bind="shotKey"></button><label>Bag surface</label><button data-bind="surfaceKey"></button><label>Confirm</label><button data-bind="confirmKey"></button><label>Pause / menu</label><button data-bind="menuKey"></button></div>
<div class="arcade-actions"><button id="arcade-calibrate">CALIBRATE TRACKBALL</button><button id="arcade-reset" class="secondary">RESET SETTINGS</button><button id="arcade-settings-back" class="secondary">BACK</button></div>
<pre id="arcade-diagnostics" aria-live="off"></pre></section>
<div id="arcade-prompt" hidden><span id="arcade-state">↓</span><span id="arcade-height">NORMAL</span><span id="arcade-surface">SLICK</span></div>
<div id="arcade-calibration" class="arcade-card" hidden><h2>↓ &nbsp; ↑</h2><p>Roll back, then forward.</p><button id="arcade-calibration-cancel" class="secondary">CANCEL</button></div>
<div id="arcade-error" class="arcade-card" hidden><h2>TRACKBALL NOT CAPTURED</h2><p id="arcade-error-copy"></p><button id="arcade-error-back">BACK</button></div>`;
document.body.append(root);
let windy=false,capturing=false,nativeInput=false,nativeStatus=null;
function save(){config=settings(config);machine.config=config;localStorage.setItem('backyard-arcade-v2',JSON.stringify(config));renderSettings();}
function renderSettings(){
 $('sensitivity-value').textContent=config.sensitivity.toFixed(2)+'×';$('range-value').textContent=Math.round(config.pullRange);$('spin-value').textContent=config.spin===0?'OFF':config.spin.toFixed(2);
 for(const b of root.querySelectorAll('[data-power]'))b.setAttribute('aria-pressed',String(b.dataset.power===config.mode));
 for(const b of root.querySelectorAll('[data-bind]'))b.textContent=binding===b.dataset.bind?'PRESS BUTTON…':config[b.dataset.bind];
 $('arcade-wind').textContent=windy?'WIND':'CALM';
}
function buttons(){return [...root.querySelectorAll('button')].filter(b=>!b.disabled&&b.getClientRects().length);}
function focusFirst(){navIndex=0;buttons()[0]?.focus();}
function inputReady(){return nativeInput||!!document.pointerLockElement;}
function unlock(){if(document.pointerLockElement)document.exitPointerLock();}
function showMenu(){
 menuOpen=true;settingsOpen=false;calibration=null;machine.reset(performance.now());host.pause(true);host.hideArrow();unlock();
 $('arcade-menu').hidden=false;$('arcade-settings').hidden=true;$('arcade-calibration').hidden=true;$('arcade-error').hidden=true;
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
 settingsOpen=false;menuOpen=false;root.querySelectorAll('.arcade-card').forEach(el=>el.hidden=true);host.pause(false);machine.reset(performance.now());await capture();
}
function start(mode,lock=true){host.start(mode);host.wind(windy);lastTurn='';lastShot=null;machine.reset(performance.now());if(lock)resume();else{menuOpen=false;settingsOpen=false;root.querySelectorAll('.arcade-card').forEach(el=>el.hidden=true);host.pause(false);}}
$('arcade-two').onclick=()=>start('two');$('arcade-practice').onclick=()=>start('practice');$('arcade-resume').onclick=resume;
$('arcade-config').onclick=showSettings;$('arcade-settings-back').onclick=showMenu;$('arcade-error-back').onclick=showMenu;
$('arcade-exit').onclick=()=>window.close();$('pause-button').onclick=showMenu;
$('arcade-wind').onclick=()=>{windy=!windy;host.wind(windy);renderSettings();};
$('arcade-reset').onclick=()=>{config=settings();save();};
for(const b of root.querySelectorAll('[data-power]'))b.onclick=()=>{config.mode=b.dataset.power;save();};
for(const b of root.querySelectorAll('[data-adjust]'))b.onclick=()=>{config[b.dataset.adjust]+=Number(b.dataset.by);save();};
for(const b of root.querySelectorAll('[data-bind]'))b.onclick=()=>{binding=b.dataset.bind;renderSettings();};
$('arcade-calibrate').onclick=()=>{
 calibration={back:0,forward:0};settingsOpen=false;menuOpen=false;$('arcade-settings').hidden=true;$('arcade-calibration').hidden=false;capture();
};
$('arcade-calibration-cancel').onclick=()=>{calibration=null;unlock();showSettings();};
function feed(dx,dy,now=performance.now()){
 diag.x+=dx;diag.y+=dy;diag.lastX=dx;diag.lastY=dy;diag.events++;
 if(calibration){
  if(dy>0&&calibration.forward===0)calibration.back+=dy;
  if(dy<0&&calibration.back>=25)calibration.forward-=dy;
  if(calibration.forward>=48){config.pullRange=calibration.back*config.sensitivity/.7;save();calibration=null;menuOpen=true;unlock();$('arcade-calibration').hidden=true;showSettings();}
  return;
 }
 if(menuOpen||host.state.paused||host.state.phase!=='aim')return;
 const result=machine.feed(dx,dy,now);if(!result)return;
 if(result.kind==='position')host.position(result.dx*.0015);
 if(result.kind==='preview'||result.kind==='backswing')host.preview(machine.preview||{power:0,aim:0,spin:0});
 if(result.kind==='throw'){
  lastShot={...result,mode:config.mode,at:now};diag.lastShot=lastShot;
  if(!host.throw(result))machine.reset(now);
 }
}
document.addEventListener('mousemove',e=>{if(!nativeInput&&document.pointerLockElement)feed(e.movementX,e.movementY,performance.now());});
// Keep hover navigation usable with a shared trackball and a keyboard-encoded cabinet button.
root.addEventListener('pointermove',e=>{const b=e.target.closest('button');if(b&&b!==document.activeElement){b.focus({preventScroll:true});navIndex=buttons().indexOf(b);}});
document.addEventListener('pointerlockchange',()=>{
 diag.locked=inputReady();
 if(!diag.locked&&!menuOpen&&!capturing){if(calibration){calibration=null;$('arcade-calibration').hidden=true;showSettings();}else showMenu();}
 if(diag.locked)machine.reset(performance.now());
});
document.addEventListener('pointerlockerror',()=>{if(!nativeInput&&!capturing)showError('Select Resume to capture the trackball.');});
window.addEventListener('blur',()=>{if(!menuOpen)showMenu();});
window.addEventListener('keydown',e=>{
 if(e.altKey&&e.code==='F4')return;
 const handled=['F2','Digit1','Digit2','ArrowUp','ArrowDown','ArrowLeft','ArrowRight','Tab',config.shotKey,config.surfaceKey,config.menuKey,config.confirmKey,'Escape','Space','Enter'];
 if(!handled.includes(e.code)&&!binding)return;e.preventDefault();e.stopImmediatePropagation();if(e.repeat)return;
 if(binding){config[binding]=e.code;binding=null;save();return;}
 if(e.code==='F2'){showMenu();showSettings();return;}
 if(e.code===config.menuKey||e.code==='Escape'){if(calibration){calibration=null;unlock();$('arcade-calibration').hidden=true;showSettings();}else if(settingsOpen)showMenu();else if(menuOpen&&!$('arcade-resume').hidden)resume();else showMenu();return;}
 if(menuOpen){
  if(e.code==='Digit1'){start('practice');return;}if(e.code==='Digit2'){start('two');return;}
  const list=buttons();if(['ArrowUp','ArrowLeft','ArrowDown','ArrowRight','Tab'].includes(e.code)){navIndex=(navIndex+(['ArrowUp','ArrowLeft'].includes(e.code)||e.shiftKey?-1:1)+list.length)%list.length;list[navIndex]?.focus();}
  else if([config.shotKey,config.confirmKey,'Space','Enter'].includes(e.code))(document.activeElement?.matches('#arcade-ui button')?document.activeElement:list[0])?.click();return;
 }
 if(['summary','over'].includes(host.state.phase)){if([config.shotKey,config.confirmKey,'Space','Enter'].includes(e.code)){host.advance();machine.reset(performance.now());}return;}
 if(host.state.phase==='aim'&&[config.shotKey,config.surfaceKey].includes(e.code)){if(e.code===config.shotKey)host.height();else host.surface();host.hideArrow();machine.reset(performance.now());}
},true);
const padState=new Map();
function pollPads(){
 if(nativeStatus?.pads.length)return;
 let pads;try{pads=navigator.getGamepads?.()||[];}catch{return;}
 const connected=new Set();
 for(const pad of pads){if(!pad)continue;for(let i=0;i<pad.buttons.length;i++){
  const code=`Pad${pad.index}Button${i}`,pressed=pad.buttons[i].pressed;connected.add(code);
  if(pressed&&!padState.get(code)){
   let action=code;
   if(!binding&&![config.shotKey,config.surfaceKey,config.menuKey,config.confirmKey].includes(code)){
    action=({0:menuOpen?config.confirmKey:config.shotKey,1:menuOpen?config.menuKey:config.surfaceKey,8:config.menuKey,9:menuOpen?'Digit2':config.menuKey,12:'ArrowUp',13:'ArrowDown',14:'ArrowLeft',15:'ArrowRight'})[i];
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
 if(data.kind==='nav'&&menuOpen){window.dispatchEvent(new KeyboardEvent('keydown',{code:data.code,bubbles:true}));return;}
 if(data.kind!=='button'||!data.pressed)return;
 const code=`Pad${data.pad}Button${data.button}`;
 let action=code;
 if(!binding&&![config.shotKey,config.surfaceKey,config.menuKey,config.confirmKey].includes(code)){
  action=({0:menuOpen?config.confirmKey:config.shotKey,3:menuOpen?config.confirmKey:config.shotKey,1:menuOpen?config.menuKey:config.surfaceKey,4:menuOpen?config.menuKey:config.surfaceKey,8:config.menuKey,9:menuOpen?config.confirmKey:config.menuKey})[data.button];
 }
 if(action)window.dispatchEvent(new KeyboardEvent('keydown',{code:action,bubbles:true}));
}
window.cabinetInput?.subscribe(nativeEvent);
function frame(now){
 pollPads();
 fps=fps*.95+1000/Math.max(1,now-lastFrame)*.05;lastFrame=now;
 const s=host.state,key=`${s.phase}:${s.inning}:${s.k}:${s.thrower}`;
 if(key!==lastTurn){lastTurn=key;machine.reset(now);if(s.phase==='wait')host.wind(windy);}
 if(!menuOpen&&inputReady())machine.tick(now);
 document.body.classList.toggle('arcade-native-playing',nativeInput&&!menuOpen);
 $('arcade-prompt').hidden=menuOpen||s.phase!=='aim'||!!calibration;
 $('arcade-state').textContent=machine.phase==='settling'?'·':machine.phase==='ready'?'↓':machine.phase==='back'?'↑':'↗';
 $('arcade-height').textContent=['SLIDER','NORMAL','KOBE'][s.arc];
 $('arcade-surface').textContent=s.side[s.thrower].toUpperCase();
 if(settingsOpen)$('arcade-diagnostics').textContent=`${nativeStatus?`TRACKBALL: ${nativeStatus.mice.join(", ")||"NOT FOUND"} · ${nativeStatus.pads.length} controllers\n`:""}${Math.round(fps)} FPS · ${diag.events} events\nΔ ${diag.lastX}, ${diag.lastY} · total ${Math.round(diag.x)}, ${Math.round(diag.y)}\n${lastShot?`${lastShot.mode} · power ${lastShot.power.toFixed(3)} · aim ${lastShot.aim.toFixed(3)} · spin ${lastShot.spin.toFixed(2)}`:'No throw recorded'}\nF2 setup · 1 practice · 2 players · Alt+F4 exit`;
 // Existing score/round panels remain functional; only replace the main lobby.
 const panel=$('panel');if(s.phase==='menu'){panel.hidden=true;if(!menuOpen)showMenu();}
 requestAnimationFrame(frame);
}
renderSettings();showMenu();requestAnimationFrame(frame);
window.__arcade={start,showMenu,showSettings,feed,get config(){return {...config};},setConfig(value){config=settings({...config,...value});save();machine.reset(performance.now());},get diagnostics(){return {...diag};},get machine(){return machine;}};
