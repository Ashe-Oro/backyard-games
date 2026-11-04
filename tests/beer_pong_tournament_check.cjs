const {chromium}=require('playwright-core');
const assert=require('node:assert/strict');
const fs=require('node:fs');
const BASE=process.env.BASE_URL||'http://127.0.0.1:8012';
(async()=>{
 const browser=await chromium.launch({executablePath:'/usr/bin/google-chrome',args:['--no-sandbox']});
 try{
  for(const phone of [false,true]){
   const context=await browser.newContext({viewport:phone?{width:390,height:844}:{width:1280,height:800}});
   const page=await context.newPage(),errors=[];page.on('pageerror',e=>errors.push(e.message));
   await page.addInitScript(()=>localStorage.setItem('tailgate-beer-pong-help-seen','1'));
   // Fast-forward only the rack, then use the real end-of-shot/result/save path.
   await page.route('**/beer-pong/game.js',async route=>{
    const response=await route.fetch();await route.fulfill({response,body:(await response.text())+`\nwindow.__finishRound=winner=>{for(const c of cups)if(c.team===1-winner){c.alive=false;c.group.visible=false;}finishShot();};`});
   });
   await page.goto(BASE+'/beer-pong/');await page.waitForFunction(()=>window.__pong);
   const shot=async name=>{fs.mkdirSync('shots',{recursive:true});await page.screenshot({path:`shots/pong-tournament-${name}-${phone?'phone':'desktop'}.png`});};
   assert.equal(await page.locator('[data-mode=tournament]').count(),1);await shot('menu');
   await page.locator('[data-mode=tournament]').click();await shot('intro');
   assert.equal(await page.locator('#tournament-title').textContent(),'Sunny');
   const ladder=await page.evaluate(()=>__pong.tournament.ladder);
   assert.equal(ladder.length,7);assert.equal(new Set(ladder.map(o=>o.name)).size,7);assert.equal(new Set(ladder.map(o=>o.avatar)).size,7);
   for(let i=1;i<ladder.length;i++){
    assert(ladder[i].shoot.precise>ladder[i-1].shoot.precise);assert(ladder[i].shoot.jitter<ladder[i-1].shoot.jitter);assert(ladder[i].shoot.bounce>ladder[i-1].shoot.bounce);
    assert(ladder[i].swat.attention>ladder[i-1].swat.attention);assert(ladder[i].swat.reaction<ladder[i-1].swat.reaction);
   }
   await page.locator('#tournament-go').click();assert.equal(await page.evaluate(()=>__pong.state.mode),'cpu');await shot('match');
   await page.evaluate(()=>__finishRound(1));assert.equal(await page.locator('#play-again').textContent(),'TRY AGAIN');await shot('loss');
   await page.locator('#play-again').click();assert.equal(await page.locator('#tournament-title').textContent(),'Sunny');await page.locator('#tournament-go').click();
   await page.evaluate(()=>__finishRound(0));assert.equal(await page.locator('#play-again').textContent(),'NEXT ROUND');await shot('win');
   await page.locator('#play-again').click();assert.equal(await page.locator('#tournament-title').textContent(),'Sprout');
   await page.reload();await page.waitForFunction(()=>window.__pong);await page.locator('[data-mode=tournament]').click();assert.equal(await page.locator('#tournament-title').textContent(),'Sprout');
   for(let i=1;i<7;i++){
    await page.locator('#tournament-go').click();assert.equal(await page.evaluate(()=>__pong.state.opponent),ladder[i].id);
    await page.evaluate(()=>__finishRound(0));await page.locator('#play-again').click();
   }
   assert.equal(await page.locator('#tournament-title').textContent(),'PONG CHAMPION!');await shot('champion');
   await page.reload();await page.waitForFunction(()=>window.__pong);await page.locator('[data-mode=tournament]').click();assert.equal(await page.locator('#tournament-title').textContent(),'PONG CHAMPION!');
   if(phone)for(const id of ['tournament-go','tournament-menu'])assert((await page.locator('#'+id).boundingBox()).height>=44);
   await page.locator('#tournament-menu').click();assert(await page.locator('#menu').isVisible());assert.deepEqual(errors,[]);await context.close();
  }
  console.log('Tournament menu, ladder tuning, intros, win/loss flow, persistence and champion pass on desktop and phone.');
 }finally{await browser.close();}
})().catch(e=>{console.error(e);process.exitCode=1;});
