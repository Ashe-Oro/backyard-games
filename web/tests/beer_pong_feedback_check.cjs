const {chromium}=require('playwright-core');const assert=require('node:assert/strict');
const BASE=process.env.BASE_URL||'http://127.0.0.1:8012';
(async()=>{const browser=await chromium.launch();try{
 const page=await browser.newPage(),errors=[];page.on('pageerror',e=>errors.push(e.message));await page.goto(BASE+'/beer-pong/');await page.waitForFunction(()=>!!__pong);
 for(const [width,height]of [[1280,800],[390,844],[844,390]]){
  await page.setViewportSize({width,height});await page.evaluate(()=>__pong.startMatch('practice'));await page.evaluate(()=>new Promise(r=>requestAnimationFrame(()=>requestAnimationFrame(r))));
  const p=await page.evaluate(()=>__pong._screen(...__pong.state.held.p));await page.mouse.move(p.x,p.y);await page.mouse.down();await page.mouse.move(p.x+8,p.y+35,{steps:5});await page.mouse.up();
  assert.equal(await page.locator('#shot-feedback,#feedback-power,#feedback-tip,[role=meter]').count(),0);assert(await page.locator('#swipe-feedback').evaluate(e=>e.classList.contains('show')),'Keep the throw-direction arrow');assert.equal(await page.evaluate(()=>__pong.state.shots[0]),1);await page.waitForFunction(()=>__pong.state.phase==='aim');
  assert(!(await page.locator('#hud').innerText()).match(/TOO STRONG|TOO WEAK|POWER IN RANGE|shorter pull|Pull farther/));
  if(process.env.SHOTS)await page.screenshot({path:process.env.SHOTS+`/pong-no-feedback-${width}.png`});
 }
 await page.evaluate(()=>__pong.startMatch('two'));await page.locator('#ready').click();await page.evaluate(()=>__pong._dropAt(10));await page.waitForFunction(()=>__pong.state.phase==='handoff');await page.locator('#ready').click();await page.evaluate(()=>__pong.throwWith(.95,.3));assert(await page.locator('#swipe-feedback').evaluate(e=>e.classList.contains('show')));assert.equal(await page.locator('#shot-feedback').count(),0);
 await page.evaluate(()=>__pong.startMatch('cpu'));await page.evaluate(()=>__pong.throwWith(.95,.5));await page.waitForFunction(()=>__pong.state.turn===1);assert.equal(await page.locator('#shot-feedback').count(),0);assert.deepEqual(errors,[]);console.log('No post-throw text box; real pointer throws retain direction arrows across layouts and mirrored players; CPU turn transitions remain clean.');
}finally{await browser.close()}})().catch(e=>{console.error(e);process.exitCode=1});
