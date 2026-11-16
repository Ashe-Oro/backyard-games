import {test} from 'node:test';
import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
import {runInNewContext} from 'node:vm';
import * as THREE from '../resources-app/dist/site/vendor/three.module.min.js';
import {Trackball} from '../app/dist/site/arcade/trackball.mjs';
import {AIR_SPIN_RATE,landingCurl,slideVelocity} from '../resources-app/dist/site/cornhole/spin.mjs';
const source=readFileSync(new URL('../resources-app/dist/site/cornhole/index.html',import.meta.url),'utf8');
function physics(original=false){
 const ctx={THREE,AIR_SPIN_RATE,landingCurl,slideVelocity,clamp:(v,a,b)=>Math.max(a,Math.min(b,v))};
 let body=source.slice(source.indexOf('function newBody('),source.indexOf('// calibrated power windows'));
 if(original){body=body.replace('b.curl=landingCurl(b.english,b.side)','b.curl=0').replace('b.curl = slideVelocity(lv,b.curl,b.side,S,G_LOCAL,G_N,dt);',`lv.x += G_LOCAL.x * dt; lv.z += G_LOCAL.z * dt;
 const sp=Math.hypot(lv.x,lv.z),dec=S.mu*G_N*dt;
 if(sp<=dec){lv.x=0;lv.z=0;}else{lv.x-=lv.x/sp*dec;lv.z-=lv.z/sp*dec;}`);}
 const constants=source.slice(source.indexOf('const G ='),source.indexOf('const TEAM ='));
 runInNewContext(constants+`
 const target={deckG:{getWorldQuaternion(q){return q.setFromAxisAngle(new THREE.Vector3(1,0,0),TH)},matrixWorld:new THREE.Matrix4().compose(new THREE.Vector3(0,FRONT_H,BOARD_Z),new THREE.Quaternion().setFromAxisAngle(new THREE.Vector3(1,0,0),TH),new THREE.Vector3(1,1,1))}};
 const HOLE_WORLD=new THREE.Vector3();
 `+source.slice(source.indexOf('const boardInv ='),source.indexOf('// ---------- bag physics'))+body+
 '\nglobalThis.api={newBody,launch,step,collide,simulate,HOLE_LOCAL,TH,G_N,G_LOCAL};',ctx);
 return ctx.api;
}
const api=physics();
// Execute the shipped helper with the snapshot's bundled Three.js, as the browser import map does.
const arrowSource=readFileSync(new URL('../app/dist/site/yard-games/power-arrow.js',import.meta.url),'utf8');
const arrowContext={THREE};
runInNewContext(arrowSource.replace(/import[^\n]+\n/,'').replace(/export /g,'')+
 '\nglobalThis.arrow={POWER_ARROW_MAX,powerArrowLength,paintPowerArrow};',arrowContext);
const {POWER_ARROW_MAX,powerArrowLength,paintPowerArrow}=arrowContext.arrow;

function slide(spin,side='slick',arc=1){
 const b=api.newBody(0,side);b.state='slide';b.arc=arc;b.english=spin;b.curl=landingCurl(spin,side);
 b.lp.set(6*.0254,.016,api.HOLE_LOCAL.z+.32);b.lv.set(0,0,-2);
 for(let n=0;n<480&&b.state==='slide';n++)api.step(b,1/240);
 return b;
}
test('six-inch-right slick normal and slider landings curl left into the hole only with spin',()=>{
 for(const arc of [0,1]){assert.equal(slide(1,'slick',arc).state,'hole');assert.notEqual(slide(0,'slick',arc).state,'hole');}
});
test('clockwise curls right, counterclockwise left; slick carries farther than sticky',()=>{
 assert(landingCurl(-1,'slick')>0);assert(landingCurl(1,'slick')<0);assert.equal(landingCurl(0,'slick'),0);
 const slick=slide(1),sticky=slide(1,'sticky');assert(slick.lp.x<sticky.lp.x-.05);assert.notEqual(sticky.state,'hole');
});
test('zero-spin CPU calibration matches original slide arithmetic for every WIN sample',()=>{
 const baseline=physics(true);
 for(const side of ['slick','sticky'])for(const arc of [0,1,2])for(let p=0;p<=1;p+=.0025)assert.equal(api.simulate(0,p,side,0,0,arc),baseline.simulate(0,p,side,0,0,arc));
});
test('spun sliders retain blocker contacts and wake a resting bag',()=>{
 const a=api.newBody(0),b=api.newBody(1);a.state='slide';b.state='board';a.lp.set(0,.016,api.HOLE_LOCAL.z+.4);b.lp.set(0,.016,a.lp.z-.14);a.lv.set(0,0,-2);a.curl=landingCurl(1,'slick');
 api.collide([a,b]);assert.equal(b.state,'slide');assert(b.lv.z<0);api.step(a,1/240);assert(Number.isFinite(a.lp.x));
});

// Run the production launch and physics through first deck contact, then rest.
// rx is world metres; -0.20 m equals four completed 0.05 m LEFT nudges.
function toss(rx,aim=0,spin=0){
 const b=api.newBody(0,'slick');api.launch(b,rx,.65,aim,0,1,spin);
 let landing=null;
 for(let n=0;n<240*8;n++){
  const event=api.step(b,1/240);
  if(!landing&&(event==='land'||event==='hole'))landing={x:b.lp.x,z:b.lp.z};
  if(['board','grass','in','removed'].includes(b.state))break;
 }
 assert(landing,'forward toss must reach the deck');
 return {landing,x:b.p.x,state:b.state};
}
test('nudged straight throws land at their release x, parallel to the center line',()=>{
 for(const rx of [-.20,0,.20]){
  const shot=toss(rx);
  assert(Math.abs(shot.landing.x-rx)<.001,`release ${rx} m landed at ${shot.landing.x} m`);
  assert(Math.abs(shot.x-rx)<.001,`release ${rx} m finished at ${shot.x} m`);
  if(rx)assert.equal(Math.sign(shot.landing.x-api.HOLE_LOCAL.x),Math.sign(rx));
 }
});
test('forward 1/3 aim remains an offset on top of the nudged straight path',()=>{
 for(const aim of [-.015,.015]){
  const centered=toss(0,aim),shifted=toss(-.10,aim);
  assert.equal(Math.sign(centered.landing.x),Math.sign(aim));
  assert(Math.abs(centered.landing.x)>.05);
  assert(Math.abs(shifted.landing.x-centered.landing.x+.10)<.001);
 }
});
test('backswing A/C curl remains an offset after a nudged straight landing',()=>{
 const straight=toss(-.20);
 for(const [spin,direction] of [[-1,1],[1,-1]]){
  const shot=toss(-.20,0,spin);
  assert(Math.abs(shot.landing.x+.20)<.001);
  assert((shot.x-straight.x)*direction>.005,`spin ${spin} must curl in direction ${direction}`);
 }
});

function arrowHarness(width=1280,height=720,x=0){
 const camera=new THREE.PerspectiveCamera(19,width/height,.1,200);
 const portrait=Math.max(0,Math.min(1,(1.2-width/height)/.65));
 camera.position.set(0,3.6,8).lerp(new THREE.Vector3(0,2.6,4),portrait);
 camera.lookAt(0,.5,-8.7);camera.fov=18+6*portrait;
 camera.updateProjectionMatrix();camera.updateMatrixWorld();
 const calls=[],rect={left:17,top:23,width,height};
 const ctx={THREE,camera,powerArrowLength,REL_Y:.98,REL_Z:-.45,releaseX:()=>x,thrower:()=>0,
  wrap:{getBoundingClientRect:()=>rect},showAim:(from,to,pulling,power)=>calls.push({from,to,power})};
 runInNewContext(source.slice(source.indexOf('function bagScreen('),source.indexOf('function showAim('))+
  source.slice(source.indexOf('function showShotArrow('),source.indexOf('const gauss'))+'globalThis.draw=showShotArrow;',ctx);
 return {ctx,calls,project:(x,z)=>{const p=new THREE.Vector3(x,.98,z).project(camera);return {x:17+(p.x+1)*width/2,y:23+(1-p.y)*height/2};}};
}
test('shot guide matches launched velocity yaw including drift, with mirrored and obvious extremes',()=>{
 for(const [width,height] of [[1280,720],[1920,1080],[720,1280]]){
  const offsets=[];
  for(const aim of [-.1,-.025,0,.025,.1]){
   const h=arrowHarness(width,height),b=api.newBody(0,'slick');api.launch(b,0,.2,aim,0,1,0);h.ctx.draw(b,1);
   const {from,to}=h.calls[0];offsets.push(to.x-from.x);
   assert(to.y<from.y,'guide points forward');
   assert(Math.abs(Math.hypot(to.x-from.x,to.y-from.y)-110)<1e-9);
  }
  assert(Math.abs(offsets[2])<1e-9,'straight follows center line');
  assert(Math.abs(offsets[0]+offsets[4])<1e-9);assert(Math.abs(offsets[1]+offsets[3])<1e-9);
  assert(offsets[4]>20,'strong 3 is clearly right on a compact shaft');
  assert(offsets[0]<-20);assert(offsets[4]>offsets[3]*3);assert(offsets[0]<offsets[1]*3);
  assert(offsets[4]-offsets[3]>15);assert(offsets[1]-offsets[0]>15);
 }
 for(const x of [-.2,0,.2])for(const aim of [-.1,0,.1])for(const drift of [-.4,0,.4]){
  const h=arrowHarness(1280,720,x),b=api.newBody(0,'slick');api.launch(b,x,.65,aim,drift,1,0);h.ctx.draw(b,.65);
  const yaw=Math.atan2(b.v.x,-b.v.z),expected=h.project(x+Math.sin(yaw),-.45-Math.cos(yaw));
  const {from,to,power}=h.calls[0];assert.equal(from.x,h.project(x,-.45).x);assert.equal(power,.65);
  const dx=expected.x-from.x,dy=expected.y-from.y,len=powerArrowLength(.65);
  assert(Math.abs(to.x-from.x-dx/Math.hypot(dx,dy)*len)<1e-9);
  assert(Math.abs(to.y-from.y-dy/Math.hypot(dx,dy)*len)<1e-9);
 }
});
test('live host preview uses scaled aim and shares released-shot geometry',()=>{
 const h=arrowHarness();Object.assign(h.ctx,{humanTurn:()=>true,M:{},setMeter(){},physicalPower:p=>p});
 const preview=source.match(/ preview\(shot\)\{[^\n]+/)[0].trim().replace(/,$/,'');
 runInNewContext('globalThis.host={'+preview+'};',h.ctx);
 for(const power of [.1,.5,1])for(const aim of [-.1,0,.025,.1]){
  h.ctx.host.preview({aim,power});const live=h.calls.at(-1),b=api.newBody(0,'slick');
  api.launch(b,0,power,aim,0,1,0);h.ctx.draw(b,power);const released=h.calls.at(-1);
  assert(Math.abs(live.to.x-released.to.x)<1e-9);assert(Math.abs(live.to.y-released.to.y)<1e-9);
 }
});
test('real forward strokes keep their 1/3 scaled aim through preview, launch and released arrow',()=>{
 for(const dx of [-45,-15,0,15,45]){
  const machine=new Trackball();machine.tick(500);
  machine.feed(dx,-30,510);const preview=machine.feed(dx,-30,530),shot=machine.tick(690);
  assert.equal(preview.kind,'preview');assert.equal(shot.kind,'throw');
  const expected=Math.max(-.1,Math.min(.1,Math.atan2(dx,30)*.10));
  assert.equal(shot.aim,expected);assert.equal(preview.aim,expected);
  const h=arrowHarness();Object.assign(h.ctx,{humanTurn:()=>true,M:{},setMeter(){},physicalPower:p=>p});
  const hostPreview=source.match(/ preview\(shot\)\{[^\n]+/)[0].trim().replace(/,$/,'');
  runInNewContext('globalThis.host={'+hostPreview+'};',h.ctx);
  h.ctx.host.preview(preview);const live=h.calls.at(-1),b=api.newBody(0,'slick');
  api.launch(b,0,shot.power,shot.aim,0,1,shot.spin);h.ctx.draw(b,shot.power);
  assert(Math.abs(Math.atan2(b.v.x,-b.v.z)-expected)<1e-12);
  assert(Math.abs(live.to.x-h.calls.at(-1).to.x)<1e-9);
 }
});

test('power grows a short shaft to compact full length on a fixed faint guide',()=>{
 assert.equal(POWER_ARROW_MAX,110);
 assert.equal(powerArrowLength(0),24);assert.equal(powerArrowLength(2),110);
 const lengths=[],colors=[];
 for(const power of [.1,.5,1]){
  const h=arrowHarness();const b=api.newBody(0,'slick');api.launch(b,0,power,0,0,1,0);h.ctx.draw(b,power);
  const {from,to}=h.calls[0];
  assert(Math.abs(to.x-from.x)<1e-9,'all straight powers share a direction');
  const length=Math.hypot(to.x-from.x,to.y-from.y);lengths.push(length);
  assert.equal(length,powerArrowLength(power));
  const parts=Object.fromEntries(['shaft','tip','grip','track','limit'].map(k=>[k,{setAttribute(key,value){this[key]=value;}}]));
  const svg={querySelector:selector=>parts[selector.match(/data-power-(\w+)/)[1]]};
  const rect={left:17,top:23};paintPowerArrow(svg,power,from,to,rect);
  assert(Math.abs(Math.hypot(parts.track.x2-parts.track.x1,parts.track.y2-parts.track.y1)-110)<1e-9);
  assert.equal(parts.limit.cx,parts.track.x2);assert.equal(parts.limit.cy,parts.track.y2);
  colors.push(parts.shaft.stroke);assert.equal(parts.tip.fill,parts.shaft.stroke);
 }
 assert(lengths[0]>=20&&lengths[0]<=30);assert(lengths[1]>lengths[0]+25);
 assert(lengths[2]>lengths[1]+50);assert.equal(lengths[2],110);
 assert.equal(new Set(colors).size,3);
 assert.doesNotMatch(source,/#power-track,#power-limit\{display:none/);
 assert.match(source,/data-power-track[^>]*opacity=".28"/);
 assert.match(source,/id="swipe-line"[^>]*stroke-width="3"/);
});

test('production SVG rendering grows shaft and outline together on the full-power track',()=>{
 const h=arrowHarness(),elements=new Map();
 const element=id=>{
  if(!elements.has(id))elements.set(id,{setAttribute(k,v){this[k]=v;},classList:{add(){},toggle(){},remove(){}}});
  return elements.get(id);
 };
 const roles={shaft:'swipe-line',tip:'swipe-tip-fill',grip:'pull-grip',track:'power-track',limit:'power-limit'};
 element('swipe-feedback').querySelector=selector=>element(roles[selector.match(/data-power-(\w+)/)[1]]);
 Object.assign(h.ctx,{$:element,paintPowerArrow,setTimeout:()=>1,clearTimeout(){}});
 runInNewContext('const ARROW_LINGER_MS=2000;let swipeTimer;'+
  source.slice(source.indexOf('function showAim('),source.indexOf('function pullAt(')),h.ctx);
 for(const power of [.1,.5,1]){
  const b=api.newBody(0,'slick');api.launch(b,0,power,.1,0,1,0);h.ctx.draw(b,power);
  const shaft=element('swipe-line'),outline=element('swipe-outline'),track=element('power-track');
  for(const key of ['x1','x2','y1','y2'])assert.equal(shaft[key],outline[key]);
  assert(Math.abs(Math.hypot(shaft.x2-shaft.x1,shaft.y2-shaft.y1)-powerArrowLength(power))<1e-9);
  assert(Math.abs(Math.hypot(track.x2-track.x1,track.y2-track.y1)-110)<1e-9);
  assert.equal(shaft.x1,track.x1);assert.equal(shaft.y1,track.y1);
  if(power===1){assert(Math.abs(shaft.x2-track.x2)<1e-9);assert(Math.abs(shaft.y2-track.y2)<1e-9);}
 }
});
