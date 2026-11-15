"""Reference preview stays identical through touch-down for every height and box."""
import asyncio,json
from pathlib import Path
from playwright.async_api import async_playwright

async def main():
 async with async_playwright() as p:
  browser=await p.chromium.launch(executable_path='/usr/bin/google-chrome',args=['--no-sandbox'])
  page=await browser.new_page(viewport={'width':456,'height':1024},has_touch=True,is_mobile=True,ignore_https_errors=True)
  page.set_default_timeout(5000);page.set_default_navigation_timeout(10000)
  errors=[];page.on('pageerror',lambda e:errors.append(str(e)))
  source=Path('index.html').read_text().replace('window.__game = {','window.__preview={aimDots,trajectory}; window.__game = {')
  await page.route('http://127.0.0.1:8000/',lambda r:r.fulfill(body=source,content_type='text/html'))
  await page.goto('http://127.0.0.1:8000/');await page.wait_for_function('!!window.__game')
  await page.evaluate('__game.startMatch({practice:true,vs:"2p",first:0})')
  await page.wait_for_function('__game.state.phase==="aim"')
  client=await page.context.new_cdp_session(page)
  async def touch(kind,x=340,y=600):
   await client.send('Input.dispatchTouchEvent',{'type':kind,'touchPoints':[] if kind=='touchCancel' else [{'x':x,'y':y}]})
  async def snapshot():
   await page.evaluate('new Promise(r=>requestAnimationFrame(()=>requestAnimationFrame(r)))')
   return await page.evaluate('({aim:__game.state.aim,dots:Object.fromEntries(Object.entries(__preview).map(([k,v])=>[k,v.map(d=>({p:d.position.toArray(),visible:d.visible}))]))})')
  results=[]
  for box in ['left','right']:
   await page.click(f'[data-box={box}]')
   assert await page.get_attribute(f'[data-box={box}]','aria-pressed')=='true'
   for side in ['slick','sticky']:
    await page.click(f'[data-side={side}]')
    shapes=[]
    for height,name in enumerate(['push','normal','kobe']):
     await page.click(f'[data-height="{height}"]');await page.evaluate('__game.setAim(.02)')
     idle=await snapshot();shapes.append(idle['dots'])
     await touch('touchStart');down=await snapshot()
     assert idle==down,(box,side,name,idle,down)
     if box=='left' and side=='slick':
      await page.screenshot(path=f'shots/task15-{name}-touch.png')
     await touch('touchMove',340,520)
     assert await snapshot()==idle,'Vertical flick changed reference preview'
     await touch('touchMove',360,520)
     assert (await snapshot())['aim']!=idle['aim'],'Horizontal movement did not aim'
     await touch('touchCancel')
     await page.evaluate('__game.setAim(.02)')
     assert await snapshot()==idle,(box,side,name,idle,await snapshot())
     if box=='left' and side=='slick':
      await page.screenshot(path=f'shots/task15-{name}-idle.png')
     results.append({'box':box,'side':side,'height':name,'idle':idle,'touchstart':down})
    assert all(shapes[i]!=shapes[j] for i in range(3) for j in range(i)), 'Height did not immediately change arc'
  assert not errors,errors
  Path('shots/task15-preview-positions.json').write_text(json.dumps(results,indent=2))
  await browser.close()
 print('TASK 15 PASS: exact idle/touch equality for all heights, sides and boxes; vertical/horizontal gesture behavior; immediate arc changes')
asyncio.run(asyncio.wait_for(main(),timeout=60))
