import {test} from 'node:test';
import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
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
 const placement=source.slice(source.indexOf('function nudgePosition'),source.indexOf("$('arcade-reset-bag').onclick"));
 runInNewContext('let placementTarget=null;\n'+placement+'\n'+handlers+'\nglobalThis.api={pollPads,nativeEvent,nudgePosition,resetBag,get target(){return placementTarget;}};',context);
 return {context,actions,api:context.api,position,setPads(value){pads=value;}};
}

test('native Pad 0 routes one action per press, retains menus and other pads',()=>{
 const h=harness();
 const press=(pad,button)=>h.api.nativeEvent({kind:'button',pad,button,pressed:true});
 const release=(pad,button)=>h.api.nativeEvent({kind:'button',pad,button,pressed:false});
 for(let button=0;button<10;button++){press(0,button);press(0,button);release(0,button);}
 assert.deepEqual(h.actions,['KeyH','KeyF','KeyR','NudgeLeft','NudgeRight','KeyR','KeyR','KeyR','Escape','Escape']);
 press(0,2);release(0,2);assert.equal(h.actions.at(-1),'KeyR');
 h.actions.length=0;h.context.menuOpen=true;
 for(const button of [0,1,3,4,8,9]){press(0,button);release(0,button);}
 assert.deepEqual(h.actions,['Enter','Escape','Enter','Escape','Escape','Enter']);
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
 assert.deepEqual(h.actions,['KeyH','KeyF','KeyR','NudgeLeft','NudgeRight','KeyR','KeyR','KeyR','Escape','Escape']);
 h.actions.length=0;h.context.menuOpen=true;
 pad.buttons[0].pressed=true;pad.buttons[1].pressed=true;h.api.pollPads();
 assert.deepEqual(h.actions,['Enter','Escape']);
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

test('v9 storage migrates factory speed only and older profile keys migrate',()=>{
 const loading=source.slice(source.indexOf('let config;'),source.indexOf('const boardViewEnabled'));
 for(let version=1;version<=9;version++)for(const powerSpeed of [undefined,1050,1650,2100,6500,2000]){
  const profile={powerSpeed,releaseGate:220,spin:0,sensitivity:1.75,shotKey:'Pad1Button3'};
  const context={settings,profileSettings,previousSettings,legacySettings,localStorage:{getItem:key=>key===`backyard-arcade-v${version}`?JSON.stringify(profile):null}};
  runInNewContext(loading+'globalThis.loaded=config;',context);
  assert.equal(context.loaded.powerSpeed,powerSpeed===2000&&version>=5?2000:3300);
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
 assert.equal(shots.length,1);assert.equal(shots[0].power,.5);assert.equal(context.lastShot.at,690);
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
