const {chromium}=require('playwright-core');
const assert=require('node:assert/strict');
const fs=require('node:fs');
const path=require('node:path');
(async()=>{
 const browser=await chromium.launch({headless:true});
 try{
  const page=await browser.newPage({viewport:{width:1280,height:800}}),errors=[];
  page.on('pageerror',e=>errors.push(e.message));
  const url='http://127.0.0.1:8012/';
  const source=fs.readFileSync(path.join(__dirname,'../index.html'),'utf8').replace('window.__game = {','window.__setPower=setMeter; window.__game = {');
  await page.route(url,r=>r.fulfill({body:source,contentType:'text/html'}));
  await page.goto(url);await page.waitForFunction(()=>!!window.__game);
  assert(await page.locator('#help-dialog').isVisible(),'First visit needs instructions');
  for(const [width,height] of [[390,844],[844,390]]){
   await page.setViewportSize({width,height});
   const box=await page.locator('#help-done').boundingBox();
   assert(box.y>=0 && box.y+box.height<height,'First-time instructions must fit mobile screens');
   if(process.env.SHOTS)await page.screenshot({path:path.join(process.env.SHOTS,`first-visit-help-${width}.png`)});
  }
  await page.setViewportSize({width:1280,height:800});
  if(process.env.SHOTS)await page.screenshot({path:path.join(process.env.SHOTS,'first-visit-help.png')});
  await page.locator('#help-done').click();await page.reload();await page.waitForFunction(()=>!!window.__game);
  assert.equal(await page.locator('#help-dialog').isVisible(),false,'Instructions must not repeat on reload');
  await page.evaluate(()=>__game.startMatch({practice:true,vs:'2p',first:0}));
  await page.waitForFunction(()=>__game.state.phase==='aim');
  for(const [width,height] of [[1280,800],[390,844],[844,390]]){
   await page.setViewportSize({width,height});
   await page.evaluate(()=>__setPower(__game.state.sweet.mid));
   assert.equal(await page.locator('#meter').getAttribute('data-zone'),'target');
   const layout=await page.evaluate(()=>{
    const b=document.querySelector('#pause-button').getBoundingClientRect(),t=document.querySelector('#t0').getBoundingClientRect();
    const sweet=document.querySelector('#sweet').getBoundingClientRect(),marker=document.querySelector('#power-marker').getBoundingClientRect();
    return {pause:[b.x,b.y,b.right,b.bottom],scoreY:t.y+54,target:[sweet.top,sweet.bottom],marker:(marker.top+marker.bottom)/2};
   });
   assert(layout.pause[0]>=0 && layout.pause[1]>=0 && layout.pause[3]<layout.scoreY+2,'Pause must fit above score');
   assert(layout.marker>=layout.target[0]-3 && layout.marker<=layout.target[1]+3,'Power marker must match green target');
   if(width<600 && await page.locator('#bag-handle').getAttribute('aria-expanded')==='true')await page.locator('#bag-handle').click();
   if(process.env.SHOTS){await page.waitForTimeout(800);await page.screenshot({path:path.join(process.env.SHOTS,`meter-ui-${width}.png`)});}
  }
  await page.setViewportSize({width:1280,height:800});
  await page.evaluate(()=>__game.setAim(0));
  await page.locator('#pause-button').hover();
  assert.equal(await page.evaluate(()=>__game.state.aim),0,'Hovering pause must not change aiming');
  await page.evaluate(()=>__game.throwWith(__game.state.sweet.mid));
  await page.locator('#pause-button').click();
  assert.equal(await page.evaluate(()=>__game.state.paused),true);
  const frozen=await page.evaluate(()=>__game.state);
  await page.waitForTimeout(500);
  assert.deepEqual(await page.evaluate(()=>__game.state.bags),frozen.bags,'Paused bag physics must freeze');
  assert.equal(await page.evaluate(()=>__game.throwWith(.5)),false,'Paused game must reject throws');
  if(process.env.SHOTS)await page.screenshot({path:path.join(process.env.SHOTS,'pause-menu.png')});
  await page.keyboard.press('Escape');
  assert.equal(await page.evaluate(()=>__game.state.paused),false,'Escape must resume');
  await page.waitForFunction(()=>__game.state.phase==='aim');
  // Pause the queued start of the CPU's turn, then ensure its timer resumes.
  await page.evaluate(()=>__game.startMatch({vs:'cpu',practice:false,first:1}));
  await page.locator('#pause-button').click();
  const phase=await page.evaluate(()=>__game.state.phase);
  await page.waitForTimeout(1300);
  assert.equal(await page.evaluate(()=>__game.state.phase),phase,'Turn delay must not run while paused');
  assert.equal(await page.evaluate(()=>__game.state.bags.length),0);
  await page.locator('#resume-game').click();
  await page.waitForFunction(()=>__game.state.phase==='cpu');
  await page.locator('#pause-button').click();
  const cpuFrozen=await page.evaluate(()=>__game.state);
  await page.waitForTimeout(1400);
  assert.deepEqual(await page.evaluate(()=>__game.state.bags),cpuFrozen.bags,'CPU must freeze during pause');
  await page.locator('#pause-menu').click();
  await page.waitForTimeout(1500);
  assert.equal(await page.evaluate(()=>__game.state.phase),'menu');
  assert.equal(await page.evaluate(()=>__game.state.bags.length),0,'Leaving to menu cancels queued game events');
  assert.equal(await page.locator('#help-dialog').isVisible(),false);
  assert.deepEqual(errors,[]);
  console.log('First-visit help, meter, pause physics/CPU/timers, resume, and menu passed.');
 }finally{await browser.close();}
})().catch(e=>{console.error(e);process.exitCode=1;});
