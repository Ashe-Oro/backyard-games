import asyncio
from pathlib import Path
from playwright.async_api import async_playwright

async def main():
 async with async_playwright() as p:
  browser=await p.chromium.launch(args=['--no-sandbox'])
  page=await browser.new_page(ignore_https_errors=True,viewport={'width':1280,'height':800})
  page.set_default_timeout(8000)
  page.set_default_navigation_timeout(12000)
  errors=[]
  page.on('pageerror',lambda e:errors.append(str(e)))
  source=Path('index.html').read_text().replace('window.__game = {','window.__meterTest={M,renderHud,meterWindow,physicalPower}; window.__game = {')
  await page.route('http://127.0.0.1:8000/',lambda r:r.fulfill(body=source,content_type='text/html'))
  await page.goto('http://127.0.0.1:8000/')
  await page.wait_for_function('!!window.__game')
  await page.evaluate('__game.startMatch({practice:true,vs:"2p",first:0})')
  await page.wait_for_function('__game.state.phase==="aim"')
  # Both thrower calibrations, both bag sides, shifted and unshifted windows.
  await page.evaluate('''()=>{
   const {M,renderHud,meterWindow,physicalPower}=__meterTest;
   const near=(a,b)=>{if(Math.abs(a-b)>1e-9)throw Error(`${a} != ${b}`)};
   for(const t of [0,1])for(const side of ['slick','sticky'])for(const shift of [0,.24,.74])for(const arc of [0,1,2]){
    M.practice=false;M.first=t;M.k=0;M.side[t]=side;M.arc=arc;M.heights[t]=arc;M.sweetShift=shift;
    renderHud();
    const base=__game.win[t+side+arc],normal=__game.win[t+side+1],w=meterWindow();
    const nw=normal.hi-normal.lo;
    const width=arc===2 ? nw*.78 : base.hi-base.lo;
    near(w.hi-w.lo,shift ? Math.max(arc===2 ? nw*.60 : .014,width*.78) : width);
    near(w.mid,shift || base.mid);
    if(arc===2 && w.hi-w.lo<nw*.60)throw Error('KOBE floor');
    near(physicalPower(w.lo),base.lo);near(physicalPower(w.hi),base.hi);
    near(physicalPower(w.mid),base.mid);
    if(arc===2){near(physicalPower(w.olo),base.olo);near(physicalPower(w.ohi),base.ohi);}
    if(!(physicalPower(w.lo-.001)<base.lo && physicalPower(w.hi+.001)>base.hi))throw Error('boundary mapping');
    const meter=document.querySelector('#meter').getBoundingClientRect();
    const target=document.querySelector('#sweet').getBoundingClientRect();
    const css=getComputedStyle(document.querySelector('#meter'));
    const inner=meter.height-parseFloat(css.borderTopWidth)-parseFloat(css.borderBottomWidth);
    if(Math.abs(target.height/inner-(w.hi-w.lo))>.001)throw Error(JSON.stringify({target:target.height,meter:meter.height,w}));
   }
   M.first=0;M.k=0;M.side[0]='slick';M.sweetShift=0;M.arc=1;M.heights[0]=1;renderHud();
  }''')
  for arc,name in enumerate(['push','normal','kobe']):
   await page.click(f'[data-height="{arc}"]')
   await page.screenshot(path=f'shots/task10-{name}.png')
  assert not errors,errors
  await browser.close()
  print('TASK 10: KOBE width, tournament floor/shift, input mapping and exact visual bounds PASS')

asyncio.run(asyncio.wait_for(main(),timeout=150))
