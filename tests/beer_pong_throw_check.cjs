const {chromium}=require('playwright-core');
const assert=require('node:assert/strict');
const fs=require('node:fs');
const {execFileSync}=require('node:child_process');
const BASE=process.env.BASE_URL||'http://127.0.0.1:8012';
(async()=>{const browser=await chromium.launch();try{
 for(const [distance,style] of [[25,'bounce'],[100,'arc']])for(const downward of [false,true])for(const touch of (process.env.PONG_TOUCH_ONLY?[true]:[false,true])){
  const context=await browser.newContext({viewport:touch?{width:390,height:844}:{width:1280,height:800},hasTouch:touch});
  const page=await context.newPage(),errors=[];page.on('pageerror',e=>errors.push(e.message));
  if(process.env.PONG_REV){
   const revision=process.env.PONG_REV;
   for(const file of ['game.js','index.html','style.css'])await page.route(`**/beer-pong/${file==='index.html'?'':file}`,route=>route.fulfill({body:execFileSync('git',['show',`${revision}:beer-pong/${file}`]),contentType:file.endsWith('.js')?'application/javascript':file.endsWith('.css')?'text/css':'text/html'}));
  }
  await page.addInitScript(()=>localStorage.setItem('tailgate-beer-pong-help-seen','1'));
  await page.goto(BASE+'/beer-pong/');await page.waitForFunction(()=>!!window.__pong);await page.locator('[data-mode=practice]').click();
  const origin=await page.evaluate(()=>__pong._screen(...__pong.state.held.p));
  if(touch){
   const cdp=await context.newCDPSession(page);
   await cdp.send('Input.dispatchTouchEvent',{type:'touchStart',touchPoints:[origin]});
   for(let i=1;i<=5;i++)await cdp.send('Input.dispatchTouchEvent',{type:'touchMove',touchPoints:[{x:origin.x+12*i/5,y:origin.y+(downward?1:-1)*distance*i/5}]});
   await cdp.send('Input.dispatchTouchEvent',{type:'touchEnd',touchPoints:[]});
  }else{
   await page.mouse.move(origin.x,origin.y);await page.mouse.down();await page.mouse.move(origin.x+12,origin.y+(downward?1:-1)*distance,{steps:5});await page.mouse.up();
  }
  const launched=await page.evaluate(()=>__pong.state);
  if(process.env.SHOTS){fs.mkdirSync(process.env.SHOTS,{recursive:true});await page.screenshot({path:`${process.env.SHOTS}/pong-bounce-power-${style}-${downward?'pull':'flick'}-${touch?'touch':'mouse'}.png`});}
  assert.equal(launched.shots[0],1,`${touch?'touch swipe':'mouse flick'} launches once`);
  assert.equal(launched.phase,'flight');assert(launched.ball.v[2]<0,'Ball heads toward opposing cups');assert.equal(launched.ball.style,style);assert(downward?launched.ball.v[0]<0:launched.ball.v[0]>0,'Gesture direction controls aim');
  await page.waitForTimeout(120);const later=await page.evaluate(()=>__pong.state);
  assert(later.ball&&later.ball.p[2]<launched.ball.p[2]-.1,'Ball actually advances toward cups');assert.equal(later.shots[0],1);assert.deepEqual(errors,[]);
  await context.close();
 }
 console.log('Desktop flick and phone native touch swipe launch and travel toward cups.');
}finally{await browser.close();}})().catch(e=>{console.error(e);process.exitCode=1});
