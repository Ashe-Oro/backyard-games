import {test} from 'node:test';
import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
import {legendMarkup,loadKeysTucked,renderKeys,toggleKeys} from '../app/dist/site/arcade/legend.mjs';
import {runInNewContext} from 'node:vm';
import {Trackball,settings,profileSettings,previousSettings,legacySettings,p1Action,PLACEMENT_STEP} from '../app/dist/site/arcade/trackball.mjs';

// Exercise the actual input handlers headlessly, with browser/host boundaries stubbed.
const source=readFileSync(new URL('../app/dist/site/arcade/controller.js',import.meta.url),'utf8');
function harness(){
 const actions=[],position=[0,0],machine=new Trackball();let pads=[];
 const context={Trackball,config:settings(),p1Action,PLACEMENT_STEP,machine,
  performance:{now:()=>1000},nativeStatus:null,nativeInput:false,diag:{},menuOpen:false,tournament:false,binding:null,
  host:{state:{phase:'aim',paused:false,vs:'two',thrower:0,position},hideArrow:()=>actions.push('hide'),resetBag:()=>actions.push('reset')},
  navigator:{getGamepads:()=>pads},window:{dispatchEvent:e=>actions.push(e.code)},KeyboardEvent:class{constructor(_,options){Object.assign(this,options);}},
  inMenu:()=>context.menuOpen,console:{info(){}},inputReady:()=>false,showMenu(){},feed(){},$:()=>({hidden:true})};
 const handlers=source.slice(source.indexOf('const padState='),source.indexOf("for(const type of ['pointerdown'"));
 const placement=source.slice(source.indexOf('function nudgePosition'),source.indexOf("$('arcade-calibrate').onclick"));
 runInNewContext('let placementTarget=null;\n'+placement+'\n'+handlers+'\nglobalThis.api={pollPads,nativeEvent,nudgePosition,resetBag,clearBoard,get target(){return placementTarget;}};',context);
 return {context,actions,api:context.api,position,setPads(value){pads=value;}};
}

test('native Pad 0 routes one action per press, retains menus and other pads',()=>{
 const h=harness();
 const press=(pad,button)=>h.api.nativeEvent({kind:'button',pad,button,pressed:true});
 const release=(pad,button)=>h.api.nativeEvent({kind:'button',pad,button,pressed:false});
 for(let button=0;button<10;button++){press(0,button);press(0,button);release(0,button);}
 assert.deepEqual(h.actions,['KeyH','KeyF','KeyR','NudgeLeft','NudgeRight','KeyR','ClearBoard','ToggleKeys','Escape','Escape']);
 press(0,2);release(0,2);assert.equal(h.actions.at(-1),'KeyR');
 h.actions.length=0;h.context.menuOpen=true;
 for(const button of [0,1,2,3,4,5,6,7,8,9]){press(0,button);release(0,button);}
 assert.deepEqual(h.actions,['Enter','Escape','KeyR','Enter','Escape','KeyR','ToggleKeys','Escape','Enter']);
 h.actions.length=0;h.context.menuOpen=false;
 for(const button of [2,3,4,5]){press(1,button);release(1,button);}
 assert.deepEqual(h.actions,['KeyR','KeyH','KeyF','KeyR']);
});

test('polled Pad 0 uses the new map and rising edges; other pads keep their table',()=>{
 const h=harness();h.context.performance.now=()=>2000;
 const pad={index:0,buttons:Array.from({length:10},()=>({pressed:false}))};h.setPads([pad]);h.api.pollPads();
 for(let i=0;i<10;i++){
  pad.buttons[i].pressed=true;h.api.pollPads();h.api.pollPads();
  pad.buttons[i].pressed=false;h.api.pollPads();
 }
 assert.deepEqual(h.actions,['KeyH','KeyF','KeyR','NudgeLeft','NudgeRight','KeyR','ClearBoard','ToggleKeys','Escape','Escape']);
 h.actions.length=0;h.context.menuOpen=true;
 pad.buttons[0].pressed=true;pad.buttons[1].pressed=true;pad.buttons[6].pressed=true;pad.buttons[7].pressed=true;h.api.pollPads();
 assert.deepEqual(h.actions,['Enter','Escape','ToggleKeys']);
 h.actions.length=0;h.context.menuOpen=false;pad.index=1;pad.buttons.forEach(b=>b.pressed=false);h.api.pollPads();
 pad.buttons[2].pressed=true;pad.buttons[5].pressed=true;h.api.pollPads();
 assert.deepEqual(h.actions,['KeyR','KeyR']);
});

test('position nudges cancel windup, clamp placement and cannot throw',()=>{
 const h=harness();h.context.machine.tick(500);h.context.machine.feed(0,-100,510);
 h.api.nudgePosition(-1);assert.equal(h.api.target,-.05);assert.equal(h.context.machine.phase,'settling');
 for(let i=0;i<10;i++)h.api.nudgePosition(-1);assert.equal(h.api.target,-.45);
 for(let i=0;i<20;i++)h.api.nudgePosition(1);assert.equal(h.api.target,.45);
 assert(h.actions.every(a=>a==='hide'));
 h.context.host.state.paused=true;h.api.nudgePosition(-1);assert.equal(h.api.target,.45);
 h.context.host.state.paused=false;h.context.host.state.vs='cpu';h.context.host.state.thrower=1;
 h.api.nudgePosition(-1);assert.equal(h.api.target,.45);
 h.context.host.state.thrower=0;h.api.resetBag();assert.equal(h.api.target,0);assert.equal(h.actions.at(-1),'reset');
});

test('top 4 clears practice bags through the host once per press and respects guards',()=>{
 const page=readFileSync(new URL('../app/dist/site/cornhole/index.html',import.meta.url),'utf8');
 for(const native of [true,false]){
  const h=harness();h.context.performance.now=()=>2000;
  const state=h.context.host.state;state.practice=true;
  const bags=[{mesh:'red'},{mesh:'blue'}],removed=[];let hud=0,keydown;
  const stats={throws:8,points:3};state.stats=stats;
  const game={M:state,bags,scene:{remove:mesh=>removed.push(mesh)},renderHud:()=>hud++,
   isHuman:()=>!(state.vs==='cpu'&&state.thrower===1),thrower:()=>state.thrower};
  const clear=page.match(/function clearBags\(\) \{[^\n]+/)[0];
  const human=page.match(/const humanTurn=[^;]+;/)[0];
  const method=page.match(/ clearBoard\(\)\{[^\n]+/)[0];
  h.context.host.clearBoard=runInNewContext(clear+human+'globalThis.api={'+method+'};api.clearBoard;',game);
  h.context.window.addEventListener=(_,handler)=>{keydown=handler;};
  runInNewContext(source.slice(source.indexOf("window.addEventListener('keydown'"),source.indexOf('const padState=')),h.context);
  h.context.window.dispatchEvent=e=>{h.actions.push(e.code);keydown({...e,preventDefault(){},stopImmediatePropagation(){}});};
  const pad={index:0,buttons:Array.from({length:8},()=>({pressed:false}))};
  h.setPads([pad]);h.api.pollPads();
  const press=()=>{
   if(native){h.api.nativeEvent({kind:'button',pad:0,button:6,pressed:true});h.api.nativeEvent({kind:'button',pad:0,button:6,pressed:true});h.api.nativeEvent({kind:'button',pad:0,button:6,pressed:false});}
   else{pad.buttons[6].pressed=true;h.api.pollPads();h.api.pollPads();pad.buttons[6].pressed=false;h.api.pollPads();}
  };
  for(const [property,value] of [['practice',false],['paused',true],['phase','flight']]){
   const old=state[property];state[property]=value;press();state[property]=old;
  }
  state.vs='cpu';state.thrower=1;press();state.vs='two';state.thrower=0;
  for(const phase of ['backswing','forward','spent']){h.context.machine.phase=phase;press();}
  h.context.machine.phase='ready';h.context.menuOpen=true;const count=h.actions.length;press();
  assert.equal(h.actions.length,count);h.context.menuOpen=false;
  assert.equal(bags.length,2);assert.equal(hud,0);
  press();assert.equal(bags.length,0);assert.deepEqual(removed,['red','blue']);assert.equal(hud,1);
  assert.deepEqual(stats,{throws:8,points:3});assert.equal(h.api.target,null);
  assert(h.actions.every(action=>action==='ClearBoard'));
  for(const [property,value] of [['practice',false],['paused',true],['phase','flight']]){
   const old=state[property];state[property]=value;assert.equal(h.context.host.clearBoard(),false);state[property]=old;
  }
  assert.equal(hud,1);
 }
});

test('v9 storage migrates factory speed only and older profile keys migrate',()=>{
 const loading=source.slice(source.indexOf('let config;'),source.indexOf('const boardViewEnabled'));
 for(let version=1;version<=9;version++)for(const powerSpeed of [undefined,null,1050,1650,2100,3300,4000,5600,6500,7560,2000]){
  const profile={powerSpeed,releaseGate:220,spin:0,sensitivity:1.75,shotKey:'Pad1Button3'};
  const context={settings,profileSettings,previousSettings,legacySettings,localStorage:{getItem:key=>key===`backyard-arcade-v${version}`?JSON.stringify(profile):null}};
  runInNewContext(loading+'globalThis.loaded=config;',context);
  assert.equal(context.loaded.powerSpeed,powerSpeed===2000&&version>=5?2000:6048);
  assert.equal(context.loaded.releaseGate,version===9?220:400);
  assert.equal(context.loaded.spin,version===9?0:1);
  assert.equal(context.loaded.sensitivity,1.75);assert.equal(context.loaded.shotKey,'Pad1Button3');
 }
});

test('calibration consumes an idle-release peak, saves v9 and preserves sensitivity',()=>{
 const elements=new Map(),saved=[];
 const context={config:settings({sensitivity:1.5}),settings,machine:new Trackball(),calibration:new Trackball(),menuOpen:false,
  localStorage:{setItem:(key,value)=>saved.push([key,JSON.parse(value)])},renderSettings(){},unlock(){},showSettings(){},
  $:id=>{if(!elements.has(id))elements.set(id,{});return elements.get(id);}};
 const save=source.slice(source.indexOf('function save()'),source.indexOf('function renderSettings()'));
 const calibration=source.slice(source.indexOf('function applyCalibration('),source.indexOf('function feed('));
 runInNewContext(save+calibration,context);
 context.calibration.tick(500);context.calibration.feed(0,-21,510);context.calibration.feed(0,-21,530);
 context.shot=context.calibration.tick(690);
 runInNewContext('applyCalibration(shot);',context);
 assert.equal(context.config.powerSpeed,1500);assert.equal(context.config.sensitivity,1.5);
 assert.equal(context.calibration,null);assert.equal(saved[0][0],'backyard-arcade-v9');
 for(const [speed,expected] of [[100,400],[14000,10000]]){
  context.shot={kind:'throw',speed};runInNewContext('applyCalibration(shot);',context);
  assert.equal(context.config.powerSpeed,expected);assert.equal(context.config.sensitivity,1.5);
 }
});

test('idle throw events reach the host through the same handler as input releases',()=>{
 const shots=[],machine=new Trackball();
 const context={machine,host:{power:p=>p,throw:shot=>{shots.push(shot);return true;}},diag:{},lastShot:null};
 const handler=source.slice(source.indexOf('function handleStroke('),source.indexOf("document.addEventListener('mousemove'"));
 runInNewContext(handler,context);
 machine.tick(500);machine.feed(0,-33,510);machine.feed(0,-33,530);
 context.result=machine.tick(690);runInNewContext('handleStroke(result,690);',context);
 assert.equal(shots.length,1);assert.equal(shots[0].power,1650/6048);assert.equal(context.lastShot.at,690);
 context.result=machine.tick(700);runInNewContext('handleStroke(result,700);',context);assert.equal(shots.length,1);
});

test('sideways spinner feeds leave controller placement unchanged',()=>{
 const h=harness();
 h.context.calibration=null;
 h.context.host.throw=()=>assert.fail('sideways roll must not throw');
 h.context.host.preview=()=>assert.fail('sideways roll must not preview');
 const input=source.slice(source.indexOf('function feed('),source.indexOf("document.addEventListener('mousemove'"));
 runInNewContext(input+'\nglobalThis.spinner={feed,handleStroke,get target(){return placementTarget;},set target(value){placementTarget=value;}};',h.context);
 h.context.machine.tick(500);
 for(const target of [null,-.2,.15]){
  h.context.spinner.target=target;
  for(let i=0;i<100;i++)h.context.spinner.feed(i%2?20:-20,-2,510+i*10);
  assert.equal(h.context.spinner.target,target);
  assert.deepEqual(h.position,[0,0]);
  h.context.machine.reset(0);h.context.machine.tick(500);
 }
 h.context.spinner.handleStroke({kind:'position',dx:100},1600);
 assert.equal(h.context.spinner.target,.15);
});

test('power meter lingers two seconds; live power and menu hiding stay intact',()=>{
 const constant=source.match(/const POWER_LINGER_MS=\d+;/)[0];
 assert.equal(constant,'const POWER_LINGER_MS=2000;');
 const visibility=source.match(/\$\('arcade-power'\)\.hidden=[^;]+;/)[0];
 assert.match(visibility,/now-lastShot\.at>POWER_LINGER_MS/);
 const meter={};
 const context={$:()=>meter,menuOpen:false,live:false,lastShot:{at:1000},now:3000};
 runInNewContext(constant+visibility,context);assert.equal(meter.hidden,false);
 context.now=3001;runInNewContext(visibility,context);assert.equal(meter.hidden,true);
 context.live=true;runInNewContext(visibility,context);assert.equal(meter.hidden,false);
 context.menuOpen=true;runInNewContext(visibility,context);assert.equal(meter.hidden,true);
 context.menuOpen=false;context.live=false;context.lastShot=null;
 runInNewContext(visibility,context);assert.equal(meter.hidden,true);
});


test('one vertical POWER meter renders live and held power with color and aria values',()=>{
 const render=source.split('\n').find(line=>line.includes("const live=")&&line.includes("arcade-power-fill"));
 const elements=new Map();
 const context={machine:{phase:'forward',preview:{power:.6}},s:{phase:'aim'},
  host:{power:value=>value},lastShot:{at:1000,power:.35},now:1500,menuOpen:false,
  shotPowerColor:value=>'color:'+value,
  $:id=>{if(!elements.has(id))elements.set(id,{style:{},setAttribute(k,v){this[k]=v;}});return elements.get(id);}};
 const constant=source.match(/const POWER_LINGER_MS=\d+;/)[0];
 runInNewContext(constant,context);
 for(const [phase,power] of [['forward',.6],['settling',.35]]){
  context.machine.phase=phase;runInNewContext('{'+render+'}',context);
  assert.equal(elements.get('arcade-power').hidden,false);
  const fill=elements.get('arcade-power-fill');
  assert.equal(fill.style.height,(power*100)+'%');
  assert.equal(fill.style.width,undefined);
  assert.equal(fill.style.background,'color:'+power);
  assert.equal(elements.get('arcade-power-track')['aria-valuenow'],Math.round(power*100));
 }
 assert.doesNotMatch(source,/arcade-power-fill'\)\.style\.width/);
 assert.match(source,/<span id="arcade-power-label">POWER<\/span>/);
 assert.doesNotMatch(source,/\$\('arcade-power-label'\)\.textContent/);
 assert.match(source,/role="meter" aria-label="Throw power" aria-valuemin="0" aria-valuemax="100" aria-valuenow="0"/);
});

test('player controller, styles and markup have no removed label; bar fills upward at either display height',()=>{
 const css=readFileSync(new URL('../app/dist/site/arcade/arcade.css',import.meta.url),'utf8');
 for(const file of ['arcade/controller.js','arcade/arcade.css','arcade/legend.mjs','arcade/legend.css','cornhole/index.html']){
  const content=readFileSync(new URL('../app/dist/site/'+file,import.meta.url),'utf8');
  assert.doesNotMatch(content,/last shot/i,file);
 }
 const rules=selector=>[...css.matchAll(new RegExp(selector+'\\{([^}]+)\\}','g'))].map(match=>Object.fromEntries(match[1].split(';').filter(Boolean).map(property=>property.split(':'))));
 const tracks=rules('#arcade-power-track');
 assert(tracks.length>0);
 let dimensions={};
 for(const rule of tracks){
  dimensions={...dimensions,...rule};
  assert(parseFloat(dimensions.height)>parseFloat(dimensions.width));
 }
 const fill=rules('#arcade-power-fill')[0];
 assert.equal(fill.position,'absolute');assert.equal(fill.bottom,'0');
 assert.equal(fill.height,'0');assert.equal(fill.width,'100%');
 const ticks=rules('#arcade-power-track:after')[0];
 assert.match(ticks.background,/repeating-linear-gradient\(0deg,/);
 const meter=rules('#arcade-power')[0];
 assert.equal(meter.width,'36px');assert.equal(meter.background,undefined);
 assert.equal(meter.border,undefined);assert.equal(meter.padding,undefined);
});

test('player help stays plain and setup diagnostics stay hidden',()=>{
 const markup=source.slice(source.indexOf('root.innerHTML=`'),source.indexOf('document.body.append(root)'));
 assert.match(markup,/FULL POWER/);assert.match(markup,/\$\{legendMarkup\}/);assert.doesNotMatch(markup,/arcade-prompt|ROLL to throw|Aim where you roll|TOP &nbsp;|BOTTOM &nbsp;/);
 assert.match(markup,/<pre id="arcade-diagnostics" hidden aria-live="off"><\/pre>/);
 assert.doesNotMatch(markup,/inferred|observed|counts\/sec|70%|headroom|FPS|Top 4=|No throw recorded/);
 assert.doesNotMatch(source,/\$\('arcade-diagnostics'\)\.textContent/);
});

function legendHarness(){
 const attributes={},map={hidden:false};
 const legend={dataset:{},querySelector:selector=>selector==='#arcade-keys-map'?map:{setAttribute:(k,v)=>attributes[k]=v}};
 const saved=new Map(),storage={getItem:k=>saved.get(k)??null,setItem:(k,v)=>saved.set(k,v)};
 return {legend,map,attributes,storage};
}
test('native and polled KEYS presses toggle once, persist, and cannot reset, nudge or throw',()=>{
 for(const native of [true,false]){
  const h=harness(),l=legendHarness();let keydown;h.context.performance.now=()=>2000;
  Object.assign(h.context,{toggleKeys,localStorage:l.storage,$:()=>l.legend});
  h.context.window.addEventListener=(_,handler)=>{keydown=handler;};
  const keyboard=source.slice(source.indexOf("window.addEventListener('keydown'"),source.indexOf('const padState='));
  runInNewContext(keyboard,h.context);
  h.context.window.dispatchEvent=e=>{
   h.actions.push(e.code);
   keydown({...e,preventDefault(){},stopImmediatePropagation(){}});
  };
  renderKeys(l.legend,loadKeysTucked(l.storage));
  const pad={index:0,buttons:Array.from({length:8},()=>({pressed:false}))};
  h.setPads([pad]);h.api.pollPads();
  const press=(button,pressed)=>{
   if(native)h.api.nativeEvent({kind:'button',pad:0,button,pressed});
   else{pad.buttons[button].pressed=pressed;h.api.pollPads();}
  };
  for(const button of [7]){
   press(button,true);press(button,true);
   assert.equal(l.legend.dataset.tucked,'true');assert.equal(l.map.hidden,true);
   assert.equal(loadKeysTucked(l.storage),true);assert.equal(l.attributes['aria-expanded'],'false');
   const restored=legendHarness();renderKeys(restored.legend,loadKeysTucked(l.storage));
   assert.equal(restored.legend.dataset.tucked,'true');
   press(button,false);press(button,true);press(button,false);
   assert.equal(l.legend.dataset.tucked,'false');assert.equal(l.map.hidden,false);
   assert.equal(loadKeysTucked(l.storage),false);
  }
  assert.deepEqual(h.actions,['ToggleKeys','ToggleKeys']);
  assert.deepEqual(h.position,[0,0]);assert.equal(h.api.target,null);
  for(const button of [2,5]){press(button,true);press(button,false);}
  assert.deepEqual(h.actions.slice(-4),['KeyR','reset','KeyR','reset']);
 }
});
test('fixture embeds the game legend markup and CSS and player copy has no old card',()=>{
 const fixture=readFileSync(new URL('../legend-shots.html',import.meta.url),'utf8');
 const css=readFileSync(new URL('../app/dist/site/arcade/legend.css',import.meta.url),'utf8');
 assert(fixture.includes(legendMarkup));assert(fixture.includes(css));
 assert(readFileSync(new URL('../spin-legends.html',import.meta.url),'utf8').includes(legendMarkup));
 const page=readFileSync(new URL('../app/dist/site/cornhole/index.html',import.meta.url),'utf8');
 assert.doesNotMatch(page,/help-dialog|How to play|ROLL to throw|TOP &nbsp;|BOTTOM &nbsp;/);
 assert.deepEqual([...legendMarkup.matchAll(/<b>([^<]+)<\/b>/g)].map(m=>m[1]),['HEIGHT','SURFACE','RESET','CLEAR','LEFT','RIGHT','RESET','KEYS']);
 assert.doesNotMatch(legendMarkup,/RESET BAG|CLEAR BOARD|arcade-reset-bag/);
 assert.doesNotMatch(page,/reset-bags/);
 assert.equal((legendMarkup.match(/class="keys-button"/g)||[]).length,8);
 assert.doesNotMatch(legendMarkup,/inferred|observed|Top 4=|ROLL to throw|1 left, 3 right|TOP &nbsp;|BOTTOM &nbsp;/);
});

test('version markup and CSS provide a small upper-right label independent of HUD and legends',()=>{
 assert.match(source,/<span id="arcade-version" hidden><\/span>/);
 assert.match(source,/<p id="arcade-kicker" class="arcade-kicker">ARCADE<\/p>/);
 assert.doesNotMatch(source,/v1\.0\.[34]|['"]1\.0\.[34]['"]|ARCADE · 1\.0\.[34]/);
 const css=readFileSync(new URL('../app/dist/site/arcade/arcade.css',import.meta.url),'utf8');
 const rule=css.match(/#arcade-version\{([^}]+)\}/)[1];
 for(const property of ['position:absolute','top:1px','right:8px','font-size:8px','pointer-events:none','color:#fff1cf'])assert(rule.includes(property));
 assert.doesNotMatch(rule,/background|border|display:none|visibility:hidden/);
 assert.equal((css.match(/#arcade-version/g)||[]).length,1);
});

test('runtime version fills label and kicker; absent or failed bridge leaves label hidden',async()=>{
 const loader=source.slice(source.indexOf('async function loadAppVersion()'),source.indexOf('void loadAppVersion();'));
 for(const version of ['1.0.3','1.0.4','9.8.7']){
  const label={textContent:'',hidden:true},kicker={textContent:'ARCADE'};let calls=0;
  await runInNewContext(loader+'loadAppVersion();',{window:{arcadeApp:{version:async()=>{calls++;return version;}}},$:id=>id==='arcade-version'?label:kicker});
  assert.equal(calls,1);assert.equal(label.textContent,'v'+version);assert.equal(label.hidden,false);assert.equal(kicker.textContent,'ARCADE · '+version);
 }
 for(const window of [{},{arcadeApp:{version:async()=>null}},{arcadeApp:{version:async()=>''}},{arcadeApp:{version:async()=>{throw Error('unavailable');}}}]){
  const label={textContent:'',hidden:true},kicker={textContent:'ARCADE'};
  await runInNewContext(loader+'loadAppVersion();',{window,$:id=>id==='arcade-version'?label:kicker});
  assert.equal(label.textContent,'');assert.equal(label.hidden,true);assert.equal(kicker.textContent,'ARCADE');
 }
});

test('preload invokes version IPC and main guards installed version with trusted frame check',async()=>{
 const preload=readFileSync(new URL('../app/preload.cjs',import.meta.url),'utf8');
 const bridges={},channels=[];
 runInNewContext(preload,{require:name=>{
  assert.equal(name,'electron');
  return {contextBridge:{exposeInMainWorld:(name,api)=>bridges[name]=api},ipcRenderer:{on(){},invoke:async channel=>{channels.push(channel);return '1.0.3';}}};
 }});
 assert.equal(await bridges.arcadeApp.version(),'1.0.3');assert.deepEqual(channels,['app:version']);
 const main=readFileSync(new URL('../app/main.cjs',import.meta.url),'utf8');
 const wiring=main.slice(main.indexOf(' const trusted='),main.indexOf(" ipcMain.handle('music:list'"));
 const handlers={};let reads=0;
 runInNewContext(wiring,{ipcMain:{handle:(name,handler)=>handlers[name]=handler},app:{getVersion:()=>{reads++;return '9.8.7';}}});
 for(const event of [{},{senderFrame:{url:'https://example.com/'}},{senderFrame:{url:'backyard://game/other/'}}])assert.equal(handlers['app:version'](event),null);
 assert.equal(reads,0);assert.equal(handlers['app:version']({senderFrame:{url:'backyard://game/cornhole/index.html'}}),'9.8.7');assert.equal(reads,1);
});
