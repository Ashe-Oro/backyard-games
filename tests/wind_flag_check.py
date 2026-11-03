import asyncio, math
from playwright.async_api import async_playwright

async def main():
 async with async_playwright() as p:
  browser=await p.chromium.launch(args=['--no-sandbox'])
  page=await browser.new_page(ignore_https_errors=True,viewport={'width':1280,'height':800})
  errors=[]
  page.on('pageerror',lambda e:errors.append(str(e)))
  page.on('console',lambda m:errors.append(m.text) if m.type=='error' else None)
  await page.goto('http://127.0.0.1:8000');await page.wait_for_function('!!window.__game')
  await page.click('#two-player')
  for angle in [i*math.pi/4 for i in range(8)]:
   await page.evaluate('(a)=>__game._setWind(8,a)',angle)
   assert await page.locator('#wind').is_visible()
   assert await page.locator('#wind-speed').inner_text()=='8 MPH'
   assert await page.locator('#conditions #wind').count()==0
   rotation=await page.locator('#wind-arrow').evaluate('(e)=>e.style.transform')
   assert abs(float(rotation[7:-4])-angle)<.0001,rotation
   flag=await page.evaluate('__game._flag(0)');v=flag['vertices']
   # Tip's horizontal displacement is aligned with the actual physics vector.
   x,z=v[48],v[50];dx,dz=math.sin(angle),-math.cos(angle)
   assert x*dx+z*dz>.2
   assert abs(x*dz-z*dx)<.04
  await page.evaluate('__game._setWind(2,Math.PI/2)')
  low=await page.evaluate('__game._flag(0)')
  low_later=await page.evaluate('__game._flag(.1)')
  assert low['vertices']!=low_later['vertices']
  await page.evaluate('__game._setWind(12,Math.PI/2)')
  high=await page.evaluate('__game._flag(0)')
  assert high['extension']>low['extension'] and high['frequency']>low['frequency'] and high['amplitude']<low['amplitude']
  for w,h in [(1280,800),(390,844),(480,360)]:
   await page.set_viewport_size({'width':w,'height':h})
   r=await page.locator('#wind').bounding_box()
   assert r['x']>w/2 and r['x']+r['width']<=w and r['y']>=0 and r['y']+r['height']<h
   await page.screenshot(path=f'shots/wind-{w}.png')
  await page.evaluate('__game._setWind(0,0)')
  assert not await page.locator('#wind').is_visible()
  calm=await page.evaluate('__game._flag(0)');later=await page.evaluate('__game._flag(1)')
  assert calm['vertices']==later['vertices']
  assert abs(calm['vertices'][48])<.02 and abs(calm['vertices'][50])<.001 and calm['vertices'][49]<-.29
  await page.click('#menu');await page.click('#practice')
  assert not await page.locator('#wind').is_visible()
  assert not errors,errors
  print('Wind widget direction/speed, eight flag directions, stronger flutter/stiffness, calm droop, practice, responsive placement: PASS')
  await browser.close()
asyncio.run(main())
