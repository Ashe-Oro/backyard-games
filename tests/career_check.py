import asyncio, json
from playwright.async_api import async_playwright
URL='http://127.0.0.1:8000'
async def main():
 async with async_playwright() as p:
  b=await p.chromium.launch(args=['--no-sandbox'])
  errors=[]
  ctx=await b.new_context(ignore_https_errors=True,viewport={'width':1280,'height':800})
  page=await ctx.new_page()
  page.on('pageerror',lambda e:errors.append(str(e)))
  page.on('console',lambda m:errors.append(m.text) if m.type=='error' else None)
  await page.goto(URL);await page.wait_for_function('!!window.__game')
  await page.click('#career');await page.click('#career-go')
  assert 'NEXT UP: Slidin’ Steve' in await page.locator('#panel').inner_text()
  await page.screenshot(path='shots/career-intro.png')
  await page.click('#fight');await page.wait_for_function('__game.state.phase==="aim" || __game.state.phase==="cpu"')
  assert await page.evaluate('__game.state.wind.mph')==0
  # Scripted real throws, preset near-winning scores and resting bags via existing physics hooks.
  # Four misses per side leave a known board point; scoring still runs through the full inning.
  async def finish(winner):
   await page.evaluate(f'__game._setScores({20 if winner==0 else 0},{20 if winner==1 else 0});__game._place({winner},.26,-.3)')
   while await page.evaluate('__game.state.phase') not in ['over','summary']:
    if await page.evaluate('__game.state.phase') in ['aim','cpu']:await page.evaluate('__game.throwWith(0)')
    await page.wait_for_timeout(80)
   assert await page.evaluate('__game.state.winner')==winner
  await finish(0)
  print('Early win passed',flush=True)
  assert await page.evaluate('__game.career.fight')==1
  await page.reload();await page.wait_for_function('!!window.__game')
  assert await page.evaluate('__game.career.fight')==1
  await page.click('#career');await page.click('#career-go');assert 'Big Bag Barb' in await page.locator('#panel').inner_text()
  await page.click('#fight');await page.wait_for_function('__game.state.phase==="aim" || __game.state.phase==="cpu"')
  print('Persistence passed',flush=True)
  await finish(1);assert await page.evaluate('__game.career.fight')==1
  await page.click('#next-fight');assert 'NEXT UP: Big Bag Barb' in await page.locator('#panel').inner_text()
  # Seed unlocked saves to inspect later circuits and rematch portraits.
  for fight in [2,4,5,6,7]:
   await page.evaluate('(n)=>localStorage.setItem("backyard-cornhole-career-v1",JSON.stringify({fight:n,wins:n}))',fight)
   await page.reload();await page.wait_for_function('!!window.__game');await page.click('#career');await page.click('#career-go')
   assert await page.locator('.portrait').count()==1
   await page.click('#fight');await page.wait_for_function('__game.state.phase==="aim" || __game.state.phase==="cpu"')
   wind=await page.evaluate('__game.state.wind');assert wind['mph']>0
   await page.wait_for_timeout(1500);assert await page.evaluate('__game.state.wind')==wind
  await page.reload();await page.wait_for_function('!!window.__game');await page.click('#career');await page.click('#career-go');await page.click('#fight');await page.wait_for_function('__game.state.phase==="aim" || __game.state.phase==="cpu"')
  await finish(0);await page.click('#next-fight');assert 'CORNHOLE KING!' in await page.locator('#panel').inner_text()
  await page.screenshot(path='shots/career-champion.png')
  for width,height,name in [(1280,800,'desktop'),(480,360,'card')]:
   await page.set_viewport_size({'width':width,'height':height});await page.evaluate('localStorage.removeItem("backyard-cornhole-career-v1")');await page.reload();await page.wait_for_function('!!window.__game')
   await page.click('#career');await page.click('#career-go');await page.click('#fight');await page.wait_for_timeout(1100)
   await page.screenshot(path=f'shots/career-{name}.png')
   assert await page.locator('#wind').is_visible()
  mobile=await b.new_context(ignore_https_errors=True,**p.devices['iPhone 13']);mp=await mobile.new_page()
  mp.on('pageerror',lambda e:errors.append(str(e)))
  mp.on('console',lambda m:errors.append(m.text) if m.type=='error' else None)
  await mp.goto(URL);await mp.wait_for_function('!!window.__game');await mp.click('#career');await mp.click('#career-go');await mp.screenshot(path='shots/career-mobile.png');await mp.click('#fight');await mp.wait_for_timeout(1800)
  assert await mp.evaluate('__game.state.wind.mph')==0
  print('Career start, scripted win/loss, persistence, rematches, circuit wind, champion, desktop/card/mobile: PASS');print('Console errors:',errors);assert not errors
  await b.close()
asyncio.run(main())
