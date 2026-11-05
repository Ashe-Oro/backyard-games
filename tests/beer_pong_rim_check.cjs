const {chromium}=require('playwright-core');const assert=require('node:assert/strict');
const BASE=process.env.BASE_URL||'http://127.0.0.1:8012';
(async()=>{const browser=await chromium.launch();try{
 const page=await browser.newPage(),errors=[];page.on('pageerror',e=>errors.push(e.message));await page.addInitScript(()=>localStorage.setItem('tailgate-beer-pong-help-seen','1'));
 // Instrument the served module only in this test; exercise the real collision step without adding a production API.
 await page.route('**/beer-pong/game.js',async route=>{const response=await route.fetch(),source=await response.text();assert(source.includes('window.__pong={'));await route.fulfill({response,body:source.replace('window.__pong={',`window.__pong={
 _rimProbe:(offset,wall=false)=>{const c=cups[10],b=newBall(0,.4);b.p.set(c.x+offset,wall?TABLE_Y+.08:RIM_Y+BALL_R+.01,c.z);b.v.set(wall?-2:0,wall?0:-2,0);for(let i=0;i<32&&b.state==='fly'&&!b.rimHits;i++)stepBall(b,1/240);return {state:b.state,cup:b.hitCup?.id,v:b.v.toArray(),rimHits:b.rimHits};},`)});});
 await page.goto(BASE+'/beer-pong/');await page.waitForFunction(()=>!!__pong);await page.evaluate(()=>__pong.startMatch('practice'));
 const edge=await page.evaluate(()=>__pong._rimProbe(.058));assert.equal(edge.state,'sink','A ball near the visible inner edge should fit');assert.equal(edge.cup,10);assert.equal(edge.rimHits,0);
 const rim=await page.evaluate(()=>__pong._rimProbe(.083));assert.equal(rim.rimHits,1);assert.equal(rim.state,'fly');assert(rim.v[1]>0&&rim.v[1]<.8,'Rim contacts should rebound softly instead of launching the ball away');
 const wall=await page.evaluate(()=>__pong._rimProbe(.095,true));assert.equal(wall.rimHits,1);assert(wall.v[0]>0&&wall.v[0]<.7,'Cup sides should deflect gently');
 const miss=await page.evaluate(()=>__pong._rimProbe(.125));assert.equal(miss.rimHits,0);assert.notEqual(miss.state,'sink','A clear miss must stay a miss');
 for(const id of [10,11,12,13,14,15,16,17,18,19]){const result=await page.evaluate(id=>__pong._probe(__pong._powerFor(id),__pong._slopeFor(id)),id);assert.equal(result.cup,id,'Every cup remains reachable');}
 assert.deepEqual(errors,[]);console.log('Forgiving inner-edge sinks, soft rim/side rebounds, honest misses and all ten cup targets passed.');
}finally{await browser.close()}})().catch(e=>{console.error(e);process.exitCode=1});
