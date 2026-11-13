import {test} from 'node:test';import assert from 'node:assert/strict';
import {Trackball,FULL_POWER_COUNTS,settings,legacySettings,previousSettings,p1Action,PLACEMENT_STEP} from '../app/dist/site/arcade/trackball.mjs';
function ready(config={}){const m=new Trackball(config);m.tick(500);return m;}
function stroke(m, dx=0, interval=10, start=510, dy=8) {
 let shot;
 for(let i=0;i<60;i++){
  const event=m.feed(dx,-dy,start+i*interval);
  if(event?.kind==='throw'){shot=event;break;}
 }
 return shot;
}
test('settings bound counts and reject conflicting or malformed mappings',()=>{const c=settings({sensitivity:Infinity,powerSpeed:-100,releaseGate:9999,shotKey:'../../file',menuKey:'F2'});assert.equal(c.sensitivity,1);assert(!("powerSpeed" in c));assert.equal(c.releaseGate,1200);assert.equal(c.shotKey,'KeyH');assert.equal(c.menuKey,'Escape');assert.equal(settings({releaseGate:20}).releaseGate,20);assert(!('mode' in settings()));});
test('sideways placement and backward rolls cannot throw',()=>{
 const m=ready();
 for(let i=0;i<100;i++)assert.equal(m.feed(15,-2,510+i*10)?.kind,'position');
 for(let i=0;i<20;i++)assert.equal(m.feed(0,20,1510+i*10),null);
 assert.equal(m.phase,'backswing');
});
test('forward-only throws aim left, straight, and right in the direction rolled',()=>{
 const [left,straight,right]=[-4,0,4].map(dx=>stroke(ready(),dx));
 assert(left.aim<0);
 assert.equal(straight.aim,0);
 assert.equal(left.aim,-right.aim);
 assert.equal(left.power,right.power);
});
test('power is linear in accumulated counts, independent of report speed',()=>{
 assert.equal(FULL_POWER_COUNTS,400);
 for(const interval of [1,10,50]){
  const m=ready();
  assert.equal(m.feed(0,0,505),null);
  for(let i=0;i<20;i++){
   const event=m.feed(0,-20,510+i*interval);
   assert.equal(event.power,(i+1)/20);
   assert.equal(event.kind,i===19?'throw':'preview');
   if(i===9)assert.equal(event.power,.5);
  }
 }
 const m=ready({sensitivity:2});
 assert.equal(m.feed(0,-100,510).power,.5);
 assert.equal(m.feed(0,-100,520).power,1);
});
test('idle before the forward roll does not reduce power',()=>{
 assert.equal(stroke(ready()).power,stroke(ready(),0,10,1600).power);
});
test('optional backswing sets proportional landing spin; forward stroke still aims',()=>{
 for(const [x,spin] of [[-20,1],[20,-1],[0,0],[-5,.25]]){
  const m=ready();m.feed(x,20,510);
  const shot=stroke(m,4,10,520);
  assert.equal(shot.spin,spin);assert(shot.aim>0);
 }
 for(const dx of [-4,0,4])assert.equal(stroke(ready(),dx).spin,0);
 const off=ready({spin:0});off.feed(-20,20,510);assert.equal(stroke(off,0,10,520).spin,0);
 const stale=ready();stale.feed(-20,20,510);stale.tick(4000);stale.tick(4500);assert.equal(stroke(stale,0,10,4510).spin,0);
});
test('one gesture makes exactly one shot; residual motion cannot throw',()=>{const m=ready();assert(stroke(m));for(let i=0;i<100;i++)assert.equal(m.feed(10,i%2?30:-30,1000+i*10),null);m.reset(2100);for(let t=2110;t<=2500;t+=10)assert.equal(m.feed(0,10,t),null);assert.equal(m.tick(2900),'settling');assert.equal(m.tick(2960),'ready');});
test('stale forward stroke, reset and malformed events cannot launch',()=>{const m=ready();m.feed(0,-40,510);m.tick(700);assert.equal(m.phase,'settling');assert.equal(m.feed(0,-200,710),null);for(const args of [[NaN,1,5000],[1,1,-1],[4000,0,5000]])assert.equal(m.feed(...args),null);m.reset(6000);assert.equal(m.preview,null);});
test('a timestamped gesture is independent of render tick frequency',()=>{const shots=[];for(const hz of [30,60,120]){const m=ready();let shot,event=0;for(let frame=500;frame<1400;frame+=1000/hz){while(event<78&&510+event*10<=frame){const i=event++,r=m.feed(i<18?1:2,i<18?10:-8,510+i*10);if(r?.kind==='throw')shot=r;}m.tick(frame);}shots.push(shot);}assert.deepEqual(shots[0],shots[1]);assert.deepEqual(shots[1],shots[2]);});
test('saved speed profiles migrate to counts without losing bindings or sensitivity',()=>{
 for(const powerSpeed of [400,2000,6500,9000])for(const releaseGate of [48,80,220]){
  const c=legacySettings({powerSpeed,releaseGate,sensitivity:1.5,shotKey:'Pad1Button3'});
  assert(!('powerSpeed' in c));assert.equal(c.releaseGate,400);
  assert.equal(c.sensitivity,1.5);assert.equal(c.shotKey,'Pad1Button3');
  assert.equal(settings({powerSpeed,releaseGate}).releaseGate,releaseGate);
 }
});
test('a single oversized motion report cannot launch; distinct reports are required',()=>{
 const m=ready();
 assert.equal(m.feed(0,-800,510).kind,'preview');
 assert.equal(m.feed(0,-20,510).kind,'preview');
 assert.equal(m.feed(0,-20,540).kind,'throw');
});
test('default profile waits for a sustained 400-count forward stroke',()=>{
 const m=ready();
 for(let i=0;i<19;i++)assert.equal(m.feed(0,-20,520+i*10).kind,'preview');
 const shot=m.feed(0,-20,710);assert.equal(shot.kind,'throw');assert.equal(shot.power,1);
});
test('larger release gates allow 400 counts before firing and keep power bounded',()=>{
 const m=ready({releaseGate:600});
 assert.equal(m.feed(0,-200,510).power,.5);
 const full=m.feed(0,-200,520);assert.equal(full.kind,'preview');assert.equal(full.power,1);
 const shot=m.feed(0,-200,530);assert.equal(shot.kind,'throw');assert.equal(shot.power,1);
});

test('P1 eight-button play map, menu aliases and Start/Select',()=>{
 const c=settings();
 assert.deepEqual(Array.from({length:8},(_,i)=>p1Action(i,c)),['KeyH','KeyF','NudgeLeft','NudgeRight',undefined,'KeyR',undefined,undefined]);
 assert.deepEqual(Array.from({length:8},(_,i)=>p1Action(i,c,true)),['Enter','Escape','KeyR','Enter','Escape','KeyR',undefined,undefined]);
 assert.equal(PLACEMENT_STEP,.1);
 assert.deepEqual([8,9].map(i=>p1Action(i,c)),['Escape','Escape']);
 assert.deepEqual([8,9].map(i=>p1Action(i,c,true)),['Escape','Enter']);
 assert.equal(settings().spin,1);assert.equal(legacySettings({spin:0}).spin,1);
});

test('v7/v6/v5 and legacy migration change only factory gates and spin',()=>{
 for(const migrate of [previousSettings,legacySettings]){
  for(const releaseGate of [48,80,220,20,100,240,380,400,600,1200]){
   for(const spin of [0,.35,1]){
    const c=migrate({releaseGate,spin,sensitivity:1.75,shotKey:'Pad1Button3',surfaceKey:'KeyS',powerSpeed:1050});
    assert.equal(c.releaseGate,[48,80,220].includes(releaseGate)?400:releaseGate);
    assert.equal(c.spin,spin===0?1:spin);
    assert.equal(c.sensitivity,1.75);assert.equal(c.shotKey,'Pad1Button3');assert.equal(c.surfaceKey,'KeyS');
    assert(!('powerSpeed' in c));
   }
  }
  assert.equal(migrate().releaseGate,400);assert.equal(migrate().spin,1);
 }
 // Current v8 profiles can explicitly disable spin or choose a short gate.
 assert.equal(settings({spin:0}).spin,0);
 const m=ready({releaseGate:200});
 assert.equal(m.feed(0,-100,510).kind,'preview');
 const shot=m.feed(0,-100,520);assert.equal(shot.kind,'throw');assert.equal(shot.power,.5);
});
