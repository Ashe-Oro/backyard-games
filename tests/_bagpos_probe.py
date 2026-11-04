import asyncio, sys, json
from pathlib import Path
from playwright.async_api import async_playwright

VIEWPORTS = [(390, 844), (456, 1024), (844, 390), (1280, 800)]

async def main():
 async with async_playwright() as p:
  browser = await p.chromium.launch(args=['--no-sandbox'])
  page = await browser.new_page(ignore_https_errors=True, device_scale_factor=2)
  page.set_default_timeout(8000)
  page.set_default_navigation_timeout(12000)
  source = Path('index.html').read_text().replace(
      'window.__game = {', 'window.__view={camera,held,wrap}; window.__game = {')
  await page.route('http://127.0.0.1:8012/', lambda r: r.fulfill(body=source, content_type='text/html'))
  await page.goto('http://127.0.0.1:8012/')
  await page.wait_for_function('!!window.__game')
  results = []
  for width, height in VIEWPORTS:
    await page.set_viewport_size({'width': width, 'height': height})
    await page.evaluate('__game.startMatch({practice:true,vs:"2p",first:0})')
    await page.wait_for_function('__game.state.phase==="aim"')
    for side in ['left', 'right']:
      await page.click(f'[data-box="{side}"]')
      await page.wait_for_timeout(120)
      data = await page.evaluate('''() => {
        const {camera, held, wrap} = __view;
        const v = held.position.clone().project(camera);
        return {
          x: (v.x+1)*wrap.clientWidth/2,
          y: (1-v.y)*wrap.clientHeight/2,
          w: wrap.clientWidth, h: wrap.clientHeight,
          heldPos: held.position.toArray(),
          camPos: camera.position.toArray(),
          camFov: camera.fov,
        };
      }''')
      data['vp'] = f'{width}x{height}'
      data['side'] = side
      data['xfrac'] = round(data['x']/data['w'], 3)
      data['yfrac'] = round(data['y']/data['h'], 3)
      results.append(data)
  await browser.close()
  for r in results:
    print(f"{r['vp']:>10} {r['side']:>5}  x={r['xfrac']:.3f} y={r['yfrac']:.3f}  held={[round(c,3) for c in r['heldPos']]}  cam={[round(c,3) for c in r['camPos']]} fov={r['camFov']:.1f}")

if __name__ == '__main__':
  asyncio.run(asyncio.wait_for(main(), timeout=150))
