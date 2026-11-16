#!/usr/bin/env node
// Compares the C core (build/physics-probe) with the oracle vectors from the pinned arcade 1.0.3
// snapshot (tests/golden/arcade-1.0.3-physics.json). Tolerances (SPEC §5.1): identical state and
// transition sequence, positions within 1 mm, WIN bounds within one 0.0025 power step.
//   node wii/tests/physics_golden.mjs <physics-probe>
import {readFileSync, writeFileSync, mkdtempSync, rmSync} from 'node:fs';
import {spawnSync} from 'node:child_process';
import {tmpdir} from 'node:os';
import {dirname, join} from 'node:path';
import {fileURLToPath} from 'node:url';

const here = dirname(fileURLToPath(import.meta.url));
const probe = process.argv[2];
if (!probe) { console.error('usage: physics_golden.mjs <physics-probe>'); process.exit(2); }
const g = JSON.parse(readFileSync(join(here, 'golden', 'arcade-1.0.3-physics.json'), 'utf8'));
const SIDE = {slick: 'slick', sticky: 'sticky'};
const lines = ['win'];
for (const x of Object.keys(g.baseYaw)) lines.push(`yaw ${x}`);
for (const s of g.shots) lines.push(`shot ${s.team ?? 0} ${SIDE[s.side]} ${s.arc} ${s.power} ${s.rx} ${s.aim} ${s.drift} ${s.spin} ${s.mph ?? 0} ${s.angle ?? 0}`);
for (const s of g.scenes) lines.push(`scene ${s.resting.length} ${s.resting.map((r) => r.join(' ')).join(' ')} ${s.team ?? 0} ${s.side} ${s.arc} ${s.power} ${s.rx} ${s.aim} ${s.spin ?? 0}`);
for (const w of g.winds) lines.push(`wind ${w.circuit ?? -1} ${w.seed}`);
for (const p of g.plans) lines.push(`plan ${p.level} ${p.seed} ${p.deck.length} ${p.deck.map((d) => d.join(' ')).join(' ')} ${p.scores.join(' ')} ${p.mph} ${p.angle}`);
lines.push(`rng 42 ${g.rng42.length}`);

const dir = mkdtempSync(join(tmpdir(), 'byc-golden-'));
let failures = 0, checks = 0;
const fail = (msg) => { failures++; if (failures <= 25) console.error('FAIL', msg); };
const near = (a, b, tol, msg) => { checks++; if (!(Math.abs(a - b) <= tol)) fail(`${msg}: C ${a} vs JS ${b}`); };
const eq = (a, b, msg) => { checks++; if (a !== b) fail(`${msg}: C ${a} vs JS ${b}`); };
try {
  writeFileSync(join(dir, 'in.txt'), lines.join('\n') + '\n');
  const run = spawnSync(probe, [join(dir, 'in.txt'), join(dir, 'out.txt')], {stdio: 'inherit'});
  if (run.status !== 0) throw new Error(`probe exited ${run.status} ${run.error ?? ''}`);
  const out = readFileSync(join(dir, 'out.txt'), 'utf8').trim().split('\n').map((l) => l.split(' '));
  let k = 0;
  const sides = ['slick', 'sticky'];
  for (let n = 0; n < 12; n++) {
    const [, t, s, a, ...w] = out[k++];
    const js = g.win[`${t}${sides[s]}${a}`];
    ['lo', 'hi', 'olo', 'ohi', 'mid'].forEach((key, i) => near(+w[i], js[key], 0.0025 + 1e-9, `WIN ${t}${sides[s]}${a}.${key}`));
  }
  for (const x of Object.keys(g.baseYaw)) near(+out[k++][1], g.baseYaw[x], 1e-12, `baseYaw(${x})`);
  for (const s of g.shots) {
    const r = out[k++];
    const label = `shot ${s.side}/${s.arc} p=${s.power} rx=${s.rx} aim=${s.aim} drift=${s.drift} spin=${s.spin} wind=${s.mph ?? 0}`;
    eq(r[1], s.state, `${label} state`);
    for (let i = 0; i < 3; i++) near(+r[2 + i], s.p[i], 0.001, `${label} p[${i}]`);
    eq(!!+r[8], s.dead, `${label} dead`); eq(!!+r[9], s.offBoard, `${label} offBoard`);
    eq(!!+r[10], s.touched, `${label} touched`); eq(!!+r[11], s.swish, `${label} swish`);
    const trans = s.transitions.map(([i, ev, st]) => `${ev}:${st}`).join(';');
    const ctrans = r[13] === '-' ? '' : r[13].split(';').filter(Boolean).map((x) => x.split(':').slice(1).join(':')).join(';');
    eq(ctrans, trans, `${label} transitions`);
  }
  const names = ['fly', 'slide', 'board', 'ground', 'grass', 'hole', 'in', 'removed'];
  for (const s of g.scenes) {
    const r = out[k++]; const n = +r[2];
    eq(n, s.bags.length, 'scene bag count');
    for (let b = 0; b < n; b++) {
      const base = 3 + b * 5;
      eq(r[base], s.bags[b].state, `scene p=${s.power} arc=${s.arc} bag ${b} state`);
      if (!['in'].includes(s.bags[b].state)) for (let i = 0; i < 3; i++) near(+r[base + 1 + i], s.bags[b].lp[i], 0.001, `scene p=${s.power} bag ${b} lp[${i}]`);
    }
    void names;
  }
  for (const w of g.winds) {
    const r = out[k++];
    w.seq.forEach(([mph, angle], i) => { near(+r[1 + 2 * i], mph, 0, `wind c${w.circuit} r${i} mph`); near(+r[2 + 2 * i], angle, 1e-9, `wind c${w.circuit} r${i} angle`); });
  }
  for (const p of g.plans) {
    const r = out[k++]; const label = `plan ${p.style} deck=${p.deck.length} wind=${p.mph}`;
    eq(+r[1], p.arc, `${label} arc`); eq(r[2], p.side, `${label} side`); near(+r[3], p.spin, 1e-12, `${label} spin`);
    near(+r[4], p.power, 1e-9, `${label} power`); near(+r[5], p.aim, 1e-9, `${label} aim`);
    eq(r[6], p.goal, `${label} goal`); eq(+r[7], p.draws, `${label} rng draws`);
  }
  const rr = out[k++];
  g.rng42.forEach((v, i) => near(+rr[1 + i], v, 0, `rng42[${i}]`));
} finally { rmSync(dir, {recursive: true, force: true}); }
if (failures) { console.error(`physics golden: ${failures}/${checks} checks failed`); process.exit(1); }
console.log(`physics golden: ${checks}/${checks} checks passed (${g.shots.length} shots, ${g.scenes.length} scenes, ${g.plans.length} CPU plans, WIN, wind, rng)`);
