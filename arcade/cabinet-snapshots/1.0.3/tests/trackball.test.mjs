import {test} from 'node:test';import assert from 'node:assert/strict';
import {Trackball,DEFAULTS,SPEED_WINDOW_MS,settings,profileSettings,legacySettings,previousSettings,p1Action,PLACEMENT_STEP} from '../app/dist/site/arcade/trackball.mjs';
function ready(config={}){const m=new Trackball(config);m.tick(500);return m;}
function stroke(m, dx=0, interval=10, start=510, dy=8) {
 let shot;
 for(let i=0;i<60;i++){
  const event=m.feed(dx,-dy,start+i*interval);
  if(event?.kind==='throw'){shot=event;break;}
 }
 return shot;
}
test('settings bound speed and counts and reject conflicting or malformed mappings',()=>{const c=settings({sensitivity:Infinity,powerSpeed:-100,releaseGate:9999,shotKey:'../../file',menuKey:'F2'});assert.equal(c.sensitivity,1);assert.equal(c.powerSpeed,400);assert.equal(settings({powerSpeed:99999}).powerSpeed,10000);assert.equal(settings().powerSpeed,6048);assert.equal(DEFAULTS.powerSpeed,6048);assert.equal(c.releaseGate,1200);assert.equal(c.shotKey,'KeyH');assert.equal(c.menuKey,'Escape');assert.equal(settings({releaseGate:20}).releaseGate,20);assert(!('mode' in settings()));});
test('sideways and backward rolls cannot place or throw',()=>{
 const m=ready();
 for(let i=0;i<100;i++)assert.equal(m.feed(15,-2,510+i*10),null);
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
// Finish at a direction change so distance is never required to reach a gate.
function constantRoll(speed,counts,config={}){
 const m=ready({releaseGate:1200,...config}),interval=20,dy=speed*interval/1000;
 let result=m.feed(0,-dy,510),now=510;
 for(let i=1;i<Math.round(counts/dy);i++){
  now+=interval;result=m.feed(0,-dy,now);
  if(result?.kind==='throw')return result;
 }
 return m.feed(20,0,now+1);
}
test('short fast flick throws at roll end, without reaching the gate',()=>{
 const m=ready();m.feed(0,-33,510);
 const preview=m.feed(0,-33,530);assert.equal(preview.power,1650/6048);
 const shot=m.feed(0,10,531);assert.equal(shot.kind,'throw');assert.equal(shot.speed,1650);
 assert(-shot.forward.y<settings().releaseGate);
});
test('windowed power is linear in speed and independent of distance',()=>{
 assert.equal(SPEED_WINDOW_MS,80);
 for(const speed of [3024,6048,9000]){
  const shot=constantRoll(speed,800);
  assert(Math.abs(shot.speed-speed)<1e-9);assert(Math.abs(shot.power-Math.min(1,speed/6048))<1e-12);
 }
 for(const counts of [100,800]){
  const shot=constantRoll(1000,counts);assert.equal(shot.kind,'throw');
  assert(Math.abs(shot.power-1000/6048)<1e-12);assert(Math.abs(shot.speed-1000)<1e-9);
 }
 const slow=constantRoll(400,800);assert.equal(slow.kind,'throw');
 assert(Math.abs(slow.power-400/6048)<1e-12);
 const doubled=constantRoll(1000,100,{sensitivity:2});assert.equal(doubled.power,2000/6048);
});
test('idle before the forward roll does not reduce power',()=>{
 assert.equal(stroke(ready()).power,stroke(ready(),0,10,1600).power);
});
test('A negative-X backswing is clockwise/right; C positive-X is counterclockwise/left; forward still aims',()=>{
 for(const [x,spin] of [[-20,-1],[20,1],[0,0],[-5,-.25]]){
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
test('saved speed profiles migrate without losing bindings or sensitivity',()=>{
 for(const powerSpeed of [400,2000,5600,6500,7560,9000])for(const releaseGate of [48,80,220]){
  const c=legacySettings({powerSpeed,releaseGate,sensitivity:1.5,shotKey:'Pad1Button3'});
  assert.equal(c.powerSpeed,[5600,6500,7560].includes(powerSpeed)?6048:Math.max(6048,powerSpeed));assert.equal(c.releaseGate,400);
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
test('release gate is only an upper bound and requires a valid speed window',()=>{
 const m=ready({releaseGate:200});
 assert.equal(m.feed(0,-100,510).kind,'preview');
 assert.equal(m.feed(0,-100,520).kind,'preview'); // only 10 ms
 const shot=m.feed(0,-10,530);assert.equal(shot.kind,'throw');assert.equal(shot.speed,5500);assert.equal(shot.power,5500/6048);
 const short=constantRoll(1000,100,{releaseGate:80});assert.equal(short.kind,'throw');assert(Math.abs(short.power-1000/6048)<1e-12);
});
test('a clear speed drop releases peak power while the spinner still coasts',()=>{
 const m=ready({releaseGate:1200});let result;
 for(let i=0;i<5;i++)result=m.feed(0,-33,510+i*20);
 assert.equal(result.kind,'preview');
 for(let i=5;i<9;i++){result=m.feed(0,-2,510+i*20);if(result.kind==='throw')break;}
 assert.equal(result.kind,'throw');assert.equal(result.power,1650/6048);assert.equal(result.speed,1650);
 assert(-result.forward.y<1200);
 for(let i=0;i<100;i++)assert.equal(m.feed(0,-2,700+i*20),null);
});
test('idle, zero-motion report, and timeout release the current peak exactly once',()=>{
 for(const end of ['tick','feed','timeout']){
  const m=ready({releaseGate:1200,timeoutMs:1000});m.feed(0,-10,510);m.feed(0,-10,530);
  let result;
  if(end==='timeout'){
   for(let now=550;now<1510;now+=20)assert.equal(m.feed(0,-10,now).kind,'preview');
   result=m.tick(1510);
  }else result=end==='tick'?m.tick(690):m.feed(0,0,690);
  assert.equal(result.kind,'throw');assert.equal(result.speed,500);
  assert.equal(m.tick(2000),'spent');assert.equal(m.feed(0,-100,2010),null);
 }
});
test('opening reports, sub-16ms windows and tiny jitters cannot launch',()=>{
 for(const report of [1,800]){
  const m=ready({releaseGate:20});m.feed(0,-report,510);m.feed(0,-report,515);
  assert.equal(m.tick(675),'settling');
 }
 const m=ready();for(let i=0;i<8;i++)assert.equal(m.feed(0,-.5,510+i*20).kind,'preview');
 assert.equal(m.tick(810),'settling');
 const opening=ready();opening.feed(0,-800,510);
 const preview=opening.feed(0,-8,530);assert.equal(preview.kind,'throw');assert.equal(preview.speed,400);
});

test('P1 eight-button play map, menu aliases and Start/Select',()=>{
 const c=settings();
 assert.deepEqual(Array.from({length:8},(_,i)=>p1Action(i,c)),['KeyH','KeyF','KeyR','NudgeLeft','NudgeRight','KeyR','ToggleKeys','ToggleKeys']);
 assert.deepEqual(Array.from({length:8},(_,i)=>p1Action(i,c,true)),['Enter','Escape','KeyR','Enter','Escape','KeyR','ToggleKeys','ToggleKeys']);
 assert.equal(PLACEMENT_STEP,.05);
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
    assert.equal(c.powerSpeed,6048);
   }
  }
  assert.equal(migrate().releaseGate,400);assert.equal(migrate().spin,1);
 }
 // Current v9 profiles can explicitly disable spin or choose a short gate.
 assert.equal(settings({spin:0}).spin,0);
 const m=ready({releaseGate:200});
 assert.equal(m.feed(0,-100,510).kind,'preview');
 const shot=m.feed(0,-100,530);assert.equal(shot.kind,'throw');assert.equal(shot.power,5000/6048);
});

test('v9 through v5 factory speeds migrate once, custom speeds survive',()=>{
 for(const powerSpeed of [undefined,null,1050,1650,2100,3300,4000,5600,6500,7560,2000]){
  const current=profileSettings({powerSpeed});assert.equal(current.powerSpeed,powerSpeed===2000?2000:6048);
  const c=previousSettings({powerSpeed,sensitivity:1.5,shotKey:'Pad1Button3'});
  assert.equal(c.powerSpeed,powerSpeed===2000?2000:6048);
  assert.equal(c.sensitivity,1.5);assert.equal(c.shotKey,'Pad1Button3');
  assert.equal(legacySettings({powerSpeed}).powerSpeed,6048);
 }
 assert.equal(legacySettings({powerSpeed:400}).powerSpeed,6048);
 assert.equal(settings({powerSpeed:1050}).powerSpeed,1050);
});

test('the sliding window sees a late flick without averaging the whole stroke',()=>{
 const m=ready({releaseGate:1200});let result;
 for(let i=0;i<=20;i++)result=m.feed(0,-8,510+i*20);
 assert(Math.abs(result.speed-400)<1e-9);
 for(let i=21;i<=25;i++)result=m.feed(0,-33,510+i*20);
 assert.equal(result.kind,'preview');assert.equal(result.speed,1650);assert.equal(result.power,1650/6048);
 const shot=m.tick(1170);assert.equal(shot.kind,'throw');assert.equal(shot.speed,1650);
});
test('irregular hardware report timestamps interpolate the window boundary',()=>{
 const m=ready({releaseGate:1200});let now=510;
 m.feed(0,-999,now); // Opening count must never influence measured speed.
 let result;
 for(const dt of [17,31,23,19,29]){now+=dt;result=m.feed(0,-825*dt/1000,now);}
 assert.equal(result.kind,'preview');assert(Math.abs(result.speed-825)<1e-9);
 assert(Math.abs(result.power-825/6048)<1e-12);
});
test('idle-release power and one-shot behavior are independent of render cadence',()=>{
 const shots=[];
 for(const hz of [30,60,120]){
  const m=ready();m.feed(0,-33,510);m.feed(0,-33,530);
  const released=[];
  for(let frame=540;frame<900;frame+=1000/hz){const result=m.tick(frame);if(result?.kind==='throw')released.push(result);}
  assert.equal(released.length,1);shots.push(released[0]);
 }
 assert.deepEqual(shots[0],shots[1]);assert.deepEqual(shots[1],shots[2]);
});

test('sideways rolls preserve backswing spin and abort incomplete forward strokes',()=>{
 const back=ready();back.feed(-20,20,510);
 for(let i=0;i<10;i++)assert.equal(back.feed(20,0,520+i*10),null);
 assert.equal(back.phase,'backswing');
 const shot=stroke(back,4,10,620);assert.equal(shot.spin,-1);assert(shot.aim>0);
 const m=ready();assert.equal(m.feed(0,-10,510).kind,'preview');
 assert.equal(m.feed(20,0,520),null);assert.equal(m.phase,'ready');
 assert.equal(m.preview,null);assert.equal(m.forwardY,0);assert.equal(m.hasSample,false);
 for(let i=0;i<20;i++)assert.equal(m.feed(-20,0,530+i*10),null);
 assert.equal(m.tick(1000),'ready');
});
