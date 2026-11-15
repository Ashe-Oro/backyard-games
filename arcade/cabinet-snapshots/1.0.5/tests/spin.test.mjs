import {test} from 'node:test';
import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
import {runInNewContext} from 'node:vm';
import * as THREE from '../resources-app/dist/site/vendor/three.module.min.js';
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
