"""Task 13 phone text and pooled throw effects; short bounded browser checks."""
import asyncio
from pathlib import Path
from playwright.async_api import async_playwright

async def main():
 async with async_playwright() as p:
  browser=await p.chromium.launch(executable_path='/usr/bin/google-chrome',args=['--no-sandbox'])
  source=Path('index.html').read_text().replace('window.__game = {','window.__test={M,toWorld,flightTrail,boardDust,confetti,scoreInning,afterThrow,bags}; window.__game = {')
  for w,h in [(390,844),(456,1024)]:
   page=await browser.new_page(viewport={'width':w,'height':h},device_scale_factor=2,ignore_https_errors=True)
   page.set_default_timeout(5000);page.set_default_navigation_timeout(10000)
   errors=[];page.on('pageerror',lambda e:errors.append(str(e)))
   await page.route('http://127.0.0.1:8000/',lambda r:r.fulfill(body=source,content_type='text/html'))
   await page.goto('http://127.0.0.1:8000/');await page.wait_for_function('!!window.__game');await page.evaluate('document.fonts.ready')
   await page.evaluate('__game.careerIntro()')
   await page.screenshot(path=f'shots/task13-phone-card-{w}.png')
   async def bounds():
    assert await page.locator('#panel').evaluate('(e)=>{const r=e.getBoundingClientRect();return r.left>=0 && r.right<=innerWidth && r.top>=0 && r.bottom<=innerHeight && e.scrollWidth<=e.clientWidth}')
   await bounds()
   await page.evaluate('__game.startMatch({vs:"2p",first:0})');await page.wait_for_function('__game.state.phase==="aim"')
   assert await page.locator('#hud').evaluate('(e)=>parseFloat(getComputedStyle(e).fontSize)')==10
   await page.screenshot(path=f'shots/task13-phone-hud-{w}.png')
   await page.evaluate('__game._setWind(0,0);__game.throwWith(.5)')
   await page.wait_for_function('__test.flightTrail.filter(c=>c.m.visible).length>=2')
   await page.wait_for_timeout(220)
   await page.screenshot(path=f'shots/task13-mid-flight-{w}.png')
   assert await page.evaluate('__test.flightTrail.length')==3
   # Use the real completion path with a resting scoring bag to inspect the brief puff.
   await page.evaluate('''()=>{const t=__test; const b=t.M.active;b.state='board';b.lp.set(.2,.03,-.4);t.toWorld(b.lp,b.p);t.afterThrow();}''')
   assert await page.evaluate('__test.boardDust.filter(c=>c.life>0).length')==14
   assert await page.evaluate('__test.confetti.every(c=>c.life===0)')
   await page.wait_for_timeout(60)
   await page.screenshot(path=f'shots/task13-board-puff-{w}.png')
   await page.wait_for_timeout(450)
   assert await page.evaluate('__test.boardDust.every(c=>!c.m.visible)')
   await page.wait_for_timeout(1200)
   await page.evaluate('''()=>{const t=__test;t.bags.forEach(b=>b.state='removed');__game._place(0,.2,-.4);__game._place(0,.1,-.3);__game._place(1,-.2,-.4);t.scoreInning();}''')
   await bounds()
   assert '2 − 1 = 1' in await page.locator('#panel').inner_text()
   await page.screenshot(path=f'shots/task13-phone-recap-{w}.png')
   assert not errors,errors
   await page.close()
  await browser.close()
 print('TASK 13 PASS')
asyncio.run(asyncio.wait_for(main(),timeout=45))
