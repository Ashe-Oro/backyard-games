import {test} from 'node:test';
import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
import {runInNewContext} from 'node:vm';
import {Trackball,settings,p1Action,PLACEMENT_STEP,FULL_POWER_COUNTS} from '../app/dist/site/arcade/trackball.mjs';

// Exercise the actual input handlers headlessly, with browser/host boundaries stubbed.
const source=readFileSync(new URL('../app/dist/site/arcade/controller.js',import.meta.url),'utf8');
function harness(){
 const actions=[],position=[0,0],machine=new Trackball();let pads=[];
 const context={Trackball,FULL_POWER_COUNTS,config:settings(),p1Action,PLACEMENT_STEP,machine,
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
 assert.deepEqual(h.actions,['KeyH','KeyF','NudgeLeft','NudgeRight','KeyR','Escape','Escape']);
 press(0,2);release(0,2);assert.equal(h.actions.at(-1),'NudgeLeft');
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
 assert.deepEqual(h.actions,['KeyH','KeyF','NudgeLeft','NudgeRight','KeyR','Escape','Escape']);
 h.actions.length=0;h.context.menuOpen=true;
 pad.buttons[0].pressed=true;pad.buttons[1].pressed=true;h.api.pollPads();
 assert.deepEqual(h.actions,['Enter','Escape']);
 h.actions.length=0;h.context.menuOpen=false;pad.index=1;pad.buttons.forEach(b=>b.pressed=false);h.api.pollPads();
 pad.buttons[2].pressed=true;pad.buttons[5].pressed=true;h.api.pollPads();
 assert.deepEqual(h.actions,['KeyR','KeyR']);
});

test('position nudges cancel windup, clamp placement and cannot throw',()=>{
 const h=harness();h.context.machine.tick(500);h.context.machine.feed(0,-100,510);
 h.api.nudgePosition(-1);assert.equal(h.api.target,-.1);assert.equal(h.context.machine.phase,'settling');
 for(let i=0;i<10;i++)h.api.nudgePosition(-1);assert.equal(h.api.target,-.45);
 for(let i=0;i<20;i++)h.api.nudgePosition(1);assert.equal(h.api.target,.45);
 assert(h.actions.every(a=>a==='hide'));
 h.context.host.state.paused=true;h.api.nudgePosition(-1);assert.equal(h.api.target,.45);
 h.context.host.state.paused=false;h.context.host.state.vs='cpu';h.context.host.state.thrower=1;
 h.api.nudgePosition(-1);assert.equal(h.api.target,.45);
 h.context.host.state.thrower=0;h.api.resetBag();assert.equal(h.api.target,0);assert.equal(h.actions.at(-1),'reset');
});
