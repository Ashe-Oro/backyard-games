import {test} from 'node:test';import assert from 'node:assert/strict';
import {Trackball,settings,legacySettings} from '../trackball.mjs';
function ready(config={}){const m=new Trackball({releaseGate:80,powerSpeed:2000,...config});m.tick(500);return m;}
function stroke(m, dx=0, interval=10, start=510, dy=8) {
 let shot;
 for(let i=0;i<30;i++){
  const event=m.feed(dx,-dy,start+i*interval);
  if(event?.kind==='throw'){shot=event;break;}
 }
 return shot;
}
test('settings bound speed and reject conflicting or malformed mappings',()=>{const c=settings({sensitivity:Infinity,powerSpeed:-100,releaseGate:9999,shotKey:'../../file',menuKey:'F2'});assert.equal(c.sensitivity,1);assert.equal(c.powerSpeed,400);assert.equal(c.releaseGate,400);assert.equal(c.shotKey,'KeyH');assert.equal(c.menuKey,'Escape');assert.equal(settings({powerSpeed:1e9}).powerSpeed,10000);assert(!('mode' in settings()));});
test('sideways placement and backward rolls cannot throw',()=>{
 const m=ready();
 for(let i=0;i<100;i++)assert.equal(m.feed(15,-2,510+i*10)?.kind,'position');
 for(let i=0;i<20;i++)assert.equal(m.feed(0,20,1510+i*10),null);
 assert.equal(m.phase,'ready');
});
test('forward-only throws aim left, straight, and right in the direction rolled',()=>{
 const [left,straight,right]=[-4,0,4].map(dx=>stroke(ready(),dx));
 assert(left.aim<0);
 assert.equal(straight.aim,0);
 assert.equal(left.aim,-right.aim);
 assert.equal(left.power,right.power);
});
test('forward preview uses the same direction and power as release',()=>{
 const m=ready();
 m.feed(3,-8,510);
 const preview=m.feed(3,-8,520);
 assert.equal(preview.kind,'preview');
 assert(preview.aim>0);
 for(let i=2;i<9;i++)m.feed(3,-8,510+i*10);
 const shot=m.feed(3,-8,600);
 assert.equal(shot.kind,'throw');
 assert.equal(shot.aim,preview.aim);
 assert.equal(shot.power,preview.power);
});
test('forward speed alone controls bounded power',()=>{
 const shots=[35,10,5,1].map(dt=>stroke(ready(),0,dt));
 for(let i=1;i<shots.length;i++)assert(shots[i].power>shots[i-1].power);
 assert.equal(stroke(ready({powerSpeed:400}),0,1).power,1);
});
test('idle before the forward roll does not reduce power',()=>{
 assert.equal(stroke(ready()).power,stroke(ready(),0,10,1600).power);
});
test('optional finish rotation does not change the aim of a forward stroke',()=>{
 function curved(spin){
  const m=ready({spin});
  let shot;
  for(let i=0;i<10;i++)shot=m.feed(i,-8,510+i*10);
  return shot;
 }
 assert.equal(curved(0).spin,0);
 assert(curved(1).spin>0);
 assert.equal(curved(0).aim,curved(1).aim);
});
test('one gesture makes exactly one shot; residual motion cannot throw',()=>{const m=ready();assert(stroke(m));for(let i=0;i<100;i++)assert.equal(m.feed(10,i%2?30:-30,1000+i*10),null);m.reset(2100);for(let t=2110;t<=2500;t+=10)assert.equal(m.feed(0,10,t),null);assert.equal(m.tick(2900),'settling');assert.equal(m.tick(2960),'ready');});
test('stale forward stroke, reset and malformed events cannot launch',()=>{const m=ready();m.feed(0,-40,510);m.tick(700);assert.equal(m.phase,'settling');assert.equal(m.feed(0,-200,710),null);for(const args of [[NaN,1,5000],[1,1,-1],[4000,0,5000]])assert.equal(m.feed(...args),null);m.reset(6000);assert.equal(m.preview,null);});
test('a timestamped gesture is independent of render tick frequency',()=>{const shots=[];for(const hz of [30,60,120]){const m=ready();let shot,event=0;for(let frame=500;frame<1100;frame+=1000/hz){while(event<30&&510+event*10<=frame){const i=event++,r=m.feed(i<18?1:2,i<18?10:-8,510+i*10);if(r?.kind==='throw')shot=r;}m.tick(frame);}shots.push(shot);}assert.deepEqual(shots[0],shots[1]);assert.deepEqual(shots[1],shots[2]);});
test('older distance and sensitive speed profiles gain headroom while keeping bindings',()=>{
 const c=legacySettings({pullRange:240,powerSpeed:2000,releaseGate:48,sensitivity:1.5,shotKey:'Pad1Button3'});
 assert.equal(c.powerSpeed,6500);
 assert.equal(c.releaseGate,220);
 assert.equal(c.sensitivity,1.5);
 assert.equal(c.shotKey,'Pad1Button3');
 assert.equal(legacySettings({powerSpeed:9000}).powerSpeed,9000);
 assert.equal(settings({powerSpeed:3000}).powerSpeed,3000);
});
test('a single oversized motion report cannot launch or imply full power',()=>{
 const m=new Trackball();
 m.tick(500);
 assert.equal(m.feed(0,-240,510).kind,'preview');
 assert.equal(m.preview.power,0);
 assert.equal(m.feed(0,-20,510).kind,'preview');
 assert.equal(m.feed(0,-20,540).kind,'throw');
 assert(m.preview.power<1);
});
test('drive profile waits for a sustained 220-count forward stroke',()=>{
 const m=new Trackball();
 m.tick(500);
 for(let i=0;i<10;i++)assert.notEqual(m.feed(0,-20,520+i*10)?.kind,'throw');
 assert.equal(m.feed(0,-20,620).kind,'throw');
});
test('stronger forward rolls have headroom before full power',()=>{
 const shots=[25,10,6,4,3].map(dt=>stroke(ready({releaseGate:220,powerSpeed:6500}),0,dt,510,20).power);
 for(let i=1;i<shots.length;i++)assert(shots[i]>shots[i-1]);
 assert(shots[2]<.55);
 assert(shots[3]>.75&&shots[3]<1);
 assert.equal(shots[4],1);
});
