import asyncio
from playwright.async_api import async_playwright
async def main():
 async with async_playwright() as p:
  browser=await p.chromium.launch(args=['--no-sandbox'])
  page=await browser.new_page(ignore_https_errors=True,viewport={'width':1280,'height':800})
  errors=[]
  page.on('pageerror',lambda e:errors.append(str(e)))
  page.on('console',lambda m:errors.append(m.text) if m.type=='error' else None)
  await page.goto('http://127.0.0.1:8000');await page.wait_for_function('!!window.__game')
  assert await page.locator('#panel button').evaluate_all('(buttons)=>buttons.map(b=>b.firstChild.textContent)')==['2 PLAYER','SOLO PRACTICE','START TOURNAMENT']
  await page.click('#two-player');await page.wait_for_function('__game.state.phase==="aim"')
  assert not await page.locator('#touch-controls').is_visible()
  assert await page.locator('[data-height]').all_text_contents()==['PUSH','NORMAL','KOBE']
  assert await page.locator('[data-height="1"]').get_attribute('aria-pressed')=='true'
  first=await page.evaluate('__game.state.thrower')
  await page.click('[data-height="2"]');await page.click('[data-side="sticky"]')
  assert await page.evaluate('__game.state.heights[__game.state.thrower]')==2
  await page.wait_for_timeout(1200)
  left=await page.evaluate('__game._screenPoint(-.3048,-8.8)')
  right=await page.evaluate('__game._screenPoint(.3048,-8.8)')
  # Task 17 tightens the former ~128px desktop board span.
  assert 145 < right['x']-left['x'] < 160
  target=await page.evaluate('__game._screenPoint(.12,-8.8)')
  await page.mouse.move(target['x'],target['y'])
  actual=await page.evaluate('__game.state.aim')
  assert abs(actual)<.12
  # A board-height horizontal pointer sweep gives smooth, ordered aim values.
  aims=[]
  for x in [600,620,640,660,680]:
   await page.mouse.move(x,450);aims.append(await page.evaluate('__game.state.aim'))
  assert all(a<b for a,b in zip(aims,aims[1:])),aims
  await page.evaluate('__game.throwWith(0)');await page.wait_for_function('__game.state.phase==="aim"')
  assert await page.evaluate('__game.state.arc')==1
  await page.evaluate('__game.throwWith(0)');await page.wait_for_function('__game.state.phase==="aim"')
  assert await page.evaluate('__game.state.arc')==2
  assert await page.evaluate('(t)=>__game.state.side[t]',first)=='sticky'
  await page.click('#menu');await page.click('#practice');await page.wait_for_function('__game.state.phase==="aim"')
  assert await page.evaluate('__game.state.wind.mph')==0
  for i in range(9):
   await page.evaluate('__game.throwWith(0)');await page.wait_for_function('__game.state.phase==="aim"')
  st=await page.evaluate('__game.state');assert st['practiceStats']['throws']==9 and st['thrower']==0
  await page.click('#reset-bags');assert await page.evaluate('__game.state.bags.length')==0
  assert await page.evaluate('__game.state.practiceStats.throws')==9
  await page.screenshot(path='shots/task3-practice.png')
  mids=[]
  for fight in [0,2,5,6]:
   await page.evaluate('(n)=>localStorage.setItem("backyard-cornhole-career-v1",JSON.stringify({fight:n,wins:n}))',fight)
   await page.reload();await page.wait_for_function('!!window.__game');await page.click('#career');await page.click('#career-go');await page.click('#fight');await page.wait_for_function('__game.state.phase==="aim"')
   st=await page.evaluate('__game.state');w=st['sweet'];mids.append(w['mid'])
   base=await page.evaluate('__game.win[__game.state.thrower+"slick1"]')
   if fight==0:assert w['lo']==base['lo']
   else:assert w['hi']-w['lo']<base['hi']-base['lo'] and w['mid']!=base['mid']
   if fight==6:
    old=w['mid'];await page.evaluate('__game.throwWith(0)');await page.wait_for_function('__game.state.phase==="aim"')
    assert await page.evaluate('__game.state.sweet.mid')!=old
  assert len(set(mids))==4
  await page.screenshot(path='shots/task3-sweet.png')
  for width,height in [(480,360),(390,844)]:
   await page.set_viewport_size({'width':width,'height':height})
   for sel in ['#height-choice']:
    r=await page.locator(sel).bounding_box();assert r['x']>=0 and r['x']+r['width']<=width+1
   await page.screenshot(path=f'shots/task3-{width}.png')
  assert not errors,errors
  print('TASK_3 modes, controls, aim, per-player height, unlimited practice, reset, moving sweet spots, responsive HUD: PASS')
  await browser.close()
asyncio.run(main())
