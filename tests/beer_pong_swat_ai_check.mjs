import assert from 'node:assert/strict';import {OPPONENTS,SwatDefender,legal} from '../beer-pong/opponents.mjs';
const baseline=()=>({team:0,state:'fly',bounces:1,bounceAllowed:true,rimHits:0,age:.45,p:{x:0,y:.78,z:-.9},v:{x:0,y:2.5,z:-3.2}});
for(const alteration of [{bounces:0},{team:1},{state:'sink'},{bounceAllowed:false}]){const b={...baseline(),...alteration},d=new SwatDefender('boss',()=>0);assert.equal(legal(b),false);assert.equal(d.notice(b),false);for(let i=0;i<200;i++)assert.equal(d.update(1/240,b),false);assert.equal(d.phase,'idle');}
assert(legal({...baseline(),rimHits:1}),'A legal table bounce remains swattable after a rim deflection, while outside the cup');
const outcomes=[];
for(const id of Object.keys(OPPONENTS)){
 let hits=0,seed=123;const random=()=>((seed=(seed*1664525+1013904223)>>>0)/4294967296);
 for(let i=0;i<1000;i++){
  const d=new SwatDefender(id,random),b=baseline();b.p.x=(random()-.5)*.24;b.v.x=(random()-.5)*.3;d.notice(b);const noticed=b.age;
  for(let n=0;n<180;n++){b.age+=1/240;b.v.y-=9.8/240;for(const axis of ['x','y','z'])b.p[axis]+=b.v[axis]/240;if(d.update(1/240,b)){hits++;assert(b.age>noticed+.015);assert(Math.hypot(d.position.x-b.p.x,d.position.y-b.p.y,d.position.z-b.p.z)<=d.config.radius+.020001);break;}if(b.v.y<0&&b.p.y<.95&&b.p.z<-1.65)break;}
 }
 outcomes.push(hits);console.log(`${id}: ${hits}/1000 fixed-seed bounce contacts`);
}
assert(outcomes[0]>0&&outcomes[0]<outcomes[1]);assert(outcomes[1]<outcomes[2]);assert(outcomes[2]<=outcomes[3]);assert(outcomes[1]>250&&outcomes[1]<750);assert(outcomes[3]<1000,'Boss must retain a possible attention lapse');
const d=new SwatDefender('regular',()=>0),b=baseline();d.notice(b);d.reset();assert.equal(d.result,null);assert.equal(d.phase,'idle');assert.equal(d.target,null);console.log('Swat legality, physical contact, reaction timing, difficulty ordering and shot resets passed.');

const {handSwipe,SWIPE_DURATION}=await import('../beer-pong/hand-swipe.mjs');
for(const side of [-1,1]){
 const impact={x:side*.2,y:.3,z:.98},start=handSwipe(0,impact,side),middle=handSwipe(SWIPE_DURATION/2,impact,side),end=handSwipe(SWIPE_DURATION,impact,side);
 assert.equal(start.x,side*1.35);assert(Math.abs(middle.x-impact.x)<1e-12);assert.equal(middle.y,impact.y);assert.equal(middle.z,impact.z);assert.deepEqual(end,start);
 let previous=start.x;
 for(let i=1;i<=20;i++){const p=handSwipe(SWIPE_DURATION*i/40,impact,side);assert.equal(p.y,start.y);assert.equal(p.z,start.z);assert((p.x-previous)*side<=0);previous=p.x;}
}
console.log('Both hand edges start outside the viewport, swipe horizontally through impact, and return to the same edge.');
