const {chromium}=require('playwright-core'),assert=require('node:assert/strict'),fs=require('node:fs'),path=require('node:path');
(async()=>{const b=await chromium.launch();try{const p=await b.newPage({viewport:{width:1920,height:1080}}),url=(process.env.BASE_URL||'http://127.0.0.1:8025')+'/cornhole/';
 const source=fs.readFileSync(path.join(__dirname,'../dist/site/cornhole/index.html'),'utf8');
 // Test-only fixture hooks enter settled bag outcomes; real lie()/scoreInning() resolve them.
 const fixture=`window.__roundFixture={add(team,state){const n=bags.filter(b=>b.team===team).length,b=newBody(team);b.mesh=makeBagMesh(team,'slick');b.lp.set(n%2?.16:-.16,BAG_H,n<2?-.25:-.65);toWorld(b.lp,b.p);b.state=state;b.touched=true;if(state==='grass')b.p.set(-1,0,0);bags.push(b);M.k=bags.length;},change(i,state,dead=false){bags[i].state=state;bags[i].dead=dead;},resolve(){M.k=8;M.phase='settle';}};\n`;
 await p.route(url,r=>r.fulfill({contentType:'text/html',body:source.replace('window.__arcadeHost={',fixture+'window.__arcadeHost={')}));await p.goto(url);await p.waitForFunction(()=>window.__arcade);await p.evaluate(()=>__arcade.start('two',false));await p.waitForFunction(()=>__game.state.phase==='aim');await p.evaluate(()=>__game._setScores(7,11));
 const read=()=>p.evaluate(()=>['red','blue'].map(c=>Number(document.getElementById('arcade-'+c+'-round').textContent)));
 const results=[[0,'in',3,0],[1,'board',3,1],[0,'in',6,1],[1,'board',6,2],[0,'grass',6,2],[1,'board',6,3],[0,'grass',6,3],[1,'board',6,4]];
 for(const [team,state,r,blue] of results){await p.evaluate(([t,s])=>__roundFixture.add(t,s),[team,state]);await p.waitForFunction(([r,bl])=>document.getElementById('arcade-red-round').textContent===String(r)&&document.getElementById('arcade-blue-round').textContent===String(bl),[r,blue]);assert.deepEqual(await p.evaluate(()=>__game.state.scores),[7,11]);}
 assert.deepEqual(await read(),[6,4]);assert.equal(await p.locator('#arcade-round-edge').textContent(),'RED +2');
 await p.screenshot({path:'/Users/ashe/Documents/Codex/2026-11-03/pleas/outputs/cornhole-round-6-4.png'});
 await p.evaluate(()=>{__arcadeHost.pause(true);__roundFixture.change(1,'slide');});await p.waitForTimeout(150);assert.deepEqual(await read(),[6,4],'Do not flicker while bags are moving');
 await p.evaluate(()=>__roundFixture.change(1,'grass'));await p.waitForFunction(()=>document.getElementById('arcade-blue-round').textContent==='3');
 await p.evaluate(()=>__roundFixture.change(1,'in'));await p.waitForFunction(()=>document.getElementById('arcade-blue-round').textContent==='6');assert.equal(await p.locator('#arcade-round-edge').textContent(),'EVEN');
 await p.evaluate(()=>__roundFixture.change(1,'board',true));await p.waitForFunction(()=>document.getElementById('arcade-blue-round').textContent==='3');
 await p.evaluate(()=>{__roundFixture.change(1,'board');__arcadeHost.pause(false);__roundFixture.resolve();});await p.waitForFunction(()=>__game.state.phase==='summary');assert.deepEqual(await p.evaluate(()=>__game.state.scores),[9,11]);assert.deepEqual(await read(),[6,4]);await p.waitForTimeout(500);assert.deepEqual(await p.evaluate(()=>__game.state.scores),[9,11]);
 await p.evaluate(()=>__arcadeHost.advance());await p.waitForFunction(()=>__game.state.inning===2);await p.waitForFunction(()=>document.getElementById('arcade-red-round').textContent==='0'&&document.getElementById('arcade-blue-round').textContent==='0');assert.deepEqual(await p.evaluate(()=>__game.state.scores),[9,11]);
 console.log('Per-bag round recount; stable moving totals; knock-off/knock-in/dead bags; 6–4 resolves once to +2; next round resets only round totals passed.');
 }finally{await b.close();}})().catch(e=>{console.error(e);process.exitCode=1});
