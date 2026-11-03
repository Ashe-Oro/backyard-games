import asyncio,json
from playwright.async_api import async_playwright
async def main():
 async with async_playwright() as p:
  browser=await p.chromium.launch(args=['--no-sandbox'])
  page=await browser.new_page(ignore_https_errors=True,viewport={'width':1280,'height':800})
  errors=[]
  page.on('pageerror',lambda e:(print(str(e),flush=True),errors.append(str(e))))
  page.on('console',lambda m:errors.append(m.text) if m.type=='error' else None)
  await page.goto('http://127.0.0.1:8000');await page.wait_for_function('!!window.__game')
  print('windows',await page.evaluate('__game.win'))
  await page.evaluate('__game.startMatch({vs:"2p",first:0})');await page.wait_for_timeout(1100)
  await page.keyboard.down('Space');await page.wait_for_timeout(300);await page.keyboard.up('Space');await page.keyboard.press('Enter')
  assert await page.evaluate('__game.state.bags.length')==0
  await page.keyboard.press('ArrowLeft');await page.keyboard.press('q');await page.keyboard.press('w')
  st=await page.evaluate('__game.state');assert st['position'][0]<0 and st['spin']==-1 and st['arc']==2
  await page.keyboard.press('ArrowRight');await page.keyboard.press('e');await page.keyboard.press('s')
  await page.mouse.move(640,600);await page.mouse.down()
  for y in [550,490,420,340]:
   await page.mouse.move(640,y);await page.wait_for_timeout(25)
  await page.mouse.up();await page.wait_for_timeout(100)
  assert await page.evaluate('__game.state.bags.length')==1
  await page.screenshot(path='shots/desktop.png')
  probes=await page.evaluate('[0,1,2].map(a=>[__game._probe(.55,a,-1),__game._probe(.55,a,1)])')
  assert probes[2][0]['flight']>probes[0][0]['flight']+.4
  assert max(abs(a['p'][0]-b['p'][0]) for a,b in probes)>.005
  print('arc/spin landing probes',probes)
  await page.wait_for_function('__game.state.phase==="aim"')
  await page.evaluate('__game._place(1,0,-.65);__game._slide(0,-.42,0,-2)')
  await page.wait_for_timeout(700)
  b=await page.evaluate('__game.state.bags');assert b[1]['lp'][2]<-.66,b
  wind=await page.evaluate('__game.state.wind')
  # Play out an inning with real physics and cancellation scoring.
  while await page.evaluate('__game.state.phase')!='summary':
   if await page.evaluate('__game.state.phase')=='aim':await page.evaluate('__game.throwWith(0)')
   await page.wait_for_timeout(200)
  await page.evaluate('__game.advance()');await page.wait_for_timeout(400)
  assert await page.evaluate('__game.state.wind')==wind
  assert await page.locator('#wind').is_visible()
  # No-button trackpad wheel throw.
  await page.mouse.wheel(0,-250);await page.wait_for_timeout(200)
  assert await page.evaluate('__game.state.bags.length')==1
  await page.wait_for_function('__game.state.phase==="aim"')
  await page.keyboard.press('m')
  await page.mouse.move(640,650)
  for y in [590,510,410,290]:
   await page.mouse.move(640,y);await page.wait_for_timeout(20)
  await page.wait_for_timeout(150)
  assert await page.evaluate('__game.state.bags.length')==2
  await page.set_viewport_size({'width':480,'height':360});await page.screenshot(path='shots/card.png')
  mobile=await browser.new_context(ignore_https_errors=True,**p.devices['iPhone 13']);mp=await mobile.new_page()
  mp.on('console',lambda m:errors.append(m.text) if m.type=='error' else None)
  mp.on('pageerror',lambda e:(print(str(e),flush=True),errors.append(str(e))))
  await mp.goto('http://127.0.0.1:8000');await mp.wait_for_function('!!window.__game')
  await mp.evaluate('__game.startMatch({vs:"2p",first:0})');await mp.wait_for_timeout(1100)
  cdp=await mobile.new_cdp_session(mp)
  await cdp.send('Input.dispatchTouchEvent',{'type':'touchStart','touchPoints':[{'x':195,'y':460}]})
  for y in [430,390,340,280]:
   await cdp.send('Input.dispatchTouchEvent',{'type':'touchMove','touchPoints':[{'x':195,'y':y}]});await mp.wait_for_timeout(30)
  await cdp.send('Input.dispatchTouchEvent',{'type':'touchEnd','touchPoints':[]})
  assert await mp.evaluate('__game.state.bags.length')==1
  await mp.screenshot(path='shots/mobile.png')
  await mp.set_viewport_size({'width':844,'height':390});await mp.wait_for_timeout(200)
  await page.goto('http://127.0.0.1:8000')
  await page.evaluate("""document.body.innerHTML='<iframe sandbox="allow-scripts allow-same-origin" src="/?card" width="480" height="360"></iframe>'""")
  frame=page.frames[-1];await frame.wait_for_function('!!window.__game')
  await frame.evaluate('__game.startMatch({vs:"2p",first:0})');await page.wait_for_timeout(1100)
  assert await frame.evaluate('__game.throwWith(.55)')
  print('errors',errors);assert not errors
  await browser.close()
asyncio.run(main())
