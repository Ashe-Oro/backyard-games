// Run with Playwright Core available: node tests/framing_check.cjs
const { chromium } = require('playwright-core');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');

(async () => {
  const browser = await chromium.launch({ headless: true });
  try {
    const page = await browser.newPage();
    const errors = [];
    page.on('pageerror', error => errors.push(error.message));
    const source = fs.readFileSync(path.join(__dirname, '../index.html'), 'utf8');
    // Inspect geometry without adding production-only test interfaces.
    const instrumented = source.replace('window.__game = {', `
      window.__framing = () => {
        const bounds = object => {
          object.updateWorldMatrix(true, true);
          const box = new THREE.Box3().setFromObject(object);
          const points = [];
          for (const x of [box.min.x, box.max.x])
            for (const y of [box.min.y, box.max.y])
              for (const z of [box.min.z, box.max.z]) {
                const p = new THREE.Vector3(x,y,z).project(camera);
                points.push([(p.x+1)/2,(1-p.y)/2]);
              }
          return {left:Math.min(...points.map(p=>p[0])),right:Math.max(...points.map(p=>p[0])),
            top:Math.min(...points.map(p=>p[1])),bottom:Math.max(...points.map(p=>p[1]))};
        };
        const body = newBody(thrower());
        launch(body,releaseX(thrower()),.55,M.aim,0);
        return {bag:bounds(held),board:bounds(target.outer),visible:held.visible,
          camera:[...camera.position.toArray(),...camera.quaternion.toArray(),camera.fov],
          origin:body.p.toArray()};
      };
      window.__game = {`);
    const url = 'http://127.0.0.1:8012/';
    await page.route(url, route => route.fulfill({body:instrumented,contentType:'text/html'}));
    await page.goto(url);
    await page.waitForFunction(() => !!window.__game);
    await page.evaluate(() => __game.startMatch({practice:true,vs:'2p',first:0}));
    await page.waitForFunction(() => __game.state.phase === 'aim');
    for (const [width,height] of [[390,844],[456,872],[844,390],[1280,800],[1600,900],[2560,1080]]) {
      await page.setViewportSize({width,height});
      for (const side of ['right','left']) {
        // The drawer may be collapsed on a real phone; use its existing handler.
        await page.evaluate(side => document.querySelector('[data-box="'+side+'"]').click(), side);
        for (const aim of [-.12,0,.12]) {
          await page.evaluate(aim => __game.setAim(aim), aim);
          await page.evaluate(() => new Promise(resolve => requestAnimationFrame(() => requestAnimationFrame(resolve))));
          const view = await page.evaluate(() => __framing());
          assert(view.visible);
          for (const object of [view.bag,view.board]) {
            assert(object.left > .05 && object.right < .95, JSON.stringify({width,height,side,view}));
            assert(object.top > .15 && object.bottom < .9, JSON.stringify({width,height,side,view}));
          }
          assert(view.bag.top > view.board.bottom, 'Held bag must read as foreground');
          if (width/height >= 1.2) {
            assert(view.bag.top-view.board.bottom > .14, 'Wide screens need visible throwing distance between bag and board');
            assert(view.bag.bottom-view.bag.top < .10, 'Desktop bag must not dominate the court');
          }
          const center = (view.bag.left+view.bag.right)/2;
          const boardCenter = (view.board.left+view.board.right)/2;
          assert(Math.abs(center - boardCenter) < .02, 'Held bag must align with the board on both throwing sides');
          assert.deepEqual(view.origin,[side==='right'?.62:-.62,1.3,-.45]);
          if (aim===-.12) globalThis.fixedCamera = view.camera;
          else assert.deepEqual(view.camera,globalThis.fixedCamera,'Aiming must not move the camera');
          if (aim===0) console.log(`${width}x${height} ${side}: board height ${((view.board.bottom-view.board.top)*100).toFixed(1)}%, bag center ${center.toFixed(2)}`);
        }
      }
      if (process.env.SHOTS) {
        await page.evaluate(() => document.querySelector('[data-box="right"]').click());
        await page.evaluate(() => __game.setAim(0));
        if (width < 600) await page.evaluate(() => {if(document.querySelector('#bag-handle').getAttribute('aria-expanded')==='true')document.querySelector('#bag-handle').click();});
        await page.waitForTimeout(250);
        await page.screenshot({path:path.join(process.env.SHOTS,`framing-${width}x${height}.png`)});
      }
    }
    await page.setViewportSize({width:1600,height:900});
    await page.evaluate(() => {document.querySelector('[data-box="right"]').click();__game.setAim(0);});
    const sweet = await page.evaluate(() => __game.state.sweet.mid);
    await page.evaluate(power => __game.throwWith(power), sweet);
    await page.waitForFunction(() => __game.state.bags.length === 1);
    await page.waitForFunction(() => __game.state.phase === 'aim');
    assert.equal(await page.evaluate(() => __game.state.practiceStats.throws),1);
    assert.equal(await page.evaluate(() => __framing().visible),true);
    assert.deepEqual(errors,[]);
    console.log('Framing, fixed camera, release origins, and practice throw passed.');
  } finally { await browser.close(); }
})().catch(error => { console.error(error); process.exitCode = 1; });
