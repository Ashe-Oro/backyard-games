"""Task 17: tighter framing, visual-only bags and unchanged release/aim behavior."""
import asyncio,json
from pathlib import Path
from playwright.async_api import async_playwright

async def main():
 async with async_playwright() as p:
  browser=await p.chromium.launch(executable_path='/usr/bin/google-chrome',args=['--no-sandbox'])
  context=await browser.new_context(ignore_https_errors=True,has_touch=True,device_scale_factor=2)
  page=await context.new_page()
  source=Path('index.html').read_text()
  hook='window.__view={camera,held,trajectory,newBody,launch,releaseX,target,THREE,bagGeo,HOLE_R}; window.__game = {'
  await page.route('http://127.0.0.1:8000/',lambda r:r.fulfill(body=source.replace('window.__game = {',hook),content_type='text/html'))
  errors=[];page.on('pageerror',lambda e:errors.append(str(e)))
  await page.goto('http://127.0.0.1:8000/');await page.wait_for_function('!!window.__game');await page.evaluate('document.fonts.ready')
  results=[]
  for w,h in [(390,844),(456,1024),(844,390),(1280,800)]:
   await page.set_viewport_size({'width':w,'height':h})
   await page.evaluate('__game.startMatch({practice:true,vs:"2p",first:0})')
   await page.wait_for_function('__game.state.phase==="aim"')
   for side in ['left','right']:
    await page.click(f'[data-box={side}]');await page.wait_for_timeout(80)
    data=await page.evaluate('''()=>{const {camera:c,held,trajectory,newBody,launch,releaseX,target,THREE:T,bagGeo,HOLE_R}=__view;c.updateMatrixWorld();const pt=v=>{v=v.clone().project(c);return {x:(v.x+1)*innerWidth/2,y:(1-v.y)*innerHeight/2}};const b=newBody(0);launch(b,releaseX(0),.5,0,0);let radius=0;const p=bagGeo.attributes.position;for(let i=0;i<p.count;i++)radius=Math.max(radius,Math.hypot(p.getX(i),p.getZ(i)));return {bag:pt(held.position),gap:b.p.distanceTo(held.position),preview:trajectory[0].position.distanceTo(held.position),radius,hole:HOLE_R,board:[-.3048,.3048].flatMap(x=>[0,-1.2192].map(z=>pt(target.deckG.localToWorld(new T.Vector3(x,0,z))))),camera:[c.position.toArray(),c.quaternion.toArray(),c.fov]}}''')
    assert data['gap']<1e-9 and data['preview']<1e-9,data
    assert data['radius']*.53<data['hole'],data
    assert .2*w<data['bag']['x']<.8*w and .55*h<data['bag']['y']<.92*h,data
    assert max(v['x'] for v in data['board'])-min(v['x'] for v in data['board'])> (55 if w<500 else 70),data
    await page.screenshot(path=f'shots/task17-{w}x{h}-{side}.png')
    fixed=data['camera']
    await page.mouse.move(w*.5,h*.55);await page.mouse.down();await page.wait_for_timeout(40)
    before=await page.evaluate('__game.state.aim')
    await page.mouse.move(w*.5,h*.5)
    assert await page.evaluate('__game.state.aim')==before
    await page.mouse.up()
    assert await page.evaluate('JSON.stringify([__view.camera.position.toArray(),__view.camera.quaternion.toArray(),__view.camera.fov])')==json.dumps(fixed,separators=(',',':'))
    results.append({'viewport':[w,h],'side':side,**data})
  assert not errors,errors
  Path('shots/task17-projections.json').write_text(json.dumps(results,indent=2))
  await browser.close()
  print('TASK 17 PASS: framing, real release, visual fit, static camera, aim stability, four DPR2 captures')
asyncio.run(asyncio.wait_for(main(),timeout=60))
