const {chromium}=require('playwright-core');
const assert=require('node:assert/strict');
const BASE=process.env.BASE_URL||'http://127.0.0.1:8012';
(async()=>{
 const browser=await chromium.launch();
 try{
  const context=await browser.newContext({viewport:{width:1280,height:800},hasTouch:true}),page=await context.newPage(),errors=[];
  page.on('pageerror',e=>errors.push(e.message));await page.addInitScript(()=>localStorage.setItem('tailgate-beer-pong-help-seen','1'));
  await page.goto(BASE+'/beer-pong/');await page.waitForFunction(()=>!!window.__pong);
  await page.locator('[data-mode=practice]').click();assert.equal(await page.locator('input[type=range]').count(),0);
  const point=()=>page.evaluate(()=>__pong._screen(...__pong.state.held.p));
  async function moveMouse(sign){const p=await point();await page.mouse.move(p.x,p.y);await page.mouse.down();await page.mouse.move(p.x+sign*500,p.y,{steps:15});await page.mouse.up();}
  for(const [width,height]of [[1280,800],[390,844],[844,390]]){
   await page.setViewportSize({width,height});await page.evaluate(()=>new Promise(r=>requestAnimationFrame(()=>requestAnimationFrame(r))));
   for(const sign of [-1,1]){await moveMouse(sign);const s=await page.evaluate(()=>__pong.state);assert.equal(s.positions[0],sign*.28);assert.equal(s.held.p[0],sign*.28);assert.equal(s.shots[0],0);assert.equal(s.phase,'aim');}
  }
  // Native touch grab and lateral drag must position without launching.
  await page.setViewportSize({width:390,height:844});const cdp=await context.newCDPSession(page),p=await point();
  await cdp.send('Input.dispatchTouchEvent',{type:'touchStart',touchPoints:[{x:p.x,y:p.y}]});
  for(let i=1;i<=10;i++)await cdp.send('Input.dispatchTouchEvent',{type:'touchMove',touchPoints:[{x:p.x+(12-p.x)*i/10,y:p.y}]});
  await cdp.send('Input.dispatchTouchEvent',{type:'touchEnd',touchPoints:[]});
  assert.equal(await page.evaluate(()=>__pong.state.positions[0]),-.28);assert.equal(await page.evaluate(()=>__pong.state.shots[0]),0);
  // A new downward pull from the positioned ball launches at its visible origin.
  await page.setViewportSize({width:1280,height:800});const start=await point();
  await page.mouse.move(start.x,start.y);await page.mouse.down();await page.mouse.move(start.x,start.y+30);await page.waitForTimeout(20);await page.mouse.move(start.x,start.y+100);await page.mouse.up();
  let s=await page.evaluate(()=>__pong.state);assert.equal(s.shots[0],1);assert.equal(s.ball.p[0],-.28);
  await page.waitForFunction(()=>__pong.state.phase==='aim');await page.locator('#refill').click();assert.equal(await page.evaluate(()=>__pong.state.positions[0]),-.28);
  // Changing a lateral grab into an downward pull also throws without sideways velocity.
  await page.evaluate(()=>__pong.startMatch('practice'));const q=await point();await page.mouse.move(q.x,q.y);await page.mouse.down();await page.mouse.move(q.x+50,q.y,{steps:5});
  const held=await page.evaluate(()=>__pong.state.held.p);await page.mouse.move(q.x+50,q.y+25);await page.waitForTimeout(20);await page.mouse.move(q.x+50,q.y+100);await page.mouse.up();
  s=await page.evaluate(()=>__pong.state);assert.equal(s.shots[0],1);assert.equal(s.ball.p[0],held[0]);assert(Math.abs(s.ball.v[0])<1e-8);
  // Blue-player dragging stays screen-relative and each player retains a position.
  await page.evaluate(()=>__pong.startMatch('two'));await page.locator('#ready').click();await moveMouse(-1);
  await page.evaluate(()=>__pong._dropAt(10));await page.waitForFunction(()=>__pong.state.phase==='handoff');await page.locator('#ready').click();await moveMouse(1);
  s=await page.evaluate(()=>__pong.state);assert.equal(s.positions[1],.28);assert.equal(s.held.p[0],-.28);assert((await point()).x>640);
  await page.evaluate(()=>__pong._dropAt(0));await page.waitForFunction(()=>__pong.state.phase==='handoff');await page.locator('#ready').click();assert.equal(await page.evaluate(()=>__pong.state.positions[0]),-.28);
  assert.deepEqual(errors,[]);console.log('Direct ball grab: desktop/mobile bounds, touch, no accidental throws, downward pulls, moved origins and mirrored player persistence passed.');
 }finally{await browser.close();}
})().catch(e=>{console.error(e);process.exitCode=1;});
