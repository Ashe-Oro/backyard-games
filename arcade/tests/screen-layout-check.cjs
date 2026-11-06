const {_electron}=require('playwright-core'),assert=require('node:assert/strict'),fs=require('node:fs');
const out=process.env.SHOT_DIR||'/Users/ashe/Documents/Codex/2026-11-03/pleas/outputs/arcade-screen-debug';fs.mkdirSync(out,{recursive:true});
const overlap=(a,b)=>a&&b&&a.x<b.x+b.width&&a.x+a.width>b.x&&a.y<b.y+b.height&&a.y+a.height>b.y;
(async()=>{const app=await _electron.launch({executablePath:process.env.ELECTRON_PATH,args:['--windowed'],env:{...process.env,ARCADE_TEST_DATA:fs.mkdtempSync('/tmp/cornhole-screen-')}});try{
 const p=await app.firstWindow(),errors=[],findings=[];p.on('pageerror',e=>errors.push(e.message));await p.waitForFunction(()=>window.__arcadeHost?.boardView);
 for(const [w,h] of [[640,480],[800,600],[1024,768],[1280,720],[1280,1024],[1920,1080]]){
  await p.setViewportSize({width:w,height:h});await p.evaluate(mode=>__arcade.start(mode,false),w===1920?'two':'practice');await p.waitForFunction(()=>__game.state.phase==='aim');await p.waitForTimeout(1700);
  await p.evaluate(()=>{__game._place(0,.15,-.3);__game._place(1,-.15,-.7);__arcadeHost.preview({power:.7,aim:-.12,spin:0});});await p.waitForTimeout(100);
  const arrow=await p.evaluate(()=>({shown:document.getElementById('swipe-feedback').classList.contains('shown'),paused:__game.state.paused,phase:__game.state.phase,help:document.getElementById('help-dialog').open}));assert.equal(arrow.shown,true);assert.equal(arrow.paused,false);
  const boxes={};for(const id of ['arcade-board-view','arcade-scoreboard','pause-button','arcade-prompt','reset-bags'])boxes[id]=await p.locator('#'+id).boundingBox().catch(()=>null);
  const inset=boxes['arcade-board-view'];for(const aim of [-.12,0,.12]){await p.evaluate(aim=>__arcadeHost.preview({power:1,aim,spin:0}),aim);const shaft=await p.locator('[data-power-shaft]').boundingBox();if(overlap(inset,shaft))findings.push(`${w}x${h}: inset overlaps aiming arrow`);}for(const [id,b] of Object.entries(boxes))if(id!=='arcade-board-view'&&overlap(inset,b))findings.push(`${w}x${h}: inset overlaps ${id}`);
  if(overlap(boxes['pause-button'],boxes['arcade-scoreboard']))findings.push(`${w}x${h}: pause overlaps scoreboard`);
  for(const id of ['turn','msg']){const text=await p.locator('#'+id).evaluate(el=>{const range=document.createRange();range.selectNodeContents(el);const r=range.getBoundingClientRect();return r.width?{x:r.x,y:r.y,width:r.width,height:r.height}:null;});if(overlap(inset,text))findings.push(`${w}x${h}: inset overlaps ${id} text`);}
  const points=await p.evaluate(()=>({board:__game._screenPoint(0,__arcadeHost.boardZ),bag:__game._bagScreen()}));
  for(const [name,v] of Object.entries(points))if(v.x>=inset.x-30&&v.x<=inset.x+inset.width+30&&v.y>=inset.y-30&&v.y<=inset.y+inset.height+30)findings.push(`${w}x${h}: inset crowds ${name}`);
  await p.screenshot({path:`${out}/${w}x${h}-aim.png`});
  // Real airborne shot and landing remain visible alongside the live close-up.
  await p.evaluate(()=>__game.throwWith(__game.state.sweet.mid));await p.waitForTimeout(400);await p.screenshot({path:`${out}/${w}x${h}-flight.png`});
  await p.evaluate(()=>__arcade.showMenu());await p.waitForFunction(()=>document.getElementById('arcade-board-view').hidden);await p.screenshot({path:`${out}/${w}x${h}-pause.png`});
  await p.locator('#arcade-config').click();assert.equal(await p.locator('#arcade-board-view').isVisible(),false);
 }
 assert.deepEqual(errors,[]);fs.writeFileSync(`${out}/findings.json`,JSON.stringify(findings,null,2));assert.deepEqual(findings,[]);console.log('Six cabinet sizes: inset clears scores, pause, main board, bag and controls; aiming, flight, pause and setup screenshots captured.');
 }finally{await app.close();}})().catch(e=>{console.error(e);process.exitCode=1});
