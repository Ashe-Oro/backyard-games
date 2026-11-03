# Codex Task 9: static camera while aiming

The owner says that when the aim line (the line showing where the bag will go) is moved far left or right, the background/camera pans. That is wrong: the background must stay completely static while aiming.

- Remove the batch-6 "camera follows aim" behavior entirely. Restore the fixed v2 camera: CAM_POS (0.5, 2.05, 2.45), look-at (0, 0.5, -8.7), fov 19. It must not change with the aim angle, the pointer position, or anything else during aiming.
- Only the aim line/indicator (and the held bag if it already turns with aim) should move with the pointer.
- Check for any other camera drift tied to aim (lerps, look-at offsets, resize handlers) and remove it.
- Keep everything else from batches 6-8 unchanged (left menu, PUSH/NORMAL/KOBE, SLICK/STICKY, no step/aim/spin, no MORE, no bottom hint).
- Add or adjust a Playwright check: take a screenshot with the aim at far left and one at far right, and assert that the camera position and target are identical (compare the camera values via a page eval).
- Use short test timeouts (8s waits, 12s navigation, 150s per suite), as in batch 8.
- Do not write to .git. Write a summary to codex-summary-9.md.
