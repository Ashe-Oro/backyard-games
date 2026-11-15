"""Portrait framing, real release projection, static aim and desktop regression."""
import asyncio,json
from pathlib import Path
from playwright.async_api import async_playwright

async def main():
 async with async_playwright() as p:
  browser=await p.chromium.launch(executable_path='/usr/bin/google-chrome',args=['--no-sandbox'])
  page=await browser.new_page(ignore_https_errors=True,device_scale_factor=1)
  page.set_default_timeout(5000);page.set_default_navigation_timeout(10000)
  errors=[];page.on('pageerror',lambda e:errors.append(str(e)))
  source=Path('index.html').read_text().replace('window.__game = {','window.__view={camera,held,trajectory,THREE,CAM_POS,CAM_LOOK,newBody,launch,releaseX}; window.__game = {')
  await page.route('http://127.0.0.1:8000/',lambda r:r.fulfill(body=source,content_type='text/html'))
  await page.goto('http://127.0.0.1:8000/');await page.wait_for_function('!!window.__game');await page.evaluate('document.fonts.ready')
  results=[]
  for w,h in [(456,1024),(390,844),(1280,800)]:
   await page.set_viewport_size({'width':w,'height':h})
   await page.evaluate('__game.startMatch({practice:true,vs:"2p",first:0})')
   await page.wait_for_function('__game.state.phase==="aim"')
   for side in ['left','right']:
    await page.click(f'[data-box={side}]');await page.wait_for_timeout(80)
    data=await page.evaluate('''()=>{const {camera:c,held,trajectory,THREE:T,newBody,launch,releaseX}=__view;c.updateMatrixWorld();const pt=v=>{v=v.clone().project(c);return {x:(v.x+1)*innerWidth/2,y:(1-v.y)*innerHeight/2}};const b=newBody(0);launch(b,releaseX(0),.5,0,0);return {bag:pt(held.position),gap:b.p.distanceTo(held.position),preview:trajectory[0].position.distanceTo(held.position),board:[-.3048,.3048].flatMap(x=>[0,1.2192].map(z=>pt(new T.Vector3(x,.1016+z*Math.sin(Math.atan( .2032/1.2192)),-8.2296-z*Math.cos(Math.atan(.2032/1.2192))))))}}''')
    assert data['gap']<1e-9 and data['preview']<1e-9,data
    if w!=1280:
     menu=await page.locator('#bag-menu').bounding_box()
     assert .25*w<=data['bag']['x']<=.75*w,data
     assert .55*h<=data['bag']['y']<menu['y']-12,data
     xs=[v['x'] for v in data['board']];ys=[v['y'] for v in data['board']]
     assert min(xs)>0 and max(xs)<w and min(ys)>0 and max(ys)<menu['y'],data
     assert max(xs)-min(xs)>=w*.10 and max(ys)-min(ys)>=h*.025,data
     await page.screenshot(path=f'shots/task14-{w}x{h}-{side}.png')
    else:
     assert await page.evaluate('''()=>{const {camera:c,CAM_POS,CAM_LOOK,THREE:T}=__view;const q=new T.Quaternion();const m=new T.Matrix4().lookAt(CAM_POS,CAM_LOOK,c.up);q.setFromRotationMatrix(m);return c.position.equals(CAM_POS)&&c.quaternion.equals(q)&&c.fov===8}''')
    async def camera():
     return await page.evaluate('''()=>{const c=__view.camera;return [c.position.toArray(),c.quaternion.toArray(),c.fov]}''')
    fixed=await camera()
    for x in [2,w-2]:
     await page.mouse.move(x,h*.48);await page.wait_for_timeout(80)
     assert await camera()==fixed
    results.append({'viewport':[w,h],'side':side,**data})
  assert not errors,errors
  Path('shots/task14-projections.json').write_text(json.dumps(results,indent=2))
  await browser.close()
  print('TASK 14 PASS: portrait projections, menu clearance, board size, release alignment, static camera and exact desktop framing')
asyncio.run(asyncio.wait_for(main(),timeout=45))
