import {test} from 'node:test';
import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
// Load the unchanged browser-vendored ESM through a data URL: no npm or WebGL.
const moduleURL=source=>'data:text/javascript;base64,'+Buffer.from(source).toString('base64');
const threeURL=moduleURL(readFileSync(new URL('../app/dist/site/vendor/three.module.min.js',import.meta.url),'utf8'));
const source=readFileSync(new URL('../app/dist/site/arcade/party.mjs',import.meta.url),'utf8');
const {pongCatalog,buildPongScript,selectNext,createParty,pongTiming,buildPongMissPath}=await import(moduleURL(source.replace("'three'",JSON.stringify(threeURL))));
const THREE=await import(threeURL);

test('five distinct first-to-ten scripts have balanced winners and valid shot types',()=>{
 assert.equal(pongCatalog.length,5);
 assert.equal(new Set(pongCatalog.map(e=>e.id)).size,5);
 for(const random of [()=>0,()=>.9]) for(const entry of pongCatalog){
  const game=buildPongScript(entry.id,random),hits=[0,0];
  game.turns.forEach((shot,index)=>{
   assert.deepEqual(shot.before,hits);
   assert.ok(['hit','miss','bounce','swat'].includes(shot.result));
   assert.equal(shot.scores,['hit','bounce'].includes(shot.result));
   if(shot.scores) hits[shot.side]++;
   assert.ok(Math.max(...hits)<=10);
   if(hits.includes(10)) assert.equal(index,game.turns.length-1);
  });
  assert.equal(hits[game.winner],10);assert.ok(hits[1-game.winner]<10);
  if(entry.id.startsWith('red-')) assert.equal(game.winner,0);
  if(entry.id.startsWith('blue-')) assert.equal(game.winner,1);
 }
});
test('blowout, close, solid, long and both comeback orientations have distinct flows',()=>{
 const games=Object.fromEntries(pongCatalog.map(e=>[e.id,buildPongScript(e.id,()=>.9)]));
 assert.deepEqual(games['red-blowout'].hits,[10,3]);
 assert.deepEqual(games['blue-close'].hits,[9,10]);
 assert.deepEqual(games['blue-solid'].hits,[7,10]);
 assert.deepEqual(games['red-long'].hits,[10,8]);
 assert.equal(pongCatalog.filter(e=>e.id.startsWith('red-')).length,2);
 assert.equal(pongCatalog.filter(e=>e.id.startsWith('blue-')).length,2);
 for(const side of [0,1]) assert.ok(games['blue-solid'].turns.some(t=>t.side===side&&!t.scores));
 assert.ok(games['blue-solid'].turns.length>games['red-blowout'].turns.length);
 for(const game of Object.values(games)){
  if(game.id!=='red-long') assert.ok(game.turns.length<games['red-long'].turns.length);
 }
 for(const n of [0,.9]){
  const game=buildPongScript('comeback',()=>n);
  assert.equal(game.winner,n===0?1:0);
  assert.equal(game.hits[1-game.winner],6);
  assert.ok(game.turns.some(t=>t.before[1-game.winner]-t.before[game.winner]>=4));
 }
 const close=games['blue-close'];
 assert.ok(close.turns.every(t=>Math.abs(t.before[0]-t.before[1])<=2));
 assert.ok(games['red-long'].turns.filter(t=>!t.scores).length>=20);
});
test('selector reaches all five on load and excludes previous ids',()=>{
 const seen=new Set();
 for(let i=0;i<5;i++) seen.add(selectNext(null,()=> (i+.5)/5).id);
 assert.equal(seen.size,5);
 for(const previous of pongCatalog) for(let i=0;i<4;i++){
  assert.notEqual(selectNext(previous.id,()=> (i+.5)/4).id,previous.id);
 }
 let previous=null;const sequence=new Set();
 for(let i=0;i<50;i++){const next=selectNext(previous,()=>((i*7)%10)/10);assert.notEqual(next.id,previous);sequence.add(next.id);previous=next.id;}
 assert.equal(sequence.size,5);
});
test('party loads random scripts, scores on impact, celebrates, and resets without repeats',()=>{
 for(let i=0;i<5;i++){
  const party=createParty(new THREE.Scene(),{random:()=> (i+.5)/5});
  assert.equal(party.pongVariationId,pongCatalog[i].id);
 }
 const party=createParty(new THREE.Scene(),{random:()=>0});
 const first=party.pongVariationId,last=party.pongTurns.at(-1);
 const impact=(party.pongTurns.length-1)*4.05+2.53;
 assert.equal(party.pongLoopSeconds,impact+5.4);
 party.update(impact-.01);assert.equal(party.pongState.winner,null);
 party.update(.02);assert.equal(party.pongState.winner,last.side);
 party.update(5.38);assert.equal(party.pongVariationId,first);
 party.update(.02);assert.notEqual(party.pongVariationId,first);
 assert.deepEqual(party.pongState.remaining,[10,10]);assert.equal(party.pongState.winner,null);
 assert.equal(party.pongState.turn,0);
 for(const shot of party.pongTurns){assert.equal(!!shot.bounce,(['bounce','swat'].includes(shot.result)||shot.missType==='rim'));assert.equal(!!shot.contact,shot.result==='swat');}
 assert.deepEqual(pongTiming,{turnSeconds:4.05,releaseAt:1.18,bounceAt:1.83,swatAt:2.12,impactAt:2.53,celebrationSeconds:5.4});
});

test('misses approach standing cup rims with occasional short and long table landings',()=>{
 const counts={rim:0,short:0,long:0};
 for(let orientation of [0,.9]) for(let i=0;i<5;i++){
  const party=createParty(new THREE.Scene(),{random:(()=>{let call=0;return ()=>call++===0?(i+.5)/5:orientation;})()});
  for(const shot of party.pongTurns.filter(t=>t.result==='miss')){
   counts[shot.missType]++;
   const index=shot.before[shot.side];
   const row=Math.floor((Math.sqrt(8*index+1)-1)/2),slot=index-row*(row+1)/2;
   const cup={x:(shot.side===0?1:-1)*(.45+row*.146),z:(slot-row/2)*.146};
   const path=buildPongMissPath(shot.side,shot.round,cup);
   assert.deepEqual({...shot.target},path.target);
   assert.equal(!!shot.bounce,shot.missType==='rim');
   if(shot.missType==='rim'){
    assert.ok(Math.abs(Math.hypot(shot.bounce.x-cup.x,shot.bounce.z-cup.z)-.063)<.006);
    assert.equal(shot.bounce.y,.795);
    assert.ok(Math.abs(shot.bounce.z)<.30);
    assert.ok(Math.abs(shot.target.z)<=.50);
    assert.ok([.032,.672].includes(shot.target.y));
   }else{
    assert.equal(Math.abs(shot.target.x),shot.missType==='short'?.34:.99);
    assert.equal(shot.target.y,.672);
    assert.ok(Math.abs(shot.target.z)<.44);
   }
  }
 }
 assert.ok(counts.rim>counts.short+counts.long);
 assert.ok(counts.short>0);assert.ok(counts.long>0);
});

test('rim misses visibly reach the lip before deflecting without removing a cup',()=>{
 const party=createParty(new THREE.Scene(),{random:()=>0});
 const index=party.pongTurns.findIndex(t=>t.missType==='rim'),shot=party.pongTurns[index];
 const contactTime=index*pongTiming.turnSeconds+pongTiming.bounceAt;
 party.update(contactTime);
 assert.ok(party.pongBall.position.distanceTo(shot.bounce)<1e-10);
 const remaining=[...party.pongState.remaining];
 party.update(.2);
 assert.ok(party.pongBall.position.distanceTo(shot.bounce)>.01);
 party.update(pongTiming.impactAt-pongTiming.bounceAt-.2-.001);
 assert.ok(party.pongBall.position.distanceTo(shot.target)<.01);
 party.update(.002);
 assert.deepEqual(party.pongState.remaining,remaining);
});
