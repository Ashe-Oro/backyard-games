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
    await page.addInitScript(()=>{try{localStorage.setItem('backyard-cornhole-help-seen','1');}catch{}});
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
        bagGeo.computeBoundingBox();
        return {bag:bounds(held),board:bounds(target.outer),visible:held.visible,
          camera:[...camera.position.toArray(),...camera.quaternion.toArray(),camera.fov],
          origin:body.p.toArray(),velocity:body.v.toArray(),heldPosition:held.position.toArray(),
          heldScale:held.scale.toArray(),
          bagDimensions:bagGeo.boundingBox.getSize(new THREE.Vector3()).toArray()};
      };
      window.__releaseContinuity = () => {
        const b = M.active;
        return {gap:b.mesh.position.distanceTo(held.position),angle:b.mesh.quaternion.angleTo(held.quaternion),
          scaleGap:b.mesh.scale.distanceTo(held.scale),sameGeometry:b.mesh.geometry===held.geometry};
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
      for (const team of [0,1]) {
        await page.evaluate(first => __game.startMatch({practice:false,vs:'2p',first}), team);
        await page.waitForFunction(() => __game.state.phase === 'aim');
        for (const aim of [-.12,0,.12]) {
          await page.evaluate(aim => __game.setAim(aim), aim);
          await page.evaluate(() => new Promise(resolve => requestAnimationFrame(() => requestAnimationFrame(resolve))));
          const view = await page.evaluate(() => __framing());
          assert(view.visible);
          for (const object of [view.bag,view.board]) {
            assert(object.left > .05 && object.right < .95, JSON.stringify({width,height,team,view}));
            assert(object.top > .15 && object.bottom < .9, JSON.stringify({width,height,team,view}));
          }
          assert(view.bag.top > view.board.bottom, 'Held bag must read as foreground');
          if (width/height >= 1.2) {
            assert(view.bag.top-view.board.bottom > .10, 'Wide screens need visible throwing distance between bag and board');
            assert(view.bag.bottom-view.bag.top < .12, 'Desktop bag must not dominate the court');
          }
          const center = (view.bag.left+view.bag.right)/2;
          const boardCenter = (view.board.left+view.board.right)/2;
          assert(Math.abs(center - boardCenter) < .02, 'Held bag must align with the board for both teams');
          assert.deepEqual(view.origin,[0,1.3,-.45]);
          assert.deepEqual(view.heldPosition,view.origin,'Held bag must occupy the real release position');
          assert.equal(await page.locator('[role=meter]').count(),0);
          assert.deepEqual(view.heldScale,[1,1,1],'Held bag must use physical scale');
          assert(Math.abs(view.bagDimensions[0]-.1524)<1e-6 && Math.abs(view.bagDimensions[2]-.1524)<1e-6,
            'Rendered bags must match the six-inch physical dimensions');
          if (aim===0) assert.equal(view.velocity[0],0,'Straight throws must travel along the centerline');
          if (aim===-.12) globalThis.fixedCamera = view.camera;
          else assert.deepEqual(view.camera,globalThis.fixedCamera,'Aiming must not move the camera');
          if (aim===0) console.log(`${width}x${height} team ${team}: board height ${((view.board.bottom-view.board.top)*100).toFixed(1)}%, bag center ${center.toFixed(2)}`);
        }
      }
      if (process.env.SHOTS) {
        await page.evaluate(() => __game.setAim(0));
        if (width < 600) await page.evaluate(() => {if(document.querySelector('#bag-handle').getAttribute('aria-expanded')==='true')document.querySelector('#bag-handle').click();});
        await page.waitForTimeout(800);
        await page.screenshot({path:path.join(process.env.SHOTS,`framing-${width}x${height}.png`)});
      }
    }
    await page.setViewportSize({width:1600,height:900});
    assert.equal(await page.locator('[data-box]').count(),0,'Remove obsolete side-launch controls');
    await page.evaluate(() => __game.startMatch({practice:true,vs:'2p',first:0}));
    await page.waitForFunction(() => __game.state.phase === 'aim');
    await page.evaluate(() => __game.setAim(0));
    const sweet = await page.evaluate(() => __game.state.sweet.mid);
    if (process.env.SHOTS) {
      await page.waitForTimeout(800);
      await page.screenshot({path:path.join(process.env.SHOTS,'throw-ready.png')});
    }
    const release = await page.evaluate(power => {__game.throwWith(power);return {position:__game.state.bags.at(-1).p,continuity:__releaseContinuity()};}, sweet);
    assert.deepEqual(release.position,[0,1.3,-.45], 'Actual practice throw must spawn in the center');
    assert.equal(release.continuity.gap,0,'Release must not teleport the displayed bag');
    assert(release.continuity.angle<1e-6,'Release must preserve the displayed bag angle');
    assert.equal(release.continuity.scaleGap,0);
    assert(release.continuity.sameGeometry);
    assert.equal(await page.locator('#reset-bags').isEnabled(),false,'Cannot clear during an active throw');
    for (const milliseconds of [60,120,240]) {
      await page.waitForTimeout(60);
      const active = await page.evaluate(() => __game.state.bags.at(-1));
      assert(Math.abs(active.p[0])<.02,'Straight flight must remain aligned after release');
      assert(active.p[2]<-.45,'The displayed bag must move forward from its release point');
      if (process.env.SHOTS) await page.screenshot({path:path.join(process.env.SHOTS,'throw-flight-'+milliseconds+'.png')});
    }
    await page.waitForFunction(() => __game.state.bags.length === 1);
    await page.waitForFunction(() => __game.state.phase === 'aim');
    assert.equal(await page.evaluate(() => __game.state.practiceStats.throws),1);
    assert.equal(await page.evaluate(() => __framing().visible),true);
    await page.evaluate(() => {__game._place(0,0,-.4);__game._place(0,.12,-.8);
      if(document.querySelector('#bag-handle').getAttribute('aria-expanded')==='true')document.querySelector('#bag-handle').click();});
    const statsBeforeClear = await page.evaluate(() => __game.state.practiceStats);
    const clearBoard = page.getByRole('button',{name:'CLEAR BOARD',exact:true});
    assert(await clearBoard.isVisible(), 'Practice clear button must remain visible with options collapsed');
    assert(await clearBoard.isEnabled());
    await clearBoard.click();
    assert.equal(await page.evaluate(() => __game.state.bags.length),0,'Clear board must remove all thrown bags');
    assert.deepEqual(await page.evaluate(() => __game.state.practiceStats),statsBeforeClear,'Clearing must preserve practice stats');
    assert.equal(await clearBoard.isEnabled(),false,'Empty board must disable clear button');
    assert.equal(await page.evaluate(() => __game.state.phase),'aim','Clearing must leave practice ready to throw');
    if(process.env.SHOTS) await page.screenshot({path:path.join(process.env.SHOTS,'practice-clear-board.png')});
    await page.setViewportSize({width:390,height:844});
    await page.evaluate(() => __game._place(0,0,-.4));
    await clearBoard.click();
    assert.equal(await page.evaluate(() => __game.state.bags.length),0,'Mobile clear button must work with options collapsed');
    assert.deepEqual(await page.evaluate(() => __game.state.practiceStats),statsBeforeClear);
    if(process.env.SHOTS) await page.screenshot({path:path.join(process.env.SHOTS,'practice-clear-board-mobile.png')});
    await page.setViewportSize({width:1600,height:900});
    await page.evaluate(() => __game.startMatch({practice:false,vs:'2p',first:1}));
    await page.waitForFunction(() => __game.state.phase === 'aim');
    assert.equal(await clearBoard.isVisible(),false,'Clear board must be exclusive to practice');
    const blueRelease = await page.evaluate(() => {__game._setWind(0,0);__game.setAim(0);__game.throwWith(__game.state.sweet.mid);return __game.state.bags.at(-1).p;});
    assert.deepEqual(blueRelease,[0,1.3,-.45], 'Actual blue-team throw must spawn in the center');
    await page.waitForFunction(() => __game.state.phase === 'aim');
    assert.deepEqual(errors,[]);
    console.log('Framing, fixed camera, release origins, and practice throw passed.');
  } finally { await browser.close(); }
})().catch(error => { console.error(error); process.exitCode = 1; });
