const {chromium}=require('playwright-core');const assert=require('node:assert/strict');
const BASE=process.env.BASE_URL||'http://127.0.0.1:8012';
(async()=>{const browser=await chromium.launch();try{
 const context=await browser.newContext({viewport:{width:1280,height:800},hasTouch:true}),page=await context.newPage(),errors=[];page.on('pageerror',e=>errors.push(e.message));
 await page.addInitScript(()=>localStorage.setItem('tailgate-beer-pong-help-seen','1'));await page.goto(BASE+'/beer-pong/');await page.waitForFunction(()=>!!window.__pong);await page.locator('[data-mode=practice]').click();
 const origin=()=>page.evaluate(()=>new Promise(resolve=>requestAnimationFrame(()=>requestAnimationFrame(()=>resolve(__pong._screen(...__pong.state.held.p))))));
 const reset=()=>page.evaluate(()=>__pong.startMatch('practice'));
 async function pull(dx,dy,hold=0){const p=await origin();await page.mouse.move(p.x,p.y);await page.mouse.down();await page.mouse.move(p.x+dx,p.y+dy,{steps:5});if(hold)await page.waitForTimeout(hold);const power=await page.evaluate(()=>__pong.state.power);await page.mouse.up();return {power,state:await page.evaluate(()=>__pong.state)};}
 let fast=await pull(15,80);assert.equal(fast.state.shots[0],1);assert(fast.state.ball.v[0]<0,'Pull right aims left');await reset();let slow=await pull(15,80,700);assert.equal(slow.power,fast.power);assert.equal(slow.state.ball.v[0],fast.state.ball.v[0]);assert.equal(slow.state.ball.v[2],fast.state.ball.v[2]);
 await reset();const low=await pull(0,35);await reset();const high=await pull(0,100);assert(high.power>low.power);assert(Math.abs(high.state.ball.v[2])>Math.abs(low.state.ball.v[2]));
 // Returning to the grab point cancels; upward throws also work, but wheel input never fires.
 await reset();let p=await origin();await page.mouse.move(p.x,p.y);await page.mouse.down();await page.mouse.move(p.x,p.y+75);assert(await page.locator('#swipe-feedback').evaluate(e=>e.classList.contains('pulling')));await page.mouse.move(p.x,p.y);await page.mouse.up();assert.equal(await page.evaluate(()=>__pong.state.shots[0]),0);
 await pull(0,-100);assert.equal(await page.evaluate(()=>__pong.state.shots[0]),1);await reset();await page.mouse.wheel(0,-300);await page.waitForTimeout(150);assert.equal(await page.evaluate(()=>__pong.state.shots[0]),0);
 for(const [width,height]of [[390,844],[844,390],[1280,800]]){
  await reset();await page.setViewportSize({width,height});await page.evaluate(()=>new Promise(r=>requestAnimationFrame(()=>requestAnimationFrame(r))));p=await origin();
  const cdp=await context.newCDPSession(page);await cdp.send('Input.dispatchTouchEvent',{type:'touchStart',touchPoints:[{x:p.x,y:p.y}]});await cdp.send('Input.dispatchTouchEvent',{type:'touchMove',touchPoints:[{x:p.x-10,y:Math.min(height-25,p.y+60)}]});
  const power=await page.evaluate(()=>__pong.state.power);assert(power>0);if(process.env.SHOTS)await page.screenshot({path:process.env.SHOTS+`/beer-pong-pull-${width}.png`});
  await cdp.send('Input.dispatchTouchEvent',{type:'touchEnd',touchPoints:[]});let st=await page.evaluate(()=>__pong.state);assert.equal(st.shots[0],1);assert(st.ball.v[0]>0,'Pull left aims right');
 }
 // Pause/blur cancel an armed shot.
 await reset();p=await origin();await page.mouse.move(p.x,p.y);await page.mouse.down();await page.mouse.move(p.x,p.y+60);await page.evaluate(()=>window.dispatchEvent(new Event('blur')));await page.mouse.up();assert.equal(await page.evaluate(()=>__pong.state.shots[0]),0);
 await page.evaluate(()=>__pong.startMatch('two'));await page.locator('#ready').click();await page.evaluate(()=>__pong._dropAt(10));await page.waitForFunction(()=>__pong.state.phase==='handoff');await page.locator('#ready').click();const blue=await pull(-15,70);assert.equal(blue.state.shots[1],1);assert(blue.state.ball.v[0]<0,'Screen-right shot mirrors in blue view');
 assert.deepEqual(errors,[]);console.log('Pull/release: speed independence, distance power, opposite aim, cancel, upward throw, no wheel launch, native touch across layouts and mirrored blue throws passed.');
}finally{await browser.close();}})().catch(e=>{console.error(e);process.exitCode=1});
