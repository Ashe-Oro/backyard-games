# Beer Pong tournament

Added a seven-round CPU tournament modeled on Cornhole’s ladder, opponent introductions, saved progress, and next-opponent flow. No other games changed.

## Behavior

- Start menu includes Tournament and shows the next unlocked round when returning.
- Seven unique rivals and simple symbol avatars: Sunny, Sprout, Disco, Comet, Shark, Voltage, and The Glove. Each introduction includes a personality line and the full ladder.
- `beer-pong/tournament.mjs` interpolates the existing rookie, regular, ace, and boss profiles across seven levels. Shooting precision, bounce frequency, and swat attention increase; shooting spread and reaction time decrease. The opening rival uses rookie tuning; The Glove uses the existing boss tuning.
- Tournament matches use the existing CPU match engine, including ten cups, re-racks, two-cup bounces, human defense, and CPU swats. Upward flick and pull-back input remain unchanged.
- Win cards advance with Next Round; loss cards retry the same rival with Try Again. Every card offers Main Menu. The final win offers a trophy/champion screen, which also permits replaying from Round 1.
- `tailgate-beer-pong-tournament-v1` stores the highest unlocked rival and completion flag. Returning or reloading resumes there; unfinished matches restart with a fresh rack. Replaying after completion keeps the highest unlock. Malformed saves fall back to Round 1; failed writes show a save warning.
- New controls are at least 48px high in the portrait phone layout. Existing desktop match sizing is unchanged. Player-facing progression says Round.

## Verification

Serve the repository with `python3 -m http.server 8012`.

The new browser suite runs directly with:

```sh
node tests/beer_pong_tournament_check.cjs
```

It launches `/usr/bin/google-chrome` and verifies the menu, unique ladder and avatars, monotonic difficulty, introduction, loss/retry, win/advance, persistence after reload, final champion, trophy persistence, menu return, and phone button sizing at 1280×800 and 390×844. Match endings are accelerated through test-only routing instrumentation that clears a rack and calls the production end-of-shot path; no forced-result hook ships in the game.

Existing suites use a temporary Playwright launch adapter when the bundled browser is absent:

```js
const {chromium}=require('playwright-core');
const launch=chromium.launch.bind(chromium);
chromium.launch=opts=>launch({...opts,executablePath:'/usr/bin/google-chrome',args:['--no-sandbox']});
```

Run each existing `tests/beer_pong*_check.cjs` and `tests/beer_pong*_check.mjs` with `NODE_OPTIONS=--require=/tmp/pong-chrome.cjs`. The main Beer Pong test’s menu count now expects four modes.

Screenshots: `shots/pong-tournament-{menu,intro,match,win,loss,champion}-{desktop,phone}.png`. Inspected the phone menu, intro, and match captures for fit and legibility.

Final results: all 12 Beer Pong JavaScript suites passed, including `beer_pong_throw_check.cjs` and the new tournament suite. The concurrent run hit the existing human-swat test’s narrow post-resume timing window; that suite passed when rerun alone. All other suites passed on the first run. The tournament suite passed again after the final CSS adjustment. Save validation was also checked with malformed, out-of-range, inconsistent, and inaccessible storage. `git diff --check` passes.

Touch devices also receive 44px swat targets and a 44px minimum for dialog buttons in landscape, using `pointer: coarse`; desktop mouse sizing stays unchanged.
