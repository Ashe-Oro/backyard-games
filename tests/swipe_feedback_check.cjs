const {chromium}=require('playwright-core');const assert=require('node:assert/strict');
const BASE=process.env.BASE_URL||'http://127.0.0.1:8012';
(async()=>{const browser=await chromium.launch();try{
 const context=await browser.newContext({viewport:{width:1280,height:800},hasTouch:true}),page=await context.newPage(),errors=[];page.on('pageerror',e=>errors.push(e.message));
 await page.addInitScript(()=>localStorage.setItem('backyard-cornhole-help-seen','1'));await page.goto(BASE+'/cornhole/');await page.waitForFunction(()=>!!window.__game);
 async function start(opts={}){await page.evaluate(opts=>__game.startMatch({practice:true,vs:'2p',first:0,...opts}),opts);await page.waitForFunction(()=>__game.state.phase==='aim');}
 const origin=()=>page.evaluate(()=>__game._bagScreen());
 async function pull(dx,dy,hold=0){const p=await origin();await page.mouse.move(p.x,p.y);await page.mouse.down();await page.mouse.move(p.x+dx,p.y+dy,{steps:5});if(hold)await page.waitForTimeout(hold);await page.mouse.up();return page.evaluate(()=>__game.state);}
 await start();assert.equal(await page.locator('[role=meter]').count(),0);
 const fast=await pull(8,60);assert.equal(fast.bags.length,1);assert(Math.abs(fast.bags[0].p[0])<.05);assert(fast.aim<0);await start();const slow=await pull(8,60,600);assert.equal(fast.lastPower,slow.lastPower);assert.equal(fast.aim,slow.aim);
 await start();let p=await origin();await page.mouse.move(p.x,p.y);await page.mouse.down();await page.mouse.move(p.x,p.y+60);await page.mouse.move(p.x,p.y);await page.mouse.up();assert.equal((await page.evaluate(()=>__game.state)).bags.length,0);
 await pull(0,-80);await page.mouse.wheel(0,-300);await page.waitForTimeout(120);assert.equal((await page.evaluate(()=>__game.state)).bags.length,0);
 // Direct horizontal positioning changes the real launch origin without an accidental throw.
 await start();p=await origin();await page.mouse.move(p.x,p.y);await page.mouse.down();await page.mouse.move(p.x+150,p.y,{steps:10});await page.mouse.up();let s=await page.evaluate(()=>__game.state);assert(s.position[0]>0);assert.equal(s.bags.length,0);const pos=s.position[0];s=await pull(0,50);assert(Math.abs(s.bags[0].p[0]-pos)<.05);
 for(const arc of [0,1,2])for(const side of ['slick','sticky']){
  await start();await page.locator(`[data-height="${arc}"]`).click();await page.locator(`[data-side="${side}"]`).click();await page.evaluate(()=>__game.throwWith(__game.state.sweet.mid));assert.equal(await page.locator('#feedback-power').textContent(),'POWER IN RANGE',`arc ${arc} ${side}`);assert.equal((await page.evaluate(()=>__game.state)).arc,arc);
 }
 for(const [power,label]of [[.01,'TOO WEAK'],[1,'TOO STRONG']]){await start();await page.evaluate(p=>__game.throwWith(p),power);assert.equal(await page.locator('#feedback-power').textContent(),label);}
 for(const [width,height]of [[1280,800],[390,844],[844,390]]){
  await page.setViewportSize({width,height});await start();if(width<900&&await page.locator('#bag-handle').getAttribute('aria-expanded')==='true'){await page.locator('#bag-handle').click();await page.waitForTimeout(180);}
  p=await origin();const cdp=await context.newCDPSession(page);await cdp.send('Input.dispatchTouchEvent',{type:'touchStart',touchPoints:[p]});await cdp.send('Input.dispatchTouchEvent',{type:'touchMove',touchPoints:[{x:p.x-8,y:Math.min(height-24,p.y+50)}]});await cdp.send('Input.dispatchTouchEvent',{type:'touchEnd',touchPoints:[]});
  s=await page.evaluate(()=>__game.state);assert.equal(s.bags.length,1);assert(s.aim>0);assert((await page.locator('#feedback-direction').textContent()).includes('RIGHT'));assert(await page.locator('#swipe-feedback').evaluate(e=>e.classList.contains('shown')));
  const box=await page.locator('#shot-feedback').boundingBox();assert(box.x>=0&&box.y>=0&&box.x+box.width<=width&&box.y+box.height<height);if(process.env.SHOTS)await page.screenshot({path:process.env.SHOTS+`/cornhole-pull-feedback-${width}.png`});
 }
 await start();p=await origin();await page.mouse.move(p.x,p.y);await page.mouse.down();await page.mouse.move(p.x,p.y+50);await page.evaluate(()=>window.dispatchEvent(new Event('blur')));await page.mouse.up();assert.equal((await page.evaluate(()=>__game.state)).bags.length,0);
 assert.deepEqual(errors,[]);console.log('Cornhole pull/release: mouse/touch, repeatable holds, cancel, no flick/wheel, actual moved origin, all heights/surfaces, power/direction feedback and responsive bounds passed.');
}finally{await browser.close();}})().catch(e=>{console.error(e);process.exitCode=1});
