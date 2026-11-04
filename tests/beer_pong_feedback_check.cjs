const {chromium}=require('playwright-core');const assert=require('node:assert/strict');
const BASE=process.env.BASE_URL||'http://127.0.0.1:8012';
(async()=>{const browser=await chromium.launch();try{
 const page=await browser.newPage({viewport:{width:1280,height:800}}),errors=[];page.on('pageerror',e=>errors.push(e.message));
 await page.addInitScript(()=>localStorage.setItem('tailgate-beer-pong-help-seen','1'));await page.goto(BASE+'/beer-pong/');await page.waitForFunction(()=>!!window.__pong);await page.locator('[data-mode=practice]').click();assert.equal(await page.locator('[role=meter],#meter').count(),0);
 for(const [power,label,tip] of [[.05,'TOO WEAK','Pull farther'],[.95,'TOO STRONG','shorter pull']]){
  await page.evaluate(power=>{__pong.startMatch('practice');__pong.throwWith(power,0)},power);assert.equal(await page.locator('#feedback-power').textContent(),label);assert.equal(await page.evaluate(()=>__pong.state.ball.style),power<.45?'bounce':'arc');assert((await page.locator('#feedback-direction').textContent()).includes(power<.45?'BOUNCE':'ARC'));assert((await page.locator('#feedback-tip').textContent()).includes(tip));assert((await page.locator('#feedback-direction').textContent()).includes('STRAIGHT'));
  await page.waitForFunction(()=>__pong.state.phase==='aim');assert.equal(await page.locator('#feedback-power').textContent(),label,'Feedback remains available for the next attempt');
 }
 // Every precisely aimed cup reports good distance, including off-center targets.
 for(let id=10;id<20;id++){
  await page.evaluate(id=>{__pong.startMatch('practice');const cup=__pong.state.cups[id];__pong.throwWith(__pong._powerFor(id),__pong._slopeFor(cup.id))},id);
  assert.equal(await page.locator('#feedback-power').textContent(),'POWER IN RANGE');
 }
 for(const [slope,label]of [[-.3,'LEFT'],[.3,'RIGHT']]){await page.evaluate(slope=>{__pong.startMatch('practice');__pong.throwWith(.45,slope)},slope);assert((await page.locator('#feedback-direction').textContent()).includes(label));}
 for(const [width,height]of [[1280,800],[390,844],[844,390]]){
  await page.setViewportSize({width,height});await page.evaluate(()=>__pong.startMatch('practice'));await page.evaluate(()=>new Promise(r=>requestAnimationFrame(()=>requestAnimationFrame(r))));
  // Real pull/release populates the same feedback and leaves an arrow after release.
  const p=await page.evaluate(()=>__pong._screen(...__pong.state.held.p));await page.mouse.move(p.x,p.y);await page.mouse.down();await page.mouse.move(p.x,p.y+30,{steps:5});assert.equal(await page.locator('#shot-feedback').isVisible(),false);await page.mouse.up();assert.equal(await page.locator('#shot-feedback').isVisible(),true);assert(await page.locator('#swipe-feedback').evaluate(e=>e.classList.contains('show')));
  const box=await page.locator('#shot-feedback').boundingBox();assert(box.x>=0&&box.y>=0&&box.x+box.width<=width&&box.y+box.height<height);
  if(process.env.SHOTS)await page.screenshot({path:process.env.SHOTS+`/beer-pong-feedback-${width}.png`});
 }
 await page.evaluate(()=>__pong.startMatch('two'));assert.equal(await page.locator('#shot-feedback').isVisible(),false);await page.locator('#ready').click();await page.evaluate(()=>__pong._dropAt(10));await page.waitForFunction(()=>__pong.state.phase==='handoff');await page.locator('#ready').click();await page.evaluate(()=>__pong.throwWith(.95,.3));assert((await page.locator('#feedback-direction').textContent()).includes('RIGHT'));assert.equal(await page.locator('#feedback-power').textContent(),'TOO STRONG');
 await page.evaluate(()=>__pong.startMatch('cpu'));assert.equal(await page.locator('#shot-feedback').isVisible(),false);await page.evaluate(()=>__pong.throwWith(.05));await page.waitForFunction(()=>__pong.state.turn===1);assert.equal(await page.locator('#shot-feedback').isVisible(),false);
 assert.deepEqual(errors,[]);console.log('Shot feedback: no meter, weak/strong tips, all ten cup distances, direction, real release, responsive bounds, player mirroring and CPU isolation passed.');
}finally{await browser.close();}})().catch(e=>{console.error(e);process.exitCode=1});
