const {chromium}=require('playwright-core');
const assert=require('node:assert/strict');
const path=require('node:path');
const BASE=process.env.BASE_URL||'http://127.0.0.1:8012';
(async()=>{
 const browser=await chromium.launch({headless:true});
 try{
  for(const touch of [false,true]){
   const width=touch?390:1280,height=touch?844:800,context=await browser.newContext({viewport:{width,height},hasTouch:touch}),page=await context.newPage();
   const errors=[];page.on('pageerror',e=>errors.push(e.message));
   await page.addInitScript(()=>localStorage.setItem('tailgate-beer-pong-help-seen','1'));
   await page.goto(BASE+'/beer-pong/');await page.waitForFunction(()=>!!window.__pong);
   const cdp=await context.newCDPSession(page);let time;
   async function point(type,x,y){time+=.02;
    if(touch)await cdp.send('Input.dispatchTouchEvent',{type,timestamp:time,touchPoints:type==='touchEnd'||type==='touchCancel'?[]:[{x,y}]});
    else await cdp.send('Input.dispatchMouseEvent',{type:type==='touchStart'?'mousePressed':type==='touchMove'?'mouseMoved':'mouseReleased',x,y,timestamp:time,button:'left',buttons:type==='touchEnd'?0:1,clickCount:1});
   }
   async function swipe(direction){
    time=Date.now()/1000;const x=width*.5,y=height*.8,dx=direction*width*.09,dy=-height*.25;
    await point('touchStart',x,y);for(let i=1;i<=4;i++)await point('touchMove',x+dx*i/4,y+dy*i/4);
    assert.equal(await page.locator('#swipe-feedback').evaluate(e=>e.classList.contains('show')),false);
    await point('touchEnd',x+dx,y+dy);
    await page.waitForFunction(()=>__pong.state.phase==='flight');
    const snapshot=await page.evaluate(()=>__pong.state);
    assert.equal(snapshot.shots[snapshot.turn],1);assert(snapshot.power>.2&&snapshot.power<.8);
    assert.equal(Math.sign(snapshot.ball.v[0]),direction*(snapshot.turn===0?1:-1));
    assert.equal(await page.locator('#swipe-feedback').evaluate(e=>e.classList.contains('show')),true);
    const arrow=await page.locator('#swipe-line').evaluate(e=>({dx:+e.getAttribute('x2')-e.getAttribute('x1'),x:+e.getAttribute('x2'),y:+e.getAttribute('y2')}));
    assert.equal(Math.sign(arrow.dx),direction);assert(Math.abs(arrow.x-(x+dx))<1&&Math.abs(arrow.y-(y+dy))<1);
   }
   for(const direction of [-1,0,1]){
    await page.evaluate(()=>__pong.startMatch('practice'));await page.waitForFunction(()=>__pong.state.phase==='aim');
    await swipe(direction);
    if(process.env.SHOTS&&direction===1)await page.screenshot({path:path.join(process.env.SHOTS,`beer-pong-swipe-${touch?'mobile':'desktop'}.png`)});
    console.log(`${touch?'Touch':'Mouse'} ${direction}: power, direction, launch and feedback passed.`);
   }
   if(touch){
    await page.evaluate(()=>__pong.startMatch('practice'));time=Date.now()/1000;
    await point('touchStart',width*.5,height*.8);await point('touchMove',width*.55,height*.6);await point('touchCancel',width*.55,height*.6);
    assert.equal(await page.evaluate(()=>__pong.state.shots[0]),0);
   }
   // The same rightward swipe must remain rightward after the two-player camera flip.
   await page.evaluate(()=>__pong.startMatch('two'));await page.locator('#ready').click();await page.evaluate(()=>__pong.throwWith(.1,0));
   await page.waitForFunction(()=>__pong.state.phase==='handoff');await page.locator('#ready').click();
   assert.equal(await page.evaluate(()=>__pong.state.turn),1);await swipe(1);
   assert.deepEqual(errors,[]);await context.close();
  }
  console.log('Beer Pong mouse/touch inputs and mirrored two-player aiming passed.');
 }finally{await browser.close();}
})().catch(e=>{console.error(e);process.exitCode=1;});
