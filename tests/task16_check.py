"""Drawer persistence, hit targets and real touch gestures; bounded browser waits."""
import asyncio
from playwright.async_api import async_playwright

async def main():
 async with async_playwright() as p:
  browser=await p.chromium.launch(executable_path='/usr/bin/google-chrome',args=['--no-sandbox'])
  for width,height in [(456,1024),(1280,800)]:
   context=await browser.new_context(viewport={'width':width,'height':height},has_touch=True,is_mobile=width==456,ignore_https_errors=True)
   page=await context.new_page();page.set_default_timeout(5000);page.set_default_navigation_timeout(10000)
   errors=[];page.on('pageerror',lambda e:errors.append(str(e)))
   async def start():
    await page.goto('http://127.0.0.1:8000/');await page.wait_for_function('!!window.__game')
    await page.evaluate('__game.startMatch({practice:true,vs:"2p",first:0})')
    await page.wait_for_function('__game.state.phase==="aim"')
    await page.wait_for_function('!document.querySelector("#msg").classList.contains("on")')
   await start()
   handle=page.locator('#bag-handle')
   assert await handle.get_attribute('aria-expanded')=='true'
   rect=await handle.bounding_box();assert min(rect['width'],rect['height']) >= (44 if width==456 else 32)
   await page.screenshot(path=f'shots/task15-menu-expanded-{width}x{height}.png')
   await page.click('[data-height="2"]');await page.click('[data-side=sticky]')
   await handle.click();await page.wait_for_timeout(180)
   assert await page.locator('#bag-picks').inner_text()=='KOBE · STICKY'
   assert await page.evaluate('document.querySelector("#bag-menu").inert')
   rect=await handle.bounding_box();assert abs(rect['x'])<1
   await page.screenshot(path=f'shots/task15-menu-collapsed-{width}x{height}.png')
   # A touch in the panel's former footprint aims and releases a real throw.
   client=await context.new_cdp_session(page)
   async def touch(kind,x=110,y=height-140):
    await client.send('Input.dispatchTouchEvent',{'type':kind,'touchPoints':[] if kind=='touchEnd' else [{'x':x,'y':y}]})
   aim=await page.evaluate('__game.state.aim')
   await touch('touchStart');assert await page.evaluate('__game.state.aim')==aim
   await touch('touchMove',130,height-160)
   assert await page.evaluate('__game.state.aim')!=aim
   await page.wait_for_timeout(20)
   await touch('touchMove',130,height-290);await touch('touchEnd')
   assert await page.evaluate('__game.state.practiceStats.throws > 0 || __game.state.phase === "flying"')
   await start();assert await handle.get_attribute('aria-expanded')=='false'
   await handle.click();await page.wait_for_timeout(180)
   await page.click('[data-height="0"]');await page.click('[data-side=slick]')
   await start();assert await handle.get_attribute('aria-expanded')=='true'
   # Storage failures must not prevent toggling.
   await page.evaluate('() => {Storage.prototype.setItem=()=>{throw Error("blocked")};}')
   await handle.click();assert await handle.get_attribute('aria-expanded')=='false'
   assert not errors,errors
   await context.close()
  await browser.close()
 print('TASK 16 PASS: responsive drawer, picks, persistence, blocked storage and real touch throws')
asyncio.run(asyncio.wait_for(main(),60))
