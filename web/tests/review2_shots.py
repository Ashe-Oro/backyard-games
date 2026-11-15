import asyncio, sys
from playwright.async_api import async_playwright

VIEWPORTS = [
    ('1280', 1280, 800),
    ('844x390', 844, 390),
    ('390x844', 390, 844),
]

async def capture(p, tag, w, h):
    browser = await p.chromium.launch(args=['--no-sandbox'])
    page = await browser.new_page(viewport={'width': w, 'height': h})
    errors = []
    page.on('pageerror', lambda e: errors.append(str(e)))
    page.on('console', lambda m: errors.append(m.text) if m.type == 'error' else None)
    await page.goto('http://127.0.0.1:8011', timeout=12000)
    await page.wait_for_function('!!window.__game', timeout=8000)
    await page.wait_for_timeout(400)

    # 1. Main menu
    await page.screenshot(path=f'shots/review2/01-menu-{tag}.png')

    # 2. Tournament opponent card (career intro) - wait for title ghost to fully fade
    await page.evaluate('__game.careerIntro()')
    await page.wait_for_timeout(700)
    await page.screenshot(path=f'shots/review2/02-opponent-card-{tag}.png')

    # 3. Solo Practice while aiming, mid-drag so power meter is visible (wait out the coin-toss banner)
    await page.evaluate('__game.startMatch({vs:"2p", practice:true, first:0})')
    await page.wait_for_timeout(900)
    await page.wait_for_function('__game.state.phase === "aim"', timeout=8000)
    await page.wait_for_timeout(1600)
    cx, cy = w / 2, h * 0.82
    await page.mouse.move(cx, cy)
    await page.mouse.down()
    for frac in [0.78, 0.68, 0.58]:
        await page.mouse.move(cx, h * frac)
        await page.wait_for_timeout(30)
    await page.wait_for_timeout(80)
    await page.screenshot(path=f'shots/review2/03-aiming-practice-{tag}.png')

    # 4. Release the flick -> mid-flight
    await page.mouse.move(cx, h * 0.30)
    await page.wait_for_timeout(20)
    await page.mouse.up()
    await page.wait_for_timeout(110)
    await page.screenshot(path=f'shots/review2/04-mid-flight-{tag}.png')

    # 5. After landing (wait for settle + result message)
    await page.wait_for_timeout(850)
    await page.screenshot(path=f'shots/review2/05-after-landing-{tag}.png')

    # 6. Round-end / summary screen: use a real (non-practice) 2P match, fast-forward the inning
    await page.evaluate('__game.startMatch({vs:"2p", exact:false})')
    await page.wait_for_timeout(900)
    for _ in range(80):
        phase = await page.evaluate('__game.state.phase')
        if phase == 'summary' or phase == 'over':
            break
        if phase == 'aim':
            await page.evaluate('__game.throwWith(0.55)')
        await page.wait_for_timeout(150)
    await page.wait_for_timeout(350)
    await page.screenshot(path=f'shots/review2/06-round-end-{tag}.png')

    # 7. 2 Player turn change: fresh 2p match, throw once, capture the HUD at the turn switch
    await page.evaluate('__game.startMatch({vs:"2p", exact:false})')
    await page.wait_for_timeout(900)
    await page.wait_for_function('__game.state.phase === "aim"', timeout=8000)
    before = await page.evaluate('__game.state.thrower')
    await page.evaluate('__game.throwWith(0.55)')
    await page.wait_for_timeout(900)
    await page.wait_for_function('__game.state.phase === "aim"', timeout=8000)
    after = await page.evaluate('__game.state.thrower')
    await page.screenshot(path=f'shots/review2/07-turn-change-{tag}.png')
    print(tag, 'thrower before/after', before, after, 'errors:', errors[:5])

    await browser.close()

async def main():
    async with async_playwright() as p:
        for tag, w, h in VIEWPORTS:
            await capture(p, tag, w, h)

asyncio.run(main())
