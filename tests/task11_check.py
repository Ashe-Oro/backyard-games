"""Menu layouts and seeded, CPU-only physics calibration (no scoring overrides)."""
import asyncio,json
from pathlib import Path
from playwright.async_api import async_playwright

async def main():
 async with async_playwright() as p:
  browser=await p.chromium.launch(args=['--no-sandbox'])
  page=await browser.new_page(ignore_https_errors=True)
  page.set_default_timeout(8000);page.set_default_navigation_timeout(12000)
  errors=[];page.on('pageerror',lambda e:errors.append(str(e)))
  source=Path('index.html').read_text().replace('window.__game = {','window.__cpuTest={M,roster,wind,planCareerShot,newBody,launch,step,RX,showMenu,scoreInning}; window.__game = {')
  await page.route('http://127.0.0.1:8000/',lambda r:r.fulfill(body=source,content_type='text/html'))
  await page.goto('http://127.0.0.1:8000/');await page.wait_for_function('!!window.__game')
  await page.evaluate('document.fonts.ready')
  assert await page.locator('.menu-modes small').all_text_contents()==[
   'HUMAN VS HUMAN · SAME SCREEN','JUST YOU · NO OPPONENT',
   'YOU VS CPU · CLIMB TO CORNHOLE KING']
  for w,h in [(1280,800),(844,390),(390,844)]:
   await page.set_viewport_size({'width':w,'height':h})
   await page.evaluate('''()=>{
    const buttons=[...document.querySelectorAll('.menu-modes button')];
    const bounds=buttons.map(b=>b.getBoundingClientRect());
    if(new Set(bounds.map(b=>b.width)).size!==1)throw Error('Unequal button widths');
    buttons.forEach((b,i)=>{
     const box=bounds[i],sub=b.querySelector('small'),r=sub.getBoundingClientRect();
     if(parseFloat(getComputedStyle(sub).fontSize)<9)throw Error('Small subtitle');
     if(box.left<0||box.right>innerWidth||box.top<0||box.bottom>innerHeight)throw Error('Button outside screen');
     if(r.left<box.left||r.right>box.right||r.bottom>box.bottom||sub.scrollWidth>sub.clientWidth)throw Error('Subtitle overflow');
     if(i && box.top<bounds[i-1].bottom)throw Error('Overlapping buttons');
    });
    const title=document.querySelector('#title').getBoundingClientRect();
    if(title.bottom>bounds[0].top)throw Error('Title overlap');
   }''')
   await page.screenshot(path=f'shots/task11-{w}.png')
  await page.click('#two-player');await page.wait_for_function('__game.state.phase==="aim" || __game.state.phase==="cpu"')
  assert await page.locator('#inn').inner_text()=='ROUND 1'
  await page.evaluate('__cpuTest.scoreInning()')
  assert await page.locator('#go').inner_text()=='NEXT ROUND'
  assert 'EQUAL ROUND' in await page.locator('#panel').inner_text()
  assert 'INNING' not in source
  # Independent throws on an empty board; eight wind bearings at normal circuit strengths.
  # Planner is run for every bag. All bags use the same launch/step functions as gameplay.
  results=[]
  for level in range(8):
   result=await page.evaluate('''level=>{
    const {M,roster,wind,planCareerShot,newBody,launch,step,RX}=__cpuTest;
    M.phase='menu';M.scores=[0,1];M.position=[0,0];
    let seed=110031+level*1009;
    Math.random=()=>((seed=(Math.imul(seed,1664525)+1013904223)>>>0)/4294967296);
    const tally={name:roster[level].name,throws:400,in:0,on:0,miss:0,kobe:0};
    for(let n=0;n<tally.throws;n++){
     const mph=[0,0,3,3,7,7,10,10][level],a=(n%8)*Math.PI/4;
     wind.x=Math.sin(a)*mph*.025;wind.z=-Math.cos(a)*mph*.025;
     const c=planCareerShot(roster[level]);tally.kobe+=M.arc===2;
     const b=newBody(1,c.side);
     launch(b,RX[1],c.power,c.aim,c.drift, M.arc);
     for(let j=0;j<1920;j++){step(b,1/240);if(['board','grass','in','removed'].includes(b.state))break;}
     if(b.state==='in')tally.in++;else if(b.state==='board')tally.on++;else tally.miss++;
    }return tally;
   }''',level)
   results.append(result);print(json.dumps(result),flush=True)
  Path('/tmp/task11-rates.json').write_text(json.dumps(results,indent=2))
  for i,r in enumerate(results):
   target_in=[.07,.15,.25,.33,.40,.45,.52,.60][i];target_on=[.30,.35,.35,.35,.30,.30,.30,.30][i]
   assert abs(r['in']/r['throws']-target_in)<.075,r
   assert abs(r['on']/r['throws']-target_on)<.075,r
  assert results[0]['in']/results[0]['throws']<.10 and results[0]['kobe']==0
  assert not errors,errors
  await browser.close()
  print('TASK 11: menu layouts, round terminology, CPU calibration PASS')
asyncio.run(asyncio.wait_for(main(),timeout=150))
