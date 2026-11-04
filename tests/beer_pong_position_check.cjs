const {chromium}=require('playwright-core');
const assert=require('node:assert/strict');
const path=require('node:path');
const BASE=process.env.BASE_URL||'http://127.0.0.1:8012';
(async()=>{
 const browser=await chromium.launch();
 try{
  const context=await browser.newContext({viewport:{width:1280,height:800},hasTouch:true}),page=await context.newPage(),errors=[];
  page.on('pageerror',e=>errors.push(e.message));await page.addInitScript(()=>localStorage.setItem('tailgate-beer-pong-help-seen','1'));
  await page.goto(BASE+'/beer-pong/');await page.waitForFunction(()=>!!window.__pong);
  await page.locator('[data-mode=practice]').click();const slider=page.getByRole('slider',{name:'Ball starting position'});
  for(const [width,height]of [[1280,800],[390,844],[844,390]]){
   await page.setViewportSize({width,height});
   for(const key of ['Home','End']){
    await slider.focus();await slider.press(key);await page.evaluate(()=>new Promise(r=>requestAnimationFrame(()=>requestAnimationFrame(r))));
    const view=await page.evaluate(()=>{const st=__pong.state,p=__pong._screen(...st.held.p),meter=document.querySelector('#meter').getBoundingClientRect(),label=document.querySelector('#meter-target span').getBoundingClientRect(),control=document.querySelector('#ball-position-control').getBoundingClientRect();return {st,p,meter:{left:meter.left,right:meter.right,top:meter.top,bottom:meter.bottom},label:{left:label.left,right:label.right},control:{top:control.top,bottom:control.bottom}};});
    assert.equal(view.st.positions[0],key==='Home'?-.28:.28);assert.equal(view.st.held.p[0],view.st.positions[0]);
    assert(view.p.x>15&&view.p.x<width-15);assert.equal(view.st.shots[0],0,'Moving the slider must not throw');
    assert(view.meter.left>5&&view.meter.right<width-5&&view.meter.bottom<height-10);
    assert(view.label.left>0&&view.label.right<width,'Meter target label must remain visible');
    assert(view.meter.right<view.p.x-8||view.meter.left>view.p.x+8,'Meter must not cover the moved ball');
    assert(view.control.top>0&&view.control.bottom<height);
    if(process.env.SHOTS)await page.screenshot({path:path.join(process.env.SHOTS,`beer-pong-position-${width}-${key}.png`)});
   }
  }
  // Touch-drag the thumb: the ball moves without starting a flick throw.
  await page.setViewportSize({width:390,height:844});await slider.focus();await slider.press('Home');
  const box=await slider.boundingBox(),cdp=await context.newCDPSession(page),y=box.y+box.height/2;
  await cdp.send('Input.dispatchTouchEvent',{type:'touchStart',touchPoints:[{x:box.x+12,y}]});
  await cdp.send('Input.dispatchTouchEvent',{type:'touchMove',touchPoints:[{x:box.x+box.width-12,y}]});
  await cdp.send('Input.dispatchTouchEvent',{type:'touchEnd',touchPoints:[]});
  assert((await page.evaluate(()=>__pong.state.positions[0]))>.22);assert.equal(await page.evaluate(()=>__pong.state.shots[0]),0);
  // A real shot from the left position hits a left cup and starts at the displayed ball.
  await page.setViewportSize({width:1280,height:800});await slider.focus();await slider.press('Home');
  const shot=await page.evaluate(()=>{const held=__pong.state.held.p;__pong.throwWith(__pong._powerFor(16),0);return {held,p:__pong.state.ball.p};});
  assert.deepEqual(shot.p,shot.held);assert.equal(shot.p[0],-.28);assert.equal(await slider.isEnabled(),false);
  await page.waitForFunction(()=>__pong.state.phase==='aim');assert.equal(await page.evaluate(()=>__pong.state.cups[16].alive),false,'Actual flight must use the moved release point');
  await page.locator('#refill').click();assert.equal(await page.evaluate(()=>__pong.state.positions[0]),-.28,'Refill must preserve position');
  // Each local player keeps an independent, screen-relative starting position.
  await page.evaluate(()=>__pong.startMatch('two'));await page.locator('#ready').click();await slider.focus();await slider.press('Home');
  await page.evaluate(()=>__pong._dropAt(10));await page.waitForFunction(()=>__pong.state.phase==='handoff');await page.locator('#ready').click();
  assert.equal(await page.evaluate(()=>__pong.state.positions[1]),0);await slider.focus();await slider.press('End');
  const blue=await page.evaluate(()=>({held:__pong.state.held.p,point:__pong._screen(...__pong.state.held.p)}));
  assert.equal(blue.held[0],-.28);assert(blue.point.x>640,'Blue right must remain screen-right after the camera flip');
  await page.evaluate(()=>__pong._dropAt(0));await page.waitForFunction(()=>__pong.state.phase==='handoff');await page.locator('#ready').click();
  assert.equal(await page.evaluate(()=>__pong.state.positions[0]),-.28);assert.equal(await slider.inputValue(),'-100');
  await page.evaluate(()=>__pong.startMatch('cpu'));assert.deepEqual(await page.evaluate(()=>__pong.state.positions),[0,0]);
  await page.evaluate(()=>__pong._dropAt(10));await page.waitForFunction(()=>__pong.state.phase==='cpu');
  assert.equal(await slider.isVisible(),false,'CPU turn must not expose the human positioning control');
  assert.deepEqual(errors,[]);console.log('Ball positioning: keyboard/touch, screen bounds, meter following, actual offset scoring and per-player persistence passed.');
 }finally{await browser.close();}
})().catch(e=>{console.error(e);process.exitCode=1;});
