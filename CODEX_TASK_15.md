# Codex Task 15: the aim-path preview must not change when the finger touches down

Owner, on phone: the dotted line showing the bag's potential path looks one way when no thumb is on the screen, then changes as soon as the thumb touches down. It shouldn't. The preview must be consistent and depend only on the selected settings: throw height (PUSH / NORMAL / KOBE), aim and the pitcher's box (plus SLICK/STICKY if it affects the slide), so it gives an honest idea of the throw before and during the touch.
- Find what changes on pointerdown/touchstart: the power assumption, the aim re-centered to the touch x, a "dragging" state using a different arc, wind, and so on. Make the idle preview and the touching preview use the same function and inputs. If the preview assumes a power, use one fixed reference power, e.g. the sweet-spot center, in both states. Don't let touch-down alter the aim unless the finger actually moves horizontally.
- Switching PUSH/NORMAL/KOBE must visibly change the preview arc shape right away, without needing a touch.
- Keep the preview dots near the ground and on the path. Don't add new UI.
- Test: headless with touch emulation at 456x1024. Capture the preview dot world positions idle and immediately after touchstart (without moving), and assert they're equal. Repeat for each height.
- Keep all earlier batch behavior. Use short test timeouts. Save screenshots to shots/task15-*.png. Don't write to .git. Write a summary to codex-summary-15.md.

