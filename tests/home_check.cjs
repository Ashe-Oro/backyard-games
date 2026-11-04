const {chromium}=require('playwright-core');const assert=require('node:assert/strict');
const BASE=process.env.BASE_URL||'http://127.0.0.1:8012';
(async()=>{const browser=await chromium.launch();try{
 const page=await browser.newPage({viewport:{width:1440,height:1000}}),errors=[];page.on('pageerror',e=>errors.push(e.message));await page.addInitScript(()=>{localStorage.setItem('backyard-cornhole-help-seen','1');localStorage.setItem('tailgate-beer-pong-help-seen','1')});
 await page.goto(BASE+'/');await page.evaluate(()=>document.fonts.ready);assert.equal(await page.title(),'Tailgate Games · The backyard is open');assert.equal(await page.locator('.game-card').count(),2);
 for(const [width,height]of [[1440,1000],[390,844],[320,700],[844,390]]){
  await page.setViewportSize({width,height});assert(await page.evaluate(()=>document.documentElement.scrollWidth<=innerWidth),'No horizontal overflow');
  for(const img of await page.locator('.game-art img').all())assert(await img.evaluate(e=>e.complete&&e.naturalWidth>0),'Game artwork must load');
  for(const link of await page.locator('.play-link').all()){const b=await link.boundingBox();assert(b.width>150&&b.height>=44);assert(b.x>=0&&b.x+b.width<=width);}
  if(process.env.SHOTS)await page.screenshot({path:process.env.SHOTS+`/tailgate-home-${width}.png`,fullPage:true});
 }
 await page.setViewportSize({width:1280,height:800});await page.getByRole('link',{name:'Play Cornhole',exact:true}).click();await page.waitForFunction(()=>!!window.__game);assert(page.url().includes('/cornhole/'));
 await page.locator('#practice').click();await page.waitForFunction(()=>__game.state.phase==='aim');assert.equal(await page.locator('[role=meter]').count(),0);await page.evaluate(()=>__game.throwWith(__game.state.sweet.mid));await page.waitForFunction(()=>__game.state.phase==='aim');assert.equal(await page.evaluate(()=>__game.state.practiceStats.throws),1);
 await page.locator('#pause-button').click();await page.locator('#pause-menu').click();await page.getByRole('link',{name:'ALL TAILGATE GAMES'}).click();assert.equal(await page.locator('.game-card').count(),2);
 await page.getByRole('link',{name:'Play Beer Pong',exact:true}).click();await page.waitForFunction(()=>!!window.__pong);await page.locator('[data-mode=practice]').click();await page.evaluate(()=>__pong.throwWith(__pong._powerFor(10),0));await page.waitForFunction(()=>__pong.state.phase==='aim');assert.equal(await page.evaluate(()=>__pong.state.hits[0]),1);
 await page.locator('#pause-button').click();await page.locator('#pause-menu').click();await page.getByRole('link',{name:'ALL TAILGATE GAMES'}).click();assert.equal(await page.locator('.game-card').count(),2);
 // Selection is also usable with the keyboard, including the skip link.
 await page.goto(BASE+'/');await page.keyboard.press('Tab');assert.equal(await page.locator('.skip-link').evaluate(e=>e===document.activeElement),true);await page.keyboard.press('Enter');assert(page.url().endsWith('#games'));
 assert.deepEqual(errors,[]);console.log('Home: responsive artwork/cards, keyboard navigation, both game routes, actual scoring and return-home flows passed.');
}finally{await browser.close();}})().catch(e=>{console.error(e);process.exitCode=1});
