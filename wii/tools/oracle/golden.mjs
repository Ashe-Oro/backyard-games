#!/usr/bin/env node
// Read-only deterministic oracle (SPEC §5.1.1). Slices the pinned arcade 1.0.3 snapshot's
// cornhole/index.html exactly like arcade/cabinet-snapshots/1.0.3/tests/spin.test.mjs does and
// evaluates it with node:vm against the snapshot's vendored three.js. No game file is edited.
//
//   node wii/tools/oracle/golden.mjs            writes wii/tests/golden/arcade-1.0.3-physics.json
//   node wii/tools/oracle/golden.mjs --check    exits 1 if the committed vectors differ
import {readFileSync, writeFileSync} from 'node:fs';
import {createHash} from 'node:crypto';
import {dirname, join, relative} from 'node:path';
import {fileURLToPath, pathToFileURL} from 'node:url';
import {runInNewContext} from 'node:vm';

const here = dirname(fileURLToPath(import.meta.url));
const repo = join(here, '..', '..', '..');
const site = join(repo, 'arcade', 'cabinet-snapshots', '1.0.3', 'resources-app', 'dist', 'site');
const files = {
  game: join(site, 'cornhole', 'index.html'),
  spin: join(site, 'cornhole', 'spin.mjs'),
  controls: join(site, 'cornhole', 'controls.mjs'),
  three: join(site, 'vendor', 'three.module.min.js'),
};
const THREE = await import(pathToFileURL(files.three).href);
const {AIR_SPIN_RATE, landingCurl, slideVelocity} = await import(pathToFileURL(files.spin).href);
const {roundWind} = await import(pathToFileURL(files.controls).href);
const source = readFileSync(files.game, 'utf8');

function physics() {
  const ctx = {THREE, AIR_SPIN_RATE, landingCurl, slideVelocity, clamp: (v, a, b) => Math.max(a, Math.min(b, v))};
  const body = source.slice(source.indexOf('function newBody('), source.indexOf('// calibrated power windows'));
  const constants = source.slice(source.indexOf('const G ='), source.indexOf('const TEAM ='));
  const cpu = source.slice(source.indexOf('const CPU_RATES='), source.indexOf('function showMenu()'));
  runInNewContext(constants + `
  const target={deckG:{getWorldQuaternion(q){return q.setFromAxisAngle(new THREE.Vector3(1,0,0),TH)},matrixWorld:new THREE.Matrix4().compose(new THREE.Vector3(0,FRONT_H,BOARD_Z),new THREE.Quaternion().setFromAxisAngle(new THREE.Vector3(1,0,0),TH),new THREE.Vector3(1,1,1))}};
  const HOLE_WORLD=new THREE.Vector3();
  ` + source.slice(source.indexOf('const boardInv ='), source.indexOf('// ---------- bag physics')) + body +
  '\nconst bags=[];const M={scores:[0,0],arc:1,spin:0,side:[\'slick\',\'slick\']};\n' + cpu +
  '\nglobalThis.api={newBody,launch,step,collide,simulate,wind,HOLE_LOCAL,HOLE_WORLD,baseYaw,RX,planCareerShot,bags,M,setRandom(f){Math.random=f;}};', ctx);
  return ctx.api;
}
const api = physics();
const FINAL = ['board', 'grass', 'in', 'removed'];
const r6 = (v) => Math.round(v * 1e9) / 1e9;
const vec = (v) => [r6(v.x), r6(v.y), r6(v.z)];

function setWind(mph, angle) {
  Object.assign(api.wind, {mph, angle, x: Math.sin(angle) * mph * .025, z: -Math.cos(angle) * mph * .025});
}

// WIN table exactly as index.html builds it at startup.
const WIN = {};
for (const t of [0, 1]) for (const side of ['slick', 'sticky']) for (const arc of [0, 1, 2]) {
  let lo = 1, hi = 0, olo = 1, ohi = 0;
  for (let p = 0; p <= 1; p += 0.0025) { const r = api.simulate(api.RX[t], p, side, 0, 0, arc); if (r === 3) { lo = Math.min(lo, p); hi = Math.max(hi, p); } if (r >= 1) { olo = Math.min(olo, p); ohi = Math.max(ohi, p); } }
  WIN[t + side + arc] = {lo, hi, olo, ohi, mid: hi >= lo ? (lo + hi) / 2 : (olo + ohi) / 2};
}

// Single-bag shots: full control of stance, power, aim, drift, spin, arc, side and wind.
function shot(c) {
  setWind(c.mph ?? 0, c.angle ?? 0);
  const b = api.newBody(c.team ?? 0, c.side);
  api.launch(b, c.rx, c.power, c.aim, c.drift, c.arc, c.spin);
  const transitions = [];
  for (let i = 0; i < 1920; i++) {
    const ev = api.step(b, 1 / 240);
    if (ev) transitions.push([i, ev, b.state]);
    if (FINAL.includes(b.state)) break;
  }
  setWind(0, 0);
  return {...c, state: b.state, p: vec(b.p), lp: vec(b.lp), dead: b.dead, offBoard: b.offBoard, touched: b.touched, swish: b.swish, t: r6(b.t), transitions};
}
const shots = [];
for (const side of ['slick', 'sticky']) for (const arc of [0, 1, 2]) for (const power of [0.2, 0.45, 0.5, 0.55, 0.62, 0.8])
  shots.push(shot({side, arc, power, rx: 0, aim: 0, drift: 0, spin: 0}));
for (const [rx, aim, drift, spin] of [[0.2, 0, 0, 0], [-0.3, 0.05, 0, 0], [0.45, -0.08, 0.3, 0], [0, 0.03, 0, -1], [0, -0.02, 0, 1], [0.1, 0, -0.5, 0.6]])
  for (const arc of [0, 1, 2]) shots.push(shot({side: 'slick', arc, power: 0.52, rx, aim, drift, spin}));
for (const [mph, angle] of [[8, Math.PI / 2], [12, -Math.PI / 3], [6, 0.2]])
  for (const arc of [0, 1, 2]) shots.push(shot({side: 'slick', arc, power: 0.5, rx: 0, aim: 0, drift: 0, spin: 0, mph, angle}));
shots.push(shot({side: 'slick', arc: 1, power: 0.99, rx: 0.45, aim: 0.12, drift: 0, spin: 0}));   // side fall-off / long miss
shots.push(shot({side: 'sticky', arc: 0, power: 0.05, rx: 0, aim: 0, drift: 0, spin: 0}));        // grounder
shots.push(shot({side: 'slick', arc: 0, power: 0.3, rx: 0, aim: 0, drift: 0, spin: 0}));           // short slider

// Multi-bag scenes in the original loop order: step eligible bags, then collide once.
function scene(c) {
  setWind(0, 0);
  const bags = c.resting.map(([team, x, z]) => { const b = api.newBody(team); b.lp.set(x, 0.016, z); b.p.copy(b.lp).applyMatrix4(new THREE.Matrix4().compose(new THREE.Vector3(0, 0.0889, -27 * 0.3048), new THREE.Quaternion().setFromAxisAngle(new THREE.Vector3(1, 0, 0), Math.asin((12 - 3.5) * 0.0254 / (4 * 0.3048))), new THREE.Vector3(1, 1, 1))); b.state = 'board'; b.touched = true; return b; });
  const a = api.newBody(c.team ?? 0, c.side); api.launch(a, c.rx, c.power, c.aim, 0, c.arc, c.spin ?? 0); bags.push(a);
  const settled = () => bags.every((b) => FINAL.includes(b.state) && !(b.state === 'grass' && b.dead));
  let steps = 0;
  for (; steps < 240 * 10; steps++) {
    for (const b of bags) if (b.state !== 'in' && b.state !== 'removed' && !(b.state === 'grass' && !b.dead) && b.state !== 'board') api.step(b, 1 / 240);
    api.collide(bags);
    if (settled()) break;
  }
  return {...c, steps, bags: bags.map((b) => ({team: b.team, state: b.state, lp: vec(b.lp), p: vec(b.p), dead: b.dead, offBoard: b.offBoard}))};
}
const hz = api.HOLE_LOCAL.z;
const scenes = [];
for (const power of [0.5, 0.53, 0.56, 0.6]) scenes.push(scene({resting: [[1, 0, hz + 0.3]], side: 'slick', arc: 0, power, rx: 0, aim: 0}));
for (const power of [0.52, 0.56]) scenes.push(scene({resting: [[1, 0.02, hz + 0.12], [0, -0.1, hz - 0.2]], side: 'slick', arc: 1, power, rx: 0, aim: 0}));
scenes.push(scene({resting: [[1, 0, hz + 0.45]], side: 'slick', arc: 2, power: 0.5, rx: 0, aim: 0}));
scenes.push(scene({resting: [[1, 0.0, hz - 0.08]], side: 'slick', arc: 0, power: 0.58, rx: 0, aim: 0}));

// roundWind sequences with a seeded generator (mulberry32 = index.html rng()).
function rng(seed) { return () => { seed |= 0; seed = seed + 0x6D2B79F5 | 0; let t = Math.imul(seed ^ seed >>> 15, 1 | seed); t = t + Math.imul(t ^ t >>> 7, 61 | t) ^ t; return ((t ^ t >>> 14) >>> 0) / 4294967296; }; }
const winds = [];
for (const circuit of [null, 0, 1, 2, 3]) {
  const random = rng(1234 + (circuit ?? 9)); let previousX = 0; const seq = [];
  for (let round = 0; round < 4; round++) { const w = roundWind({practice: false, circuit, previousX}, random); seq.push([w.mph, r6(w.angle)]); previousX = w.x; }
  winds.push({circuit, seed: 1234 + (circuit ?? 9), seq});
}
// planCareerShot decisions per rival on empty and crowded decks (fixed seeds, Math.random seeded
// inside the vm context only). Records the chosen arc/side/spin/power/aim/goal and RNG draws.
const roster = [['slider',0],['blocker',1],['lobber',2],['sniper',3],['wild',4],['spinner',5],['bulldozer',6],['champion',7]];
const plans = [];
const deckMatrix = new THREE.Matrix4().compose(new THREE.Vector3(0, 0.0889, -27 * 0.3048), new THREE.Quaternion().setFromAxisAngle(new THREE.Vector3(1, 0, 0), Math.asin((12 - 3.5) * 0.0254 / (4 * 0.3048))), new THREE.Vector3(1, 1, 1));
for (const [style, level] of roster) for (const fixture of [
  {deck: [], scores: [0, 0], mph: 0, angle: 0},
  {deck: [[0, 0.05, hzOf() + 0.2], [1, -0.03, hzOf() + 0.1]], scores: [18, 10], mph: 0, angle: 0},
  {deck: [[0, -0.12, hzOf() + 0.45]], scores: [3, 5], mph: 9, angle: 1.2},
]) {
  api.bags.length = 0;
  for (const [team, x, z] of fixture.deck) { const b = api.newBody(team); b.lp.set(x, 0.016, z); b.p.copy(b.lp).applyMatrix4(deckMatrix); b.state = 'board'; b.touched = true; api.bags.push(b); }
  api.M.scores = [...fixture.scores]; api.M.arc = 1; api.M.spin = 0; api.M.side = ['slick', 'slick'];
  setWind(fixture.mph, fixture.angle);
  const seed = 900 + level * 10 + plans.length % 3; const random = rng(seed); let draws = 0;
  api.setRandom(() => { draws++; return random(); });
  const shotPlan = api.planCareerShot({style, level, name: style, circuit: 0});
  setWind(0, 0);
  plans.push({style, level, seed, ...fixture, arc: api.M.arc, spin: api.M.spin, side: shotPlan.side, power: shotPlan.power, aim: shotPlan.aim, goal: shotPlan.goal, draws});
}
function hzOf() { return api.HOLE_LOCAL.z; }

const r = rng(42); const rngSeq = Array.from({length: 8}, () => r());

const sha = (p) => createHash('sha256').update(readFileSync(p)).digest('hex');
const out = {
  generator: 'wii/tools/oracle/golden.mjs',
  sources: Object.fromEntries(Object.entries(files).map(([k, p]) => [k, {path: relative(repo, p), sha256: sha(p)}])),
  timestep: 1 / 240,
  holeLocalZ: hz,
  baseYaw: Object.fromEntries([-0.45, 0, 0.3].map((x) => [x, api.baseYaw(x)])),
  win: WIN,
  shots, scenes, winds, plans, rng42: rngSeq,
};
const text = JSON.stringify(out, null, 1) + '\n';
const dest = join(repo, 'wii', 'tests', 'golden', 'arcade-1.0.3-physics.json');
if (process.argv.includes('--check')) {
  if (readFileSync(dest, 'utf8') !== text) { console.error('golden vectors differ from the pinned snapshot oracle'); process.exit(1); }
  console.log('golden vectors match the pinned snapshot oracle');
} else { writeFileSync(dest, text); console.log(`wrote ${dest}: ${shots.length} shots, ${scenes.length} scenes`); }
