# Beer Pong: Tournament mode

Ashe (Nov 4 2026): "beer pong game needs a tournament mode."

Model it on the Cornhole career/tournament in this repo (cornhole/ — read how its ladder, opponent cards, progress save, and "Next Round" flow work) but keep it simple and native to Beer Pong.

Requirements:
1. New "Tournament" entry on the Beer Pong start menu next to Practice / CPU / Two Player, with a one-line subtitle (e.g. "Beat the bracket vs CPU").
2. A ladder of about 6–8 unique CPU opponents, each with their own name, short personality line, and unique simple avatar (no reused characters, no "II" rematches). Difficulty ramps steadily, ending in a boss (e.g. "The Glove"). Build on beer-pong/opponents.mjs profiles: interpolate shooting accuracy, bounce-shot rate, and swat attention/reaction from easy to hard. The first opponent should be beatable by a new player; the last should be genuinely tough.
3. Each match uses the normal Beer Pong rules already in the game (cups, re-racks, bounce = two cups, swats). Show an opponent intro card before each match and a win/lose result card after: on win, "Next Round" advances; on loss, "Try Again" replays the same opponent; a menu button returns to the menu.
4. Save progress (highest unlocked opponent) in localStorage; allow continuing. Beating the final boss shows a champion screen.
5. Player-facing text uses "Round", never "inning". Keep the game flick-only (upward flick or pull-back). Big tap targets on phone (>=44px). Desktop sizing unchanged elsewhere.
6. Add tests/beer_pong_tournament_check.cjs (headless Chrome like the other pong tests; see codex-summary-pong-throw.md for how to run with /usr/bin/google-chrome) covering: menu entry, intro card, difficulty increases across the ladder, win advances, loss retries, progress persists across reload, champion screen. All Beer Pong JS tests + beer_pong_throw_check.cjs must pass.
7. Screenshots shots/pong-tournament-*.png (desktop and 390x844 phone) and codex-summary-pong-tournament.md. Don't touch other games.
