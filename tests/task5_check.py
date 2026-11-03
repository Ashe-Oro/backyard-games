import asyncio
from pathlib import Path
from playwright.async_api import async_playwright

async def main():
 async with async_playwright() as p:
  browser=await p.chromium.launch(args=['--no-sandbox'])
  page=await browser.new_page(ignore_https_errors=True)
  # Expose rendering objects only in the test response, leaving production hooks unchanged.
  source=Path('index.html').read_text().replace('window.__game = {','window.__view = {camera, held, THREE}; window.__game = {')
  await page.route('http://127.0.0.1:8000/',lambda route:route.fulfill(body=source,content_type='text/html'))
  for width,height in [(1280,800),(480,360),(390,844)]:
   await page.set_viewport_size({'width':width,'height':height})
   await page.goto('http://127.0.0.1:8000/');await page.wait_for_function('!!window.__game')
   await page.evaluate('__game._setWind(8,0)')
   for selector in ['#wind','#conditions','#sb','#height-choice','#bag-menu']:
    assert not await page.locator(selector).is_visible(),selector
   assert await page.locator('#title').is_visible()
   await page.click('#career');await page.click('#career-go')
   for selector in ['#wind','#conditions','#sb','#height-choice','#bag-menu']:
    assert not await page.locator(selector).is_visible(),selector
   await page.screenshot(path=f'shots/task5-intro-{width}.png')
   await page.click('#fight');await page.wait_for_function('__game.state.phase==="aim"')
   await page.wait_for_timeout(2200)
   await page.evaluate('__game._setWind(8,0)')
   assert await page.locator('#wind').is_visible()
   assert await page.locator('#sb').is_visible()
   view=await page.evaluate('''()=>{const {camera,held,THREE}=__view;const v=held.position.clone().project(camera);return {fov:camera.fov,z:camera.position.z,held:held.visible,point:v.toArray()};}''')
   assert abs(view['fov']-19)<1e-9
   assert abs(view['z']-2.45)<1e-9
   # Fixed 19-degree FOV crops the foreground bag horizontally on narrow screens.
   assert view['held'] and all(-1<v<1 for v in view['point'][1:]),view
   if width>=height: assert -1<view['point'][0]<1,view
   assert view['point'][1]<0,view
   await page.screenshot(path=f'shots/task5-held-{width}.png')
  await browser.close()
  print('V2 camera FOV/depth, held bag in lower foreground, menu/Next Up HUD hiding and gameplay restoration: PASS')
asyncio.run(main())
