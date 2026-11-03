"""Task 12: real CPU physics, saved side choice, release alignment and responsive captures."""
import asyncio,json
from pathlib import Path
from playwright.async_api import async_playwright

async def main():
 async with async_playwright() as p:
  browser=await p.chromium.launch(executable_path='/usr/bin/google-chrome',args=['--no-sandbox'])
  page=await browser.new_page(ignore_https_errors=True)
  page.set_default_timeout(8000);page.set_default_navigation_timeout(12000)
  errors=[];page.on('pageerror',lambda e:errors.append(str(e)))
  source=Path('index.html').read_text().replace('window.__game = {','window.__test={M,roster,career,wind,planCareerShot,newBody,launch,step,RX,held,camera,releaseX,trajectory}; window.__game = {')
  await page.route('http://127.0.0.1:8000/',lambda r:r.fulfill(body=source,content_type='text/html'))
  await page.goto('http://127.0.0.1:8000/');await page.wait_for_function('!!window.__game')
  await page.evaluate('document.fonts.ready')
  await page.set_viewport_size({'width':1280,'height':800})
  portraits=[]
  for i in range(8):
   await page.evaluate('(i)=>{__test.career.fight=i;__game.careerIntro()}',i)
   await page.wait_for_timeout(120 if i else 650)
   portraits.append(await page.locator('.portrait').inner_html())
   await page.screenshot(path=f'shots/task12-opp-{i+1}.png')
  assert len(set(portraits))==8
  for w,h in [(1280,800),(844,390),(456,1024)]:
   await page.set_viewport_size({'width':w,'height':h})
   await page.evaluate('__game.startMatch({practice:true,vs:"2p",first:0})')
   await page.wait_for_function('__game.state.phase==="aim"')
   await page.evaluate('__game.setAim(0)');await page.wait_for_timeout(800)
   check=await page.evaluate('''()=>{const {held,trajectory,newBody,launch,releaseX,M,camera}=__test;const b=newBody(0);launch(b,releaseX(0),.5,0,0);return {gap:b.p.distanceTo(held.position),preview:trajectory[0].position.distanceTo(held.position),camera:camera.position.toArray()}}''')
   assert check['gap']<1e-9 and check['preview']<1e-9,check
   assert check['camera']==([0,5,14] if w/h<=.6 else [0,2.05,3.2])
   if w!=1280:
    sizes=await page.locator('#bag-menu button:visible').evaluate_all('(bs)=>bs.map(b=>({h:b.getBoundingClientRect().height,font:parseFloat(getComputedStyle(b).fontSize)}))')
    assert all(b['h']>=(44 if w==456 else 36) and b['font']>=(12 if w==456 else 10) for b in sizes),sizes
    await page.screenshot(path=f'shots/task12-menu-{w}x{h}.png')
   await page.screenshot(path=f'shots/task12-bag-{w}.png')
  for side in ['left','right']:
   await page.click(f'[data-box={side}]');await page.wait_for_timeout(60)
   assert await page.evaluate('__test.releaseX(0)')==(-.62 if side=='left' else .62)
   assert await page.evaluate('localStorage.getItem("backyard-cornhole-practice-box")')==side
   await page.screenshot(path=f'shots/task12-practice-{side}.png')
   gap=await page.evaluate('''()=>{const held=__test.held.position.toArray();__game.throwWith(.5);return Math.hypot(...__game.state.bags[0].p.map((x,i)=>x-held[i]));}''')
   assert gap<1e-9,gap
   await page.reload();await page.wait_for_function('!!window.__game')
   await page.click('#practice');await page.wait_for_function('__game.state.phase==="aim"')
   assert await page.locator(f'[data-box={side}]').get_attribute('aria-pressed')=='true'
  results=[]
  for level in range(8):
   r=await page.evaluate('''level=>{
    const {M,roster,wind,planCareerShot,newBody,launch,step,RX}=__test;
    M.phase='menu';M.scores=[0,1];M.position=[0,0];
    let seed=120031+level*1009;
    Math.random=()=>((seed=(Math.imul(seed,1664525)+1013904223)>>>0)/4294967296);
    const tally={name:roster[level].name,throws:400,in:0,on:0,miss:0,kobe:0};
    for(let n=0;n<tally.throws;n++){
     const mph=[0,0,3,3,7,7,10,10][level],a=(n%8)*Math.PI/4;
     wind.x=Math.sin(a)*mph*.025;wind.z=-Math.cos(a)*mph*.025;
     const c=planCareerShot(roster[level]);tally.kobe+=M.arc===2;
     const b=newBody(1,c.side);launch(b,RX[1],c.power,c.aim,c.drift,M.arc,M.spin);
     for(let j=0;j<1920;j++){step(b,1/240);if(['board','grass','in','removed'].includes(b.state))break;}
     if(b.state==='in')tally.in++;else if(b.state==='board')tally.on++;else tally.miss++;
    }return tally;
   }''',level)
   results.append(r);print(json.dumps(r),flush=True)
  Path('shots/task12-rates.json').write_text(json.dumps(results,indent=2))
  for i,r in enumerate(results):
   assert abs(r['in']/400-[.07,.15,.25,.35,.45,.55,.67,.80][i])<.065,r
   assert abs(r['on']/400-[.30,.35,.35,.35,.30,.30,.25,.15][i])<.065,r
  assert results[0]['in']/400<.10 and results[0]['kobe']==0
  assert not errors,errors
  await browser.close()
  print('TASK 12 PASS')
asyncio.run(asyncio.wait_for(main(),timeout=150))
