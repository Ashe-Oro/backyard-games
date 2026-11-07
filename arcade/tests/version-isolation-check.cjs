const {chromium,_electron}=require('playwright-core');
const assert=require('node:assert/strict'),fs=require('node:fs'),path=require('node:path');
(async()=>{
 const browser=await chromium.launch({executablePath:process.env.CHROME_PATH});let app;
 try{
  const web=await browser.newPage({viewport:{width:1280,height:800}}),requests=[],errors=[];
  web.on('request',r=>requests.push(r.url()));web.on('pageerror',e=>errors.push(e.message));
  await web.route('https://cdn.jsdelivr.net/npm/three@0.170.0/build/three.module.min.js',route=>route.fulfill({path:path.resolve(__dirname,'../node_modules/three/build/three.module.min.js'),contentType:'application/javascript'}));
  await web.route(/https:\/\/fonts\./,route=>route.abort());
  await web.goto((process.env.WEB_URL||'http://127.0.0.1:8025')+'/cornhole/');await web.waitForFunction(()=>window.__game);
  assert(!requests.some(url=>url.includes('/arcade/')),'Website requested arcade assets');
  assert.equal(await web.evaluate(()=>!!window.__arcade),false);assert.equal(await web.locator('#arcade-ui').count(),0);
  await web.evaluate(()=>localStorage.setItem('backyard-arcade-v4','browser-only-sentinel'));
  // Even an accidental import must leave the normal browser game and saves alone.
  const rejected=await web.evaluate(async()=>{try{await import('/arcade/controller.js');return false;}catch(e){return e.message.includes('standalone Cornhole build');}});assert(rejected);
  assert.equal(await web.locator('#arcade-ui').count(),0);
  await web.locator('#practice').click();await web.waitForFunction(()=>__game.state.phase==='aim');
  const bag=await web.evaluate(()=>__game._bagScreen());await web.mouse.move(bag.x,bag.y);await web.mouse.down();await web.mouse.move(bag.x,bag.y+85,{steps:10});await web.mouse.up();await web.waitForFunction(()=>__game.state.bags.length===1);
  app=await _electron.launch({executablePath:process.env.ELECTRON_PATH,args:['--windowed'],env:{...process.env,ARCADE_TEST_DATA:fs.mkdtempSync('/tmp/cornhole-isolation-')}});
  const cabinet=await app.firstWindow();await cabinet.waitForFunction(()=>window.__arcade);
  assert.equal(await cabinet.evaluate(()=>location.origin),'backyard://game');
  assert.equal(await cabinet.evaluate(async()=>(await fetch('/arcade/controller.js')).headers.get('cache-control')),'no-store');
  assert.equal(await cabinet.evaluate(()=>localStorage.getItem('backyard-arcade-v4')),null);
  assert(await cabinet.evaluate(()=>document.documentElement.hasAttribute('data-backyard-arcade')));
  await cabinet.evaluate(()=>{__arcade.setConfig({sensitivity:2});__arcade.start('practice',false)});await cabinet.waitForFunction(()=>__game.state.phase==='aim');
  assert.equal(await cabinet.evaluate(()=>__arcade.config.sensitivity),2);
  assert.equal(await web.evaluate(()=>localStorage.getItem('backyard-arcade-v4')),'browser-only-sentinel');
  assert.equal(await web.evaluate(()=>__game.state.bags.length),1);assert.equal(await web.locator('#arcade-ui').count(),0);assert.deepEqual(errors,[]);
  console.log('Web loads no arcade assets; accidental import is inert; normal pull-to-throw works; standalone app starts on backyard://game; app settings do not touch browser saves.');
 }finally{if(app)await app.close();await browser.close();}
})().catch(e=>{console.error(e);process.exitCode=1});
