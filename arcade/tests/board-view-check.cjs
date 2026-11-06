const {_electron}=require('playwright-core'),assert=require('node:assert/strict'),fs=require('node:fs');
(async()=>{const app=await _electron.launch({executablePath:process.env.ELECTRON_PATH,args:['--windowed'],env:{...process.env,ARCADE_TEST_DATA:fs.mkdtempSync('/tmp/cornhole-board-')}});try{
 const p=await app.firstWindow(),errors=[];p.on('pageerror',e=>errors.push(e.message));await p.waitForFunction(()=>window.__arcadeHost?.boardView);
 assert.equal(await p.locator('#arcade-board-view').isVisible(),false);
 await p.evaluate(()=>__arcade.start('practice',false));await p.waitForFunction(()=>__game.state.phase==='aim');await p.waitForFunction(()=>!document.getElementById('arcade-board-view').hidden);
 // Instrument both cameras and ensure the close-up uses the same live objects.
 await p.evaluate(()=>{const v=__arcadeHost.boardView,r=v.renderer,render=r.render.bind(r);window.viewDraws={main:0,close:0};r.render=(s,c)=>{if(c===v.camera){viewDraws.close++;if(s!==v.scene)throw Error('Close-up scene differs');}else viewDraws.main++;return render(s,c);};__game._place(0,.15,-.3);__game._place(1,-.15,-.7);});
 await p.waitForFunction(()=>viewDraws.main>3&&viewDraws.close>3);
 await p.waitForTimeout(1600);
 const checks=await p.evaluate(()=>{const v=__arcadeHost.boardView;return {distance:__arcadeHost.boardZ,scissor:v.renderer.getScissorTest(),auto:v.renderer.autoClear,aspect:v.camera.aspect}});assert(Math.abs(checks.distance+27*.3048)<1e-8);assert.equal(checks.scissor,false);assert.equal(checks.auto,true);assert(checks.aspect>0);
 for(const [w,h] of [[1920,1080],[1280,720],[640,480]]){await p.setViewportSize({width:w,height:h});await p.waitForTimeout(100);const a=await p.locator('#arcade-board-view').boundingBox(),b=await p.locator('#arcade-scoreboard').boundingBox();assert(a.x>=0&&a.y>=0&&a.x+a.width<=w&&a.y+a.height<=h);assert(a.x+a.width<=b.x||a.y>=b.y+b.height,'Close-up overlaps scoreboard');}
 await p.setViewportSize({width:1280,height:720});await p.waitForTimeout(100);await p.screenshot({path:'/Users/ashe/Documents/Codex/2026-11-03/pleas/outputs/cornhole-board-view-026.png'});
 await p.evaluate(()=>__arcade.showMenu());await p.waitForFunction(()=>document.getElementById('arcade-board-view').hidden);await p.locator('#arcade-view-toggle').click();assert.equal(await p.locator('#arcade-view-toggle').textContent(),'BOARD VIEW: OFF');await p.evaluate(()=>__arcade.start('practice',false));await p.waitForTimeout(100);assert.equal(await p.locator('#arcade-board-view').isVisible(),false);
 await p.reload();await p.waitForFunction(()=>window.__arcade);assert.equal(await p.locator('#arcade-view-toggle').textContent(),'BOARD VIEW: OFF');assert.deepEqual(errors,[]);console.log('Live shared-scene board camera, renderer-state restoration, 27ft distance, viewport clearance, menu hiding and persistent toggle passed.');
 }finally{await app.close();}})().catch(e=>{console.error(e);process.exitCode=1});
