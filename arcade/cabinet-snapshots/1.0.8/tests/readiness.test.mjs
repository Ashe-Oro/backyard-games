import {test} from 'node:test';
import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
import {Trackball,TurnInput} from '../app/dist/site/arcade/trackball.mjs';
const setup=()=>new TurnInput(new Trackball({releaseGate:1200}));
function burst(g,start=1000){let r;for(let i=0;i<5;i++)r=g.feed(0,-100,start+i*20,true);return r;}
test('cabinet snapshot reports version 1.0.8',()=>{
 const pkg=JSON.parse(readFileSync(new URL('../resources-app/package.json',import.meta.url),'utf8'));
 assert.equal(pkg.version,'1.0.8');
});
test('legal ready TurnInput stream preserves a late flick peak through release',()=>{
 const g=setup();
 assert.equal(g.sync(true,1000),true);
 assert.equal(g.machine.phase,'ready');
 // One continuous stroke: 400 counts/s, then a full 80 ms at 1650 counts/s,
 // then a coast that releases the peak. All reports remain legal.
 const stream=[
  ...Array.from({length:21},(_,i)=>({at:1000+i*20,dy:-8,speed:i?400:0,kind:'preview'})),
  ...[712.5,1025,1337.5,1650,1650].map((speed,i)=>({at:1420+i*20,dy:-33,speed,kind:'preview'})),
  {at:1520,dy:-2,speed:1650,kind:'preview'},
  {at:1540,dy:-2,speed:1650,kind:'throw'},
 ];
 const shots=[];
 for(const {at,dy,speed,kind} of stream){
  const result=g.feed(0,dy,at,true);
  assert.equal(result?.kind,kind,`report at ${at}`);
  assert.equal(result.speed,speed,`speed at ${at}`);
  assert.equal(result.power,speed/6048,`power at ${at}`);
  assert.equal(g.machine.forwardStart,1000);
  if(result.kind==='throw')shots.push(result);
 }
 assert.equal(shots.length,1);
 assert.equal(shots[0].speed,1650);
 assert.equal(shots[0].power,1650/6048);
});
test('settling without real motion accepts immediately; frame sync preserves opening',()=>{const g=setup();g.machine.reset(999);assert.equal(g.sync(true,1000),true);g.feed(0,-100,1000,true);g.sync(true,1010);assert.equal(g.machine.forwardStart,1000);assert.equal(g.feed(0,-100,1020,true).speed,5000);});
test('flight/wait burst crossing aim is entirely ignored; fresh burst has full power',()=>{for(const phase of ['flying','wait']){const g=setup();g.feed(0,-100,100,false);for(let t=120;t<400;t+=20)assert.equal(g.feed(0,-100,t,true),null,phase);assert.equal(burst(g).power,burst(setup()).power);}});
test('residual cannot launch next bag; real quiet needs no extra turn delay',()=>{const g=setup();burst(g);assert.equal(g.machine.tick(1250).kind,'throw');for(let t=1260;t<1500;t+=20)assert.equal(g.feed(0,-100,t,true),null);assert.equal(g.sync(true,2000),true);assert.equal(burst(g,2001).speed,5000);});
test('same timestamp burst is ignored entirely rather than returning a corrupt low tail',()=>{const g=setup();g.feed(0,-100,100,true);for(const t of [100,100,120,140,160])assert.equal(g.feed(0,-100,t,true),null);assert.equal(burst(g).speed,5000);});
test('ready halo is gated by human acceptance and native source time is preserved',()=>{const c=readFileSync(new URL('../app/dist/site/arcade/controller.js',import.meta.url),'utf8'),h=readFileSync(new URL('../app/dist/site/cornhole/index.html',import.meta.url),'utf8');assert(c.includes('turnInput.sync(legalThrow(),now)'));assert(c.includes("accepting&&['ready','backswing'].includes(machine.phase)"));assert(c.includes('data.at-performance.timeOrigin'));assert(h.includes("readyHalo.visible = showHeld && !M.paused && document.body.classList.contains('arcade-throw-ready')"));});
