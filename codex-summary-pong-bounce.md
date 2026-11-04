# Beer Pong power-derived bounce shots

Removed the ARC/BOUNCE buttons, styling, selection state, and handlers. No shot-style setting or shortcut remains.

In practice and versus CPU, human power below 45% produces the low, one-bounce trajectory; 45% and above produces a direct arc. Each band maps power to its own launch-speed range, so distance still changes within each band. The soft band maps to the existing bounce physics’ 0–0.6 power range, and the firm band to the full direct range. Both styles can reach all ten original cups. Direct shots also remain reachable after re-racks. Local two-player retains its existing direct-only, table-contact-is-dead rules.

The split corresponds to about a 73 px gesture on desktop and 44 px on a portrait phone (varies with available space). Upward flicks and downward pulls use the same split, independently of gesture speed. Existing shot feedback now includes BOUNCE or ARC, with no new button.

Bounce scoring still requires a real clean rebound and awards two cups. Second bounces end the shot. Direct human arcs cannot rebound or be swatted. CPU shot selection and trajectory powers are unchanged.

## Harder swatting

Human swats require the bounced ball to cross the 60 cm midfield interval from table center +0.15 m to +0.75 m, about 0.2 seconds for a typical incoming ball. Previously the window continued across the entire near half. Hit radius is 18 px on desktop (previously 38), or 22 px on phones (previously 44); the visible target matches the new diameter. The 250 ms swing cooldown prevents retrying within most crossings. Close, timely taps/swipes and keyboard swats still work; pre-bounce, off-target, late, and sinking-ball attempts fail.

These changes aim at the requested 30–40% success for decent attempts through tighter timing and precision; that human success rate needs playtesting and is not claimed as measured by automated tests.

The fixed-seed CPU simulation showed the boss was nearly automatic at 997/1000 blocks. Reduced only its attention from 99.5% to 94%, yielding 937/1000. Other profiles remain unchanged: rookie 22, regular 550, ace 927 blocks per 1000.

## Verification

Served the repository with python3 -m http.server 8012. Used the existing temporary /tmp/pong-chrome.cjs adapter to launch Playwright with /usr/bin/google-chrome --no-sandbox, since the bundled browser is absent.

Run every tests/beer_pong*_check.cjs and tests/beer_pong*_check.mjs with NODE_OPTIONS=--require=/tmp/pong-chrome.cjs node TEST_PATH (240-second limit per suite). Capture screenshots with SHOTS=shots NODE_OPTIONS=--require=/tmp/pong-chrome.cjs node tests/beer_pong_throw_check.cjs.

All 11 Beer Pong JavaScript suites passed with Google Chrome, including the expanded throw regression. The final bounce suite also passed after the CPU attention adjustment. git diff --check passes.

Updated bounce, natural bounce, feedback, input, throw, human swat, and CPU swat checks. The throw regression now exercises both power bands with upward flicks and downward pulls using actual desktop mouse and native phone touch events. Human defense checks include a 30 px off-target miss, a close hit, and a centered swipe after the timing window while the ball is still flying.

Screenshots: shots/pong-bounce-power-{bounce,arc}-{flick,pull}-{mouse,touch}.png (eight images). Inspected the portrait soft-flick screenshot for feedback visibility and removed-control layout.

Only Beer Pong, its tests, these screenshots, and this summary were changed.
