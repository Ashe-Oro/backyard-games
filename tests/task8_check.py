import asyncio
from playwright.async_api import async_playwright

async def main():
 async with async_playwright() as p:
  browser=await p.chromium.launch(args=['--no-sandbox'])
  page=await browser.new_page(ignore_https_errors=True)
  page.set_default_timeout(8000)
  page.set_default_navigation_timeout(12000)
  errors=[]
  page.on('pageerror',lambda e:errors.append(str(e)))
  await page.goto('http://127.0.0.1:8000')
  await page.wait_for_function('!!window.__game')
  for width,height in [(1280,800),(390,844),(480,360),(844,390)]:
   await page.set_viewport_size({'width':width,'height':height})
   await page.evaluate('__game.startMatch({practice:true,vs:"2p",first:0})')
   await page.wait_for_function('__game.state.phase==="aim"')
   mid=await page.locator('#sb .mid').bounding_box()
   assert abs(mid['x']+mid['width']/2-width/2)<1,mid
   assert len((await page.locator('#goal').inner_text()).splitlines())==2
   if height<=450:
    for b in await page.locator('#bag-menu button:visible').all():
     box=await b.bounding_box()
     assert box['height']>=36,box
     assert await b.evaluate('e=>parseFloat(getComputedStyle(e).fontSize)')>=9
   await page.evaluate('__game._setWind(8,Math.PI/2)')
   assert await page.locator('#wind-arrow').evaluate('e=>e.tagName')=='CANVAS'
   assert await page.locator('#wind-arrow').evaluate('e=>e.getContext("2d").getImageData(5,1,1,1).data[3]')==255
   await page.screenshot(path=f'shots/task8-{width}.png')
  await page.evaluate('__game.startMatch({vs:"2p",first:0})')
  await page.wait_for_function('__game.state.phase==="aim"')
  for index,arc in enumerate([0,2,0,2]):
   if index<2: await page.click(f'[data-height="{arc}"]')
   state=await page.evaluate('__game.state')
   assert state['arc']==arc and state['heights'][state['thrower']]==arc,state
   assert await page.locator(f'[data-height="{arc}"]').get_attribute('aria-pressed')=='true'
   await page.evaluate('__game.throwWith(0)')
   await page.wait_for_function('__game.state.phase==="wait"')
   state=await page.evaluate('__game.state')
   expected=state['heights'][state['thrower']]
   assert state['arc']==expected,state
   assert await page.locator(f'[data-height="{expected}"]').get_attribute('aria-pressed')=='true'
   await page.wait_for_function('__game.state.phase==="aim"')
  assert not errors,errors
  await browser.close()
  print('Batch 8: centered practice stats, landscape controls, pixel arrow, per-player heights PASS')
asyncio.run(main())
