const {chromium}=require('playwright-core');const assert=require('node:assert/strict');
const BASE=process.env.BASE_URL||'http://127.0.0.1:8012';
(async()=>{const browser=await chromium.launch();try{
 const page=await browser.newPage({viewport:{width:1280,height:800}}),errors=[];page.on('pageerror',e=>errors.push(e.message));await page.addInitScript(()=>localStorage.setItem('tailgate-beer-pong-help-seen','1'));await page.goto(BASE+'/beer-pong/');await page.waitForFunction(()=>!!window.__pong);
 const button=page.locator('#rerack-button');
 async function start(mode='practice'){await page.evaluate(mode=>__pong.startMatch(mode),mode);if(mode==='two')await page.locator('#ready').click();}
 async function sink(id){assert(await page.evaluate(id=>__pong._dropAt(id),id));await page.waitForFunction(()=>['aim','handoff'].includes(__pong.state.phase));}
 async function reduce(count){for(let id=10;id<20-count;id++)await sink(id);}
 await start();assert(await button.isVisible());assert.equal(await button.isEnabled(),false);
 for(const [count,layouts]of [[6,['triangle']],[4,['diamond','square']],[3,['triangle','stoplight']],[2,['line']]])for(const layout of layouts){
  await start();await reduce(count);assert(await button.isEnabled());const before=await page.evaluate(()=>__pong.state);await button.click();assert(await page.locator('#rerack-dialog').isVisible());assert.equal(await page.locator('#rerack-options button').count(),layouts.length);assert.equal(await page.evaluate(()=>__pong.state.reracksUsed[0]),false);assert.equal(await page.evaluate(()=>__pong.throwWith(.5)),false,'No throws through the picker');
  await page.getByRole('button',{name:layout==='line'?'Front-to-back':layout[0].toUpperCase()+layout.slice(1),exact:true}).click();
  if(process.env.SHOTS)await page.screenshot({path:process.env.SHOTS+`/rerack-${count}-${layout}.png`});await page.locator('#rerack-confirm').click();
  const after=await page.evaluate(()=>__pong.state),rack=after.cups.filter(c=>c.team===1&&c.alive);assert.equal(after.remaining[1],count);assert.deepEqual(after.shots,before.shots);assert.deepEqual(after.hits,before.hits);assert.deepEqual(rack.map(c=>c.id),before.cups.filter(c=>c.team===1&&c.alive).map(c=>c.id));assert.equal(after.reracksUsed[0],true);assert.equal(await button.isVisible(),false);
  assert(Math.abs(rack.reduce((n,c)=>n+c.x,0))<1e-8,'Formation must be centered');for(const c of rack){assert(Math.abs(c.x)<.5&&c.z> -2.95+.078&&c.z< -1.5);for(const other of rack)if(c!==other)assert(Math.hypot(c.x-other.x,c.z-other.z)>.163,'Cup rims must not overlap');}
  if(layout==='line'||layout==='stoplight')assert(rack.every(c=>c.x===0));
  // Aim at every relocated cup using the real ballistic sink detector.
  for(const c of rack){const probe=await page.evaluate(c=>__pong._probe(__pong._powerFor(c.id),__pong._slopeFor(c.id)),c);assert.equal(probe.cup,c.id,`${layout} cup ${c.id} must be reachable at its new position`);}
  // A real relocated-cup hit proves scoring follows the geometry, not the old location.
  const cup=rack[0];await page.evaluate(c=>__pong.throwWith(__pong._powerFor(c.id),__pong._slopeFor(c.id)),cup);await page.waitForFunction(id=>!__pong.state.cups[id].alive,cup.id);await page.waitForFunction(()=>__pong.state.phase==='aim');assert.equal(await button.isVisible(),false);
  await page.locator('#refill').click();assert(await button.isVisible());assert.equal(await button.isEnabled(),false);assert.deepEqual(await page.evaluate(()=>__pong.state.reracksUsed),[false,false]);const initial=await page.evaluate(()=>__pong.state.cups.filter(c=>c.team===1));assert(Math.abs(initial[0].z+2.1)<1e-8);
 }
 // Escape/cancel do not consume or move cups; 5 and 1 cups are not eligible.
 await start();await reduce(6);const original=await page.evaluate(()=>__pong.state.cups);await button.click();await page.keyboard.press('Escape');assert.deepEqual(await page.evaluate(()=>__pong.state.cups),original);assert.equal(await page.evaluate(()=>__pong.state.reracksUsed[0]),false);await button.click();await page.locator('#rerack-cancel').click();assert(await button.isEnabled());await sink(14);assert.equal(await button.isEnabled(),false);
 // Independent allowance for local players and mirrored placement of blue's target rack.
 await start('two');for(let i=0;i<4;i++){await sink(10+i);await page.locator('#ready').click();await sink(i);await page.locator('#ready').click();}await button.click();await page.locator('#rerack-confirm').click();assert.equal(await button.isVisible(),false);await sink(14);await page.locator('#ready').click();assert(await button.isEnabled());await button.click();await page.locator('#rerack-confirm').click();assert.deepEqual(await page.evaluate(()=>__pong.state.reracksUsed),[true,true]);assert.equal(await button.isVisible(),false);assert((await page.evaluate(()=>__pong.state.cups.filter(c=>c.team===0&&c.alive))).every(c=>c.z>-.6&&c.z<.55));
 // Responsive, keyboard-operable picker and HUD controls.
 for(const [width,height]of [[390,844],[320,700],[844,390]]){await page.setViewportSize({width,height});await start();await reduce(4);const box=await button.boundingBox();assert(box.x>=0&&box.y>=0&&box.x+box.width<=width);await button.click();await page.getByRole('button',{name:'Square',exact:true}).focus();await page.keyboard.press('Enter');assert.equal(await page.getByRole('button',{name:'Square',exact:true}).getAttribute('aria-pressed'),'true');if(process.env.SHOTS)await page.screenshot({path:process.env.SHOTS+`/rerack-dialog-${width}.png`});await page.locator('#rerack-confirm').click();assert.equal(await button.isVisible(),false);}
 assert.deepEqual(errors,[]);console.log('Re-racks: all layouts, one-use hiding, cancel, no restored cups/stats changes, real relocated scoring, resets, per-player/mirrored layouts, keyboard/mobile passed.');
}finally{await browser.close();}})().catch(e=>{console.error(e);process.exitCode=1});
