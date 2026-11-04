// Run with Playwright Core available: node tests/swipe_feedback_check.cjs
const {chromium} = require('playwright-core');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');

(async()=>{
  const browser=await chromium.launch({headless:true});
  try {
    for(const touch of [false,true]) {
      const width=touch?390:1280,height=touch?844:800;
      const context=await browser.newContext({viewport:{width,height},hasTouch:touch});
      const page=await context.newPage(), errors=[];
      await page.addInitScript(()=>{try{localStorage.setItem('backyard-cornhole-help-seen','1');}catch{}});
      page.on('pageerror',e=>errors.push(e.message));
      const url='http://127.0.0.1:8012/';
      await page.route(url,r=>r.fulfill({body:fs.readFileSync(path.join(__dirname,'../index.html'),'utf8'),contentType:'text/html'}));
      await page.goto(url);await page.waitForFunction(()=>!!window.__game);
      const cdp=await context.newCDPSession(page);
      let inputTime;
      async function start(){
        await page.evaluate(()=>__game.startMatch({practice:true,vs:'2p',first:0}));
        await page.waitForFunction(()=>__game.state.phase==='aim');
        if(touch && await page.locator('#bag-handle').getAttribute('aria-expanded')==='true') {
          await page.locator('#bag-handle').click();await page.waitForTimeout(180);
        }
        inputTime=Date.now()/1000;
      }
      async function point(type,x,y){
        // Timestamp native events consistently even when software rendering is slow.
        inputTime+=.02;
        if(touch) await cdp.send('Input.dispatchTouchEvent',{type,timestamp:inputTime,touchPoints:type==='touchEnd'||type==='touchCancel'?[]:[{x,y}]});
        else await cdp.send('Input.dispatchMouseEvent',{type:type==='touchStart'?'mousePressed':type==='touchMove'?'mouseMoved':'mouseReleased',x,y,button:'left',buttons:type==='touchEnd'?0:1,clickCount:1,timestamp:inputTime});
      }
      for(const direction of [-1,0,1]){
        await start();
        assert.equal(await page.locator('#swipe-feedback').evaluate(e=>e.classList.contains('shown')),false);
        const x=width*.5,y=height*.8,dx=direction*width*.12,dy=-height*.25;
        await point('touchStart',x,y);
        for(let i=1;i<=4;i++){
          await point('touchMove',x+dx*i/4,y+dy*i/4);await page.waitForTimeout(20);
        }
        assert.equal(await page.locator('#swipe-feedback').evaluate(e=>e.classList.contains('shown')),false,'No arrow before release');
        await point('touchEnd',x+dx,y+dy);
        await page.waitForFunction(()=>document.querySelector('#swipe-feedback').classList.contains('shown'),null,{timeout:2000}).catch(async e=>{
          console.error(await page.evaluate(()=>({state:__game.state,arrow:document.querySelector('#swipe-feedback').outerHTML})),errors);throw e;
        });
        const arrow=await page.locator('#swipe-line').evaluate(e=>Object.fromEntries(['x1','x2','y1','y2'].map(k=>[k,+e.getAttribute(k)])));
        assert.equal(Math.sign(arrow.x2-arrow.x1),direction);
        assert(arrow.y2<arrow.y1,'Arrow must follow upward swipe');
        assert(Math.abs(arrow.x2-(x+dx))<1 && Math.abs(arrow.y2-(y+dy))<1,'Arrow must end at actual finger release');
        assert(Math.abs((arrow.x2-arrow.x1)/(arrow.y2-arrow.y1)-dx/dy)<.01,'Arrow must preserve measured swipe angle');
        assert.equal(await page.evaluate(()=>__game.state.bags.length),1);
        assert.equal(await page.locator('#swipe-feedback').evaluate(e=>getComputedStyle(e).pointerEvents),'none');
        if(process.env.SHOTS && direction===1)await page.screenshot({path:path.join(process.env.SHOTS,`swipe-feedback-${touch?'mobile':'desktop'}.png`)});
        await page.waitForFunction(()=>getComputedStyle(document.querySelector('#swipe-feedback')).opacity==='0');
        console.log(`${touch?'touch':'mouse'} ${direction}: actual swipe direction and fade passed`);
      }
      // A paused gesture is rejected by the throw logic and must not get an arrow.
      await start();await point('touchStart',width*.5,height*.8);
      await point('touchMove',width*.5,height*.6);await page.waitForTimeout(220);inputTime+=.22;
      await point('touchEnd',width*.5,height*.6);
      assert.equal(await page.evaluate(()=>__game.state.bags.length),0);
      assert.equal(await page.locator('#swipe-feedback').evaluate(e=>e.classList.contains('shown')),false);
      if(touch){
        await start();await point('touchStart',width*.5,height*.8);
        await point('touchMove',width*.55,height*.6);await point('touchCancel',width*.55,height*.6);
        assert.equal(await page.evaluate(()=>__game.state.bags.length),0);
        assert.equal(await page.locator('#swipe-feedback').evaluate(e=>e.classList.contains('shown')),false);
      }
      assert.deepEqual(errors,[]);await context.close();
    }
    console.log('Swipe feedback passed for real mouse and touch input.');
  }finally{await browser.close();}
})().catch(e=>{console.error(e);process.exitCode=1;});
