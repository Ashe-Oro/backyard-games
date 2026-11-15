Backyard Cornhole Arcade — 1.0.8

# Spinner input diagnosis — 1.0.8

Verified in the copied 1.0.8 source before changes:

- resources-app/dist/site/arcade/controller.js frame(): reset Trackball on every phase/inning/bag/thrower key change. This synthesized motion after flight and could erase a forward stroke begun before the frame noticed aim.
- resources-app/dist/site/arcade/trackball.mjs Trackball.reset(): set settling and lastMotion=now. tick() required 450 ms of quiet afterward. feed() updated lastMotion while discarding settling/spent reports.
- controller.js feed(): returned before observing non-aim motion. A burst crossing aim could therefore be measured from its tail.
- Trackball.feed(): the opening report has no known duration and is excluded from speed. Valid windows require 16 ms and eight counts. Missing opening reports shift the 80 ms window; duplicate opening timestamps have no interval.
- controller.js nativeEvent(): clamped hardware time to receipt performance.now(), potentially collapsing distinct timestamps. cabinet-input.py already emits evdev report timestamps; unchanged.

Fix: TurnInput observes all valid reports and preserves real quiet history across phase changes. sync() arms a legal human turn immediately after real quiet; frame() no longer resets on turn keys. A burst begun outside legal aim is quarantined until real quiet, including its entire tail. This deliberately selects the allowed clean-ignore policy rather than buffering/replaying unknown residual motion. Synthetic settling without actual motion can accept an intentional stroke immediately. Spent residual motion still requires actual quiet. Native event time is preserved without receipt-time clamping.

Reports with identical timestamps cannot establish physical speed without guessing a device report rate. TurnInput quarantines that entire gesture rather than fabricating speed or later emitting a weak tail. Distinct hardware timestamps delivered together retain their original intervals. The existing 80 ms interpolated peak sampler, full-power 6048, aim and spin formulas remain unchanged.

The held bag has a softly pulsing amber halo only during legal human aim with an accepting ready/backswing machine. No text is added. Cabinet status logs were not used as motion evidence.

Limitation: ambiguous same-timestamp strokes are cleanly ignored, not reconstructed into a speed. Cabinet hardware verification remains for the caller.

Copied from 1.0.7 preserving app -> resources-app. No install, commit, tag or push performed.

Validation: node --test tests/*.test.mjs passes 6 file suites (all 64 individual tests pass with --test-isolation=none). Both requested commands exit 0. node --check passes controller.js, trackball.mjs, edited test modules, top-level arcade/controller.js and extracted executable inline Cornhole scripts. Import-map JSON is excluded from JavaScript parsing. git diff --check passes. Existing assertions remain; controller VM fixtures now supply the real TurnInput gate.

Files changed within this copy: resources-app/package.json; resources-app/dist/site/arcade/controller.js and trackball.mjs; resources-app/dist/site/cornhole/index.html; tests/controller.test.mjs and readiness.test.mjs; READ-ME.txt, DEV-NOTES.txt, CODEX_SUMMARY.md. Earlier snapshots, vendor assets, runtime files, web and Wii sources are unchanged.
