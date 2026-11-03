import asyncio
from pathlib import Path
from playwright.async_api import async_playwright

async def main():
 async with async_playwright() as p:
  browser=await p.chromium.launch(args=['--no-sandbox'])
  page=await browser.new_page(ignore_https_errors=True)
  page.set_default_timeout(8000)
  page.set_default_navigation_timeout(12000)
  errors=[]
  page.on('pageerror',lambda e:errors.append(str(e)))
  source=Path('index.html').read_text().replace('window.__game = {','window.__view={camera,aimDots,held}; window.__game = {')
  await page.route('http://127.0.0.1:8000/',lambda r:r.fulfill(body=source,content_type='text/html'))
  await page.goto('http://127.0.0.1:8000/')
  await page.wait_for_function('!!window.__game')
  expected=None
  for width,height in [(1280,800),(390,844),(480,360),(844,390)]:
   await page.set_viewport_size({'width':width,'height':height})
   await page.evaluate('__game.startMatch({practice:true,vs:"2p",first:0})')
   await page.wait_for_function('__game.state.phase==="aim"')
   expected=None
   views=[]
   for side,x in [('left',2),('right',width-2)]:
    await page.mouse.move(x,height*.5)
    await page.wait_for_timeout(650)
    view=await page.evaluate('''()=>{const {camera,aimDots,held}=__view;return {position:camera.position.toArray(),target:camera.userData.look.toArray(),quaternion:camera.quaternion.toArray(),fov:camera.fov,aim:__game.state.aim,dot:aimDots[3].position.toArray(),held:held.rotation.y};}''')
    fixed={k:view[k] for k in ['position','target','quaternion','fov']}
    assert view['position']==[0,2.05,3.2] and view['target']==[0,.5,-8.7] and view['fov']>=19,view
    if expected is None: expected=fixed
    assert fixed==expected,(fixed,expected)
    views.append(view)
    await page.screenshot(path=f'shots/task9-{width}-{side}.png')
   assert views[0]['aim']<0<views[1]['aim'],views
   assert views[0]['dot']!=views[1]['dot'] and views[0]['held']!=views[1]['held'],views
  assert not errors,errors
  await browser.close()
  print('Batch 9: fixed camera position, target and rotation; responsive FOV stays fixed across pointer extremes; moving indicators and held bag PASS')

if __name__=='__main__':
 asyncio.run(asyncio.wait_for(main(),timeout=150))
