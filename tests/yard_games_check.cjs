const {chromium}=require('playwright-core');const assert=require('node:assert/strict');
const BASE=process.env.BASE_URL||'http://127.0.0.1:8012';
(async()=>{const browser=await chromium.launch();try{
 const context=await browser.newContext({viewport:{width:1280,height:800},hasTouch:true}),page=await context.newPage(),errors=[];page.setDefaultTimeout(15000);page.on('pageerror',e=>errors.push(e.message));
 await page.addInitScript(()=>{for(const g of ['ladder-golf','washers','lawn-darts'])localStorage.setItem(`tailgate-${g}-help-seen`,'1')});
 for(const kind of ['ladder-golf','washers','lawn-darts']){
  await page.goto(BASE+'/'+kind+'/');await page.waitForFunction(()=>!!window.__yard);await page.locator('[data-mode=practice]').click();assert.equal(await page.locator('[role=meter]').count(),0);
  const y=kind==='ladder-golf'?.67:kind==='washers'?.22:.04;
  const target=await page.evaluate(y=>({p:__yard.powerFor(y),probe:__yard._probe(__yard.powerFor(y))}),y);assert.equal(target.probe.value,kind==='ladder-golf'?2:3,kind+' must have a reachable scoring target');
  if(kind==='ladder-golf')for(let i=0;i<3;i++)assert.equal(await page.evaluate(y=>__yard._probe(__yard.powerFor(y)).rung,[.34,.67,1][i]),i);
  async function reset(){await page.evaluate(()=>__yard.startMatch('practice'));}
  async function mousePull(dx,dy,hold=0){const p=await page.evaluate(()=>__yard.screen());await page.mouse.move(p.x,p.y);await page.mouse.down();await page.mouse.move(p.x+dx,p.y+dy,{steps:5});if(hold)await page.waitForTimeout(hold);await page.mouse.up();return page.evaluate(()=>__yard.state);}
  let fast=await mousePull(5,60);assert(fast.flying);await reset();const slow=await mousePull(5,60,500);assert.equal(fast.power,slow.power);assert.equal(fast.flying.v[0],slow.flying.v[0]);assert.equal(fast.flying.v[2],slow.flying.v[2]);
  await reset();let p=await page.evaluate(()=>__yard.screen());await page.mouse.move(p.x,p.y);await page.mouse.down();await page.mouse.move(p.x,p.y+60);await page.mouse.move(p.x,p.y);await page.mouse.up();assert.equal(await page.evaluate(()=>__yard.state.flying),null);
  await mousePull(0,-80);assert.equal(await page.evaluate(()=>__yard.state.flying),null);
  // Direct positioning must change the real starting point, with no throw on a lateral release.
  p=await page.evaluate(()=>__yard.screen());await page.mouse.move(p.x,p.y);await page.mouse.down();await page.mouse.move(p.x+160,p.y,{steps:8});await page.mouse.up();assert((await page.evaluate(()=>__yard.state.positions[0]))>0);assert.equal(await page.evaluate(()=>__yard.state.flying),null);
  const origin=await page.evaluate(()=>{const held=__yard.state.held;__yard.throwWith(.3);return {held,launch:__yard.state.flying.p}});assert.deepEqual(origin.launch,origin.held);
  for(const [width,height]of [[1280,800],[390,844],[844,390]]){
   await page.setViewportSize({width,height});await reset();await page.evaluate(()=>new Promise(r=>requestAnimationFrame(()=>requestAnimationFrame(r))));p=await page.evaluate(()=>__yard.screen());assert(p.x>30&&p.x<width-30&&p.y>height*.4&&p.y<height*.88,JSON.stringify({kind,width,height,p}));
   const cdp=await context.newCDPSession(page);await cdp.send('Input.dispatchTouchEvent',{type:'touchStart',touchPoints:[p]});await cdp.send('Input.dispatchTouchEvent',{type:'touchMove',touchPoints:[{x:p.x-8,y:Math.min(height-24,p.y+50)}]});await cdp.send('Input.dispatchTouchEvent',{type:'touchEnd',touchPoints:[]});assert(await page.evaluate(()=>!!__yard.state.flying));assert(await page.locator('#arrow').evaluate(e=>+e.getAttribute('x2')>+e.getAttribute('x1')));assert.equal(await page.locator('#shot-feedback,#feedback-power,#feedback-tip').count(),0);
   if(process.env.SHOTS)await page.screenshot({path:process.env.SHOTS+`/${kind}-feedback-${width}.png`});
  }
  await page.setViewportSize({width:1280,height:800});await reset();await page.evaluate(y=>__yard.throwWith(__yard.powerFor(y)),y);await page.waitForFunction(()=>__yard.state.phase==='aim');assert.equal(await page.evaluate(()=>__yard.state.stats.throws),1);assert((await page.evaluate(()=>__yard.state.stats.points))>0);
  if(kind==='ladder-golf'){await page.evaluate(()=>__yard.throwWith(__yard.powerFor(.67)));await page.waitForFunction(()=>__yard.state.phase==='aim');assert.equal(await page.evaluate(()=>__yard.state.shots.filter(s=>s.rung===1&&s.alive).length),1,'A second physical hit can knock the earlier bola off');}
  const totals=await page.evaluate(()=>__yard.state.stats);await page.locator('#clear').click();assert.equal(await page.evaluate(()=>__yard.state.shots.length),0);assert.deepEqual(await page.evaluate(()=>__yard.state.stats),totals);
  // A paused physical throw freezes and resumes.
  await page.evaluate(()=>__yard.throwWith(.3));await page.locator('#pause-button').click();const frozen=await page.evaluate(()=>__yard.state.flying);await page.waitForTimeout(250);assert.deepEqual(await page.evaluate(()=>__yard.state.flying),frozen);await page.keyboard.press('Escape');
  // CPU physically throws and gives the human another turn (three bolas in ladder).
  await page.evaluate(()=>__yard.startMatch('cpu'));const count=kind==='lawn-darts'?2:3;
  for(let i=0;i<(kind==='ladder-golf'?3:1);i++){await page.evaluate(()=>__yard.throwWith(.01));await page.waitForFunction(()=>__yard.state.phase!=='flight');if(i<2&&kind==='ladder-golf')await page.waitForFunction(()=>__yard.state.phase==='aim');}
  if(kind==='ladder-golf'){await page.waitForFunction(()=>__yard.state.phase==='round',null,{timeout:30000});assert.equal(await page.evaluate(()=>__yard.state.counts[1]),3);await page.locator('#next').click();}
  else{await page.waitForFunction(()=>__yard.state.turn===0&&__yard.state.phase==='aim',null,{timeout:20000});assert.equal(await page.evaluate(()=>__yard.state.counts[1]),1);}
  // Complete a real local-player winning round, leaving the opponent short of the target.
  await page.evaluate(()=>__yard.startMatch('two'));await page.locator('#ready').click();await page.evaluate(kind=>__yard._setScores(kind==='ladder-golf'?15:kind==='washers'?12:15,0),kind);
  for(let i=0;i<count*2;i++){
   if(await page.locator('#handoff').isVisible())await page.locator('#ready').click();
   await page.waitForFunction(()=>__yard.state.phase==='aim');
   await page.evaluate(({kind,y})=>{const s=__yard.state;const p=s.turn===0?__yard.powerFor(kind==='ladder-golf'?[.34,.67,1][s.counts[0]]:y):.01;__yard.throwWith(p)}, {kind,y});
   await page.waitForFunction(()=>__yard.state.phase!=='flight');
   if(i<count*2-1)await page.waitForFunction(()=>['aim','handoff'].includes(__yard.state.phase));else await page.waitForFunction(()=>__yard.state.phase==='over');
  }
  assert.equal(await page.evaluate(()=>__yard.state.winner),0);assert.equal(await page.evaluate(()=>__yard.state.scores[0]),21);assert(await page.locator('#round-result').isVisible());await page.locator('#result-menu').click();await page.getByRole('link',{name:'ALL BACKYARD GAMES'}).click();assert.equal(await page.locator('.game-card').count(),5);
  console.log(kind+': native controls, reachability, real scoring, practice clear, pause, CPU, handoffs and complete winning round passed.');
 }
 assert.deepEqual(errors,[]);
}finally{await browser.close();}})().catch(e=>{console.error(e);process.exitCode=1});
