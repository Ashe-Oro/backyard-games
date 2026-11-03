import asyncio
from pathlib import Path
from playwright.async_api import async_playwright

async def main():
 async with async_playwright() as p:
  browser=await p.chromium.launch(args=['--no-sandbox'])
  page=await browser.new_page(ignore_https_errors=True)
  page.set_default_timeout(8000)
  errors=[]
  page.on('pageerror',lambda e:errors.append(str(e)))
  source=Path('index.html').read_text().replace('window.__game = {','window.__view={camera,CAM_LOOK,THREE}; window.__game = {')
  await page.route('http://127.0.0.1:8000/',lambda r:r.fulfill(body=source,content_type='text/html'))
  for width,height in [(1280,800),(480,360),(390,844),(844,390)]:
   await page.set_viewport_size({'width':width,'height':height})
   await page.goto('http://127.0.0.1:8000/');await page.wait_for_function('!!window.__game')
   await page.evaluate('__game.startMatch({vs:"2p",first:0})')
   await page.wait_for_timeout(1800)
   assert await page.locator('#n0').inner_text()=='RED'
   assert 'RED RED' not in await page.locator('#t0').inner_text()
   assert await page.locator('#hint').count()==0
   await page.click('#more')
   assert await page.locator('#more').get_attribute('aria-expanded')=='true'
   # Every button belongs to one panel; scrolling is allowed, overlap is not.
   assert await page.locator('#bag-menu [data-height]').count()==3
   rects=await page.locator('#bag-menu button').evaluate_all('(bs)=>bs.filter(b=>b.getClientRects().length).map(b=>{const r=b.getBoundingClientRect();return {x:r.x,y:r.y,w:r.width,h:r.height};})')
   for i,a in enumerate(rects):
    for b in rects[i+1:]:
     assert min(a['x']+a['w'],b['x']+b['w'])-max(a['x'],b['x'])<1 or min(a['y']+a['h'],b['y']+b['h'])-max(a['y'],b['y'])<1
   await page.click('[data-side="sticky"]');await page.click('[data-spin="0"]');await page.click('[data-height="2"]')
   assert await page.locator('[data-side="sticky"]').get_attribute('aria-pressed')=='true'
   assert await page.locator('[data-spin="0"]').get_attribute('aria-pressed')=='true'
   await page.evaluate('__game.setAim(0)');await page.wait_for_timeout(1500)
   neutral=await page.evaluate('__view.camera.position.toArray()')
   assert abs(neutral[0]-.5)<.001 and neutral[1:]==[2.05,2.45],neutral
   for aim in [-.12,.12]:
    await page.evaluate('(a)=>__game.setAim(a)',aim)
    await page.wait_for_timeout(1800)
    view=await page.evaluate('''()=>{const {camera,THREE}=__view;return {x:camera.position.x,look:camera.userData.look.x,board:[-.3048,.3048].flatMap(x=>[-8.23,-9.45].map(z=>new THREE.Vector3(x,.3,z).project(camera).toArray()))}}''')
    assert (view['x']-.5)*aim>0 and view['look']*aim>0
    assert all(abs(v[0])<1 and abs(v[1])<1 for v in view['board']),view
   await page.keyboard.press('ArrowRight');await page.wait_for_timeout(1500)
   assert await page.evaluate('__view.camera.position.x')>.5+.12*1.2
   await page.screenshot(path=f'shots/task6-{width}.png')
   await page.click('#menu');await page.click('#practice')
   assert await page.locator('#n0').inner_text()=='RED'
  assert not errors,errors
  await browser.close()
  print('TASK_6 panel layout/selections, labels, removed hint, neutral v2 framing, camera aim/step following and board bounds: PASS')
asyncio.run(main())
