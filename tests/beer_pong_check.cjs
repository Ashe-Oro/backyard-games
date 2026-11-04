const {chromium}=require('playwright-core');
const assert=require('node:assert/strict');
const path=require('node:path');
const BASE=process.env.BASE_URL||'http://127.0.0.1:8012';
(async()=>{
 const browser=await chromium.launch({headless:true});
 try{
  const page=await browser.newPage({viewport:{width:1440,height:900}}),errors=[];
  page.on('pageerror',e=>errors.push(e.message));page.setDefaultTimeout(12000);
  await page.goto(BASE+'/beer-pong/');await page.waitForFunction(()=>!!window.__pong);
  assert(await page.locator('#help').isVisible());await page.locator('#help-done').click();
  await page.reload();await page.waitForFunction(()=>!!window.__pong);assert.equal(await page.locator('#help').isVisible(),false);
  assert.equal(await page.locator('[data-mode]').count(),3);
  if(process.env.SHOTS)await page.screenshot({path:path.join(process.env.SHOTS,'beer-pong-menu.png')});
  await page.locator('[data-mode=practice]').click();
  await page.waitForFunction(()=>__pong.state.held.visible&&__pong.state.phase==='aim');
  assert.deepEqual(await page.evaluate(()=>__pong.state.remaining),[10,10]);
  const probes=await page.evaluate(()=>__pong.state.cups.filter(c=>c.team===1).map(c=>({id:c.id,result:__pong._probe(__pong._powerFor(c.id),Math.tan(Math.atan2(c.x,Math.abs(c.z-.55))/.65))})));
  for(const probe of probes){assert.equal(probe.result.state,'sink');assert.equal(probe.result.cup,probe.id,'Each cup must be individually reachable');}
  const contacts=await page.evaluate(()=>{
   let rim=false,table=false;
   for(let p=.18;p<.8;p+=.025)for(let slope=-.25;slope<=.25;slope+=.04){const b=__pong._probe(p,slope);rim ||= b.rimHits>0;table ||= b.deadReason==='table';if(rim&&table)return {rim,table};}
   return {rim,table};
  });assert(contacts.rim&&contacts.table,'Physics must support rim contact and dead table contact');
  const dead=await page.evaluate(()=>__pong._probe(.18,0));
  assert.equal(dead.state,'done');assert.equal(dead.deadReason,'table');assert.equal(dead.cup,null);
  assert.deepEqual(dead.v,[0,0,0]);assert.equal(dead.bounces,0,'Table contact cannot rebound or score');
  for(const [width,height]of [[1440,900],[1280,800],[390,844],[456,872],[844,390]]){
   await page.setViewportSize({width,height});await page.waitForTimeout(120);
   const view=await page.evaluate(()=>{
    const st=__pong.state,ball=__pong._screen(...st.held.p);
    return {ball,cups:st.cups.filter(c=>c.enabled&&c.alive).map(c=>__pong._screen(c.x,.93,c.z))};
   });
   assert(view.ball.x>20&&view.ball.x<width-20&&view.ball.y>height*.5&&view.ball.y<height*.85,JSON.stringify({width,height,view}));
   assert.equal(await page.locator('[role=meter]').count(),0);
   for(const c of view.cups)assert(c.x>10&&c.x<width-10&&c.y>80&&c.y<height*.7,JSON.stringify({width,height,c}));
   if(process.env.SHOTS)await page.screenshot({path:path.join(process.env.SHOTS,`beer-pong-practice-${width}.png`)});
  }
  await page.setViewportSize({width:1280,height:800});
  const launch=await page.evaluate(()=>{const held=__pong.state.held.p,power=__pong._powerFor(10);__pong.throwWith(power,0);return {held,origin:__pong.state.ball.p};});
  assert.deepEqual(launch.origin,launch.held,'Held and flying ball must share the same origin');
  assert.equal(await page.locator('#refill').isEnabled(),false);
  await page.waitForFunction(()=>__pong.state.phase==='aim'&&__pong.state.hits[0]===1);
  assert.deepEqual(await page.evaluate(()=>__pong.state.remaining),[10,9]);
  assert.equal(await page.evaluate(()=>__pong._dropAt(10)),false,'A removed cup must not score twice');
  await page.locator('#refill').click();
  assert.equal(await page.evaluate(()=>__pong.state.hits[0]),1);assert.equal(await page.evaluate(()=>__pong.state.remaining[1]),10);
  await page.evaluate(()=>__pong.throwWith(.2,0));await page.locator('#pause-button').click();
  const frozen=await page.evaluate(()=>__pong.state.ball);
  await page.waitForTimeout(500);assert.deepEqual(await page.evaluate(()=>__pong.state.ball),frozen,'Pause must freeze ball physics');
  assert.equal(await page.evaluate(()=>__pong.throwWith(.5)),false);
  await page.keyboard.press('Escape');await page.waitForFunction(()=>__pong.state.phase==='aim');
  assert.equal(await page.evaluate(()=>__pong.state.hits[0]),1,'A table-contact shot must not score');
  assert.equal(await page.evaluate(()=>__pong.state.remaining[1]),10,'Dead ball must leave the rack intact');
  await page.locator('#refill').click();
  // Complete the rack through the same sink detector used by normal shots.
  for(let id=10;id<20;id++){
   assert(await page.evaluate(id=>__pong._dropAt(id),id));
   await page.waitForFunction(id=>!__pong.state.cups[id].alive,id);
   if(id<19)await page.waitForFunction(()=>__pong.state.phase==='aim');
  }
  assert(await page.locator('#result').isVisible());assert.equal(await page.evaluate(()=>__pong.state.remaining[1]),0);
  const practiceHits=await page.evaluate(()=>__pong.state.hits[0]);await page.locator('#play-again').click();
  assert.equal(await page.evaluate(()=>__pong.state.remaining[1]),10);assert.equal(await page.evaluate(()=>__pong.state.hits[0]),practiceHits);
  console.log('Practice: reachable cups, scoring, refill, full rack, physics, pause and responsive framing passed.');
  // CPU mode actually launches from the opposing end and returns the turn.
  await page.evaluate(()=>__pong.startMatch('cpu'));
  await page.waitForFunction(()=>__pong.state.held.visible);
  if(process.env.SHOTS)await page.screenshot({path:path.join(process.env.SHOTS,'beer-pong-cpu.png')});
  await page.evaluate(()=>__pong._dropAt(10));
  await page.waitForFunction(()=>__pong.state.phase==='cpu');await page.locator('#pause-button').click();
  await page.waitForTimeout(1400);assert.equal(await page.evaluate(()=>__pong.state.phase),'cpu');assert.equal(await page.evaluate(()=>__pong.state.ball),null);
  await page.locator('#resume').click();await page.waitForFunction(()=>__pong.state.phase==='flight'&&__pong.state.ball?.team===1);
  assert.equal(await page.evaluate(()=>__pong.state.camera.position[2]>0),true,'CPU mode must keep the human viewpoint');
  await page.waitForFunction(()=>__pong.state.phase==='aim'&&__pong.state.turn===0);
  console.log('CPU: opposing launch, pause and automatic turn return passed.');
  // Same-screen multiplayer: handoffs, mirrored view, then a complete win.
  await page.evaluate(()=>__pong.startMatch('two'));
  for(let shot=0;shot<19;shot++){
   assert(await page.locator('#handoff').isVisible());
   const turn=shot%2;assert.equal(await page.evaluate(()=>__pong.state.turn),turn);
   assert.equal(await page.evaluate(()=>__pong.state.camera.position[2]>0),turn===0);
   await page.locator('#ready').click();
   if(process.env.SHOTS&&shot===1){await page.waitForTimeout(100);await page.screenshot({path:path.join(process.env.SHOTS,'beer-pong-two-player-blue.png')});}
   const id=turn===0?10+Math.floor(shot/2):Math.floor(shot/2);
   assert(await page.evaluate(id=>__pong._dropAt(id),id));
   if(shot<18)await page.waitForFunction(()=>__pong.state.phase==='handoff');
   else await page.waitForFunction(()=>__pong.state.phase==='over');
  }
  assert.equal(await page.evaluate(()=>__pong.state.winner),0);assert.deepEqual(await page.evaluate(()=>__pong.state.remaining),[1,0]);
  if(process.env.SHOTS)await page.screenshot({path:path.join(process.env.SHOTS,'beer-pong-win.png')});
  assert(await page.locator('#result').isVisible());await page.locator('#result-menu').click();assert.equal(await page.evaluate(()=>__pong.state.phase),'menu');
  await page.goto(BASE+'/cornhole/');await page.waitForFunction(()=>!!window.__game);
  const help=page.locator('#help-dialog');if(await help.isVisible())await page.locator('#help-done').click();
  await page.getByRole('link',{name:'ALL TAILGATE GAMES'}).click();await page.getByRole('link',{name:'Play Beer Pong'}).click();await page.waitForFunction(()=>!!window.__pong);
  assert.deepEqual(errors,[]);console.log('Two Player: handoffs, mirrored cameras, 19-shot complete win, and Cornhole navigation passed.');
 }finally{await browser.close();}
})().catch(e=>{console.error(e);process.exitCode=1;});
