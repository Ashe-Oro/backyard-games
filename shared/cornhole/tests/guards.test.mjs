// Guard tests for the drift checker (lib/drift.mjs, lib/baseline.mjs).
// Run from anywhere: node --test shared/cornhole/tests/*.test.mjs
//
// Each NEGATIVE test mutates an in-memory copy of the committed JSON or sources (never the files)
// and asserts that the checks FAIL; the acceptance tests show an exact approval passes. The
// fixture is the committed state: if a value used here legitimately changes (with an approval),
// update the edit strings below to the new source text.
import {test} from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import {fileURLToPath} from 'node:url';
import {checkRecorded,checkApprovals,currentValues,sha256,same} from '../lib/drift.mjs';
import {SPEC,loadShared,loadSources,baselineSources,buildBaseline,commitAvailable,compareBaseline} from '../lib/baseline.mjs';

const shared=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'..');
const repo=path.resolve(shared,'..','..');
const real=loadShared(shared);
const realSources=loadSources(repo,real.overrides);
const clone=v=>structuredClone(v);
const data=(d=real)=>({common:d.common,overrides:d.overrides});
const approvalsOf=(d,sources,approvals=d.approvals.approvals,baseline=d.baseline)=>checkApprovals({baseline,current:currentValues(data(d),sources),overrides:d.overrides,approvals});
const constant=(d,name)=>d.common.constants.find(c=>c.name===name);
const rule=(d,id)=>d.common.rules.find(r=>r.id===id);
const override=(d,platform,name)=>d.overrides[platform].overrides.find(o=>o.name===name);
function editSource(sources,platform,key,from,to){
 const out=clone(sources);
 assert.ok(out[platform][key].includes(from),`test setup: ${JSON.stringify(from)} not in ${platform} ${key}`);
 out[platform][key]=out[platform][key].replace(from,to);
 return out;
}
const fails=(errors,pattern,label)=>{
 assert.ok(errors.length>0,`${label}: expected the drift check to FAIL, but it passed`);
 assert.ok(errors.some(e=>pattern.test(e)),`${label}: expected an error matching ${pattern}, got:\n${errors.join('\n')}`);
};

// ------------------------------------------------------------------ finding 1: wrong values must FAIL
test('NEGATIVE: a wrong common value/expression fails even when its snippet is unchanged',()=>{
 const d=clone(real);
 Object.assign(constant(d,'G'),{expr:'9.7',value:9.7});            // snippet still says 9.8
 fails(checkRecorded(data(d),realSources),/^G: common\.json value 9\.7 but web source uses 9\.8/,'G 9.7');

 const e=clone(real);constant(e,'W').expr='4 * FT / 2';             // same value, different expression
 fails(checkRecorded(data(e),realSources),/^W: common\.json expr "4 \* FT \/ 2" but web source declares "2 \* FT"/,'W expr');

 const f=clone(real);constant(f,'W').expr='2 * IN';                  // value left right, expression wrong
 fails(checkRecorded(data(f),realSources),/^W: common\.json expr "2 \* IN" = 0\.0508/,'W wrong expr');

 for(const [name,value] of [['STICKY_MU',0.61],['ARC_KOBE_DEG',57],['LAND_KEEP_PUSH',1.3],['SLIDE_IN_MARGIN',0.02],['PHYSICS_DT',1/120],['WIND_MPH_TO_ACCEL',0.03]]){
  const g=clone(real);constant(g,name).value=value;
  fails(checkRecorded(data(g),realSources),new RegExp(`^${name}: common\\.json value`),name);
 }
});

test('NEGATIVE: a wrong override value fails for each platform even when its snippet is unchanged',()=>{
 for(const [platform,name,value] of [
  ['arcade-1.0.3','REL_Y',1.23],['arcade-1.0.3','SLICK_MU',0.44],['arcade-1.0.3','AIR_SPIN_RATE',12],['arcade-1.0.3','FLIGHT_CURL',0.1],
  ['arcade-1.0.3','CURL',{slick:{acceleration:3.2,decay:1.5},sticky:{acceleration:1.5,decay:4.5}}],['arcade-1.0.3','HUMAN_SPIN',false],
  ['arcade-1.0.3','POWER_INPUT','pull-distance'],['arcade-1.0.3','SPEED_WINDOW_MS',100],['arcade-1.0.3','POWER_SPEED_FULL',6000],['arcade-1.0.3','CAMERA_FOV','18 + 8 * portrait'],
  ['web','SLICK_KEEP',0.4],['web','REL_Y',1.2],['web','AIR_SPIN_RATE',14],['web','FLIGHT_CURL',0],['web','HUMAN_SPIN',true],['web','POWER_INPUT','swing-speed'],['web','CAMERA_FOV','18 + 6 * portrait'],
 ]){
  const d=clone(real);override(d,platform,name).value=value;
  fails(checkRecorded(data(d),realSources),new RegExp(`^${platform} ${name}: overrides/${platform}\\.json value`),`${platform} ${name}`);
 }
});

test('NEGATIVE: wrong scoring rules and wind data fail',()=>{
 for(const [id,value] of [['points-in-hole',2],['points-on-board',2],['target-score',20],['exact-21-bust-to',12],['bags-per-inning',6],['last-scorer-throws-first',false],['alternating-throws',false],['grounder-is-dead',false],['cancellation-scoring','total points, no cancellation']]){
  const d=clone(real);rule(d,id).value=value;
  fails(checkRecorded(data(d),realSources),new RegExp(`^rule:${id}: common\\.json value`),id);
 }
 for(const [key,value] of [['tiersByCircuit',[[4,3],[6,4],[9,4],[12,5]]],['defaultTier',[6,4]],['practiceIsCalm',false],['laterRoundsSwitchSides',false]]){
  const d=clone(real);d.common.wind[key]=value;
  fails(checkRecorded(data(d),realSources),new RegExp(`^wind:${key}: common\\.json value`),key);
 }
});

test('NEGATIVE: a source change fails even if the old snippet text is still present in the file',()=>{
 // Keep the snippet as a comment, change the real declaration.
 const s=editSource(realSources,'web','game','const V_MIN = 5.6;','// const V_MIN = 5.6;\nconst V_MIN = 5.7;');
 assert.ok(s.web.game.includes(constant(real,'V_MIN').snippet));
 fails(checkRecorded(data(),s),/^V_MIN: common\.json value 5\.6 but web source uses 5\.7/,'web V_MIN');
 fails(checkRecorded(data(),s),/^V_MIN: no longer identical across web, arcade-1\.0\.3/,'web V_MIN identical');

 const w=editSource(realSources,'web','controls','[[4,3],[6,4],[9,4],[12,4]][circuit]','[[4,3],[6,4],[9,4],[12,6]][circuit]');
 fails(checkRecorded(data(),w),/^wind:tiersByCircuit: common\.json value/,'wind tiers in source');

 const p=editSource(realSources,'arcade-1.0.3','game',"pts[b.team] += 3;","pts[b.team] += 2;");
 fails(checkRecorded(data(),p),/^rule:points-in-hole: common\.json value 3 but arcade-1\.0\.3 source uses 2/,'arcade points');

 const c=editSource(realSources,'arcade-1.0.3','spin','decay: 1.4','decay: 1.6');
 fails(checkRecorded(data(),c),/^arcade-1\.0\.3 CURL: overrides/,'arcade CURL');
});

test('NEGATIVE: intentional web/arcade differences must stay different, and unknown fields need an extractor',()=>{
 const d=clone(real);override(d,'web','SLICK_MU').value=0.43;
 const s=editSource(realSources,'web','game','slick: { mu: 0.30,','slick: { mu: 0.43,');
 fails(checkRecorded(data(d),s),/^SLICK_MU: listed as a arcade-1\.0\.3\/web difference but both use 0\.43/,'SLICK_MU harmonised');

 const u=clone(real);u.common.constants.push({name:'NEW_THING',expr:'1',value:1,snippet:'const G = 9.8;',file:'game'});
 fails(checkRecorded(data(u),realSources),/^NEW_THING: no test-owned extractor/,'unknown field');
});

// ------------------------------------------------------------------ finding 2: approvals vs baseline
// A consistent web gameplay change: source and web.json both say 0.35, baseline (a07afaa) is 0.30.
const slick=()=>{
 const d=clone(real);override(d,'web','SLICK_MU').value=0.35;override(d,'web','SLICK_MU').snippet='slick: { mu: 0.35, ms: 0.5, keep: 0.42 }';
 return {d,s:editSource(realSources,'web','game','slick: { mu: 0.30,','slick: { mu: 0.35,')};
};
const approval=(over={})=>({field:'SLICK_MU',platform:'web',old:0.3,new:0.35,baseline:'a07afaa',pr:'#123',...over});

test('NEGATIVE: the review repro (G changed in both games and all JSON fields, no approval) is rejected',()=>{
 const d=clone(real);Object.assign(constant(d,'G'),{expr:'9.7',value:9.7,snippet:'const G = 9.7;'});
 let s=editSource(realSources,'web','game','const G = 9.8;','const G = 9.7;');
 s=editSource(s,'arcade-1.0.3','game','const G = 9.8;','const G = 9.7;');
 assert.deepEqual(checkRecorded(data(d),s),[],'test setup: the edit is internally consistent');
 const errors=approvalsOf(d,s);
 fails(errors,/^web G: changed from baseline a07afaa value 9\.8 to 9\.7 with no approval/,'web G');
 fails(errors,/^arcade-1\.0\.3 G: .*immutable release snapshot/,'arcade G');
 // An approval cannot cover the immutable arcade snapshot.
 fails(approvalsOf(d,s,[{field:'G',platform:'web',old:9.8,new:9.7,baseline:'a07afaa',pr:'#1'},{field:'G',platform:'arcade-1.0.3',old:9.8,new:9.7,baseline:'a07afaa',pr:'#1'}]),/^arcade-1\.0\.3 G: .*immutable/,'arcade approval');
});

test('a web drift with an exact {field, platform, old, new, baseline, pr} approval is accepted',()=>{
 const {d,s}=slick();
 assert.deepEqual(checkRecorded(data(d),s),[]);
 assert.deepEqual(approvalsOf(d,s,[approval()]),[]);
 assert.deepEqual(approvalsOf(d,s,[approval({pr:'https://github.com/example/repo/pull/123',note:'tuned slick side'})]),[]);
});

test('NEGATIVE: missing, mismatched, wrong-baseline, incomplete and duplicate approvals are rejected',()=>{
 const {d,s}=slick();
 fails(approvalsOf(d,s,[]),/^web SLICK_MU: changed from baseline a07afaa value 0\.3 to 0\.35 with no approval/,'missing');
 for(const [label,bad,pattern] of [
  ['wrong new',approval({new:0.36}),/^web SLICK_MU: approval does not match the drift/],
  ['wrong old',approval({old:0.31}),/^web SLICK_MU: approval does not match the drift/],
  ['wrong platform',approval({platform:'arcade-1.0.3'}),/^web SLICK_MU: changed from baseline .* with no approval/],
  ['wrong field',approval({field:'SLICK_MS'}),/^web SLICK_MU: changed from baseline .* with no approval/],
  ['wrong baseline',approval({baseline:'c621a22'}),/baseline "c621a22" is not the current baseline "a07afaa"/],
  ['no pr',(({pr,...a})=>a)(approval()),/missing "pr"/],
  ['empty pr',approval({pr:' '}),/"pr" must name the PR/],
  ['unknown key',approval({approvedBy:'me'}),/unknown key "approvedBy"/],
 ])fails(approvalsOf(d,s,[bad]),pattern,label);
 fails(approvalsOf(d,s,[approval(),approval()]),/duplicate approval for web SLICK_MU/,'duplicate');
});

test('NEGATIVE: a stale approval (value is back at the baseline) is rejected',()=>{
 fails(approvalsOf(real,realSources,[approval()]),/stale, web SLICK_MU does not differ from baseline a07afaa/,'stale');
 const {d,s}=slick();                                                 // drift moved on: approval names an older value
 const s2=editSource(s,'web','game','slick: { mu: 0.35,','slick: { mu: 0.37,');override(d,'web','SLICK_MU').value=0.37;
 fails(approvalsOf(d,s2,[approval()]),/approval does not match the drift. Needed .*"new":0\.37/,'superseded');
});

test('NEGATIVE: untracking a field or tracking one missing from baseline.json is rejected',()=>{
 const d=clone(real);d.common.constants=d.common.constants.filter(c=>c.name!=='V_MIN');
 fails(approvalsOf(d,realSources),/^web V_MIN: changed from baseline a07afaa value 5\.6 to null with no approval/,'untracked');
 const b=clone(real);delete b.baseline.values.web.REL_Y;
 fails(approvalsOf(b,realSources),/baseline\.json has no web REL_Y: regenerate it from a07afaa/,'missing baseline field');
});

test('Wii overrides are drifts from the arcade 1.0.3 baseline: rejected without, accepted with an exact approval',()=>{
 const d=clone(real);d.overrides.wii.overrides.push({name:'REL_Y',value:1.1,unit:'m'});
 fails(approvalsOf(d,realSources),/^wii REL_Y: changed from baseline a07afaa value 0\.98 to 1\.1 with no approval/,'wii missing');
 fails(approvalsOf(d,realSources,[{field:'REL_Y',platform:'wii',old:1.3,new:1.1,baseline:'a07afaa',pr:'#9'}]),/^wii REL_Y: approval does not match/,'wii old = web value');
 assert.deepEqual(approvalsOf(d,realSources,[{field:'REL_Y',platform:'wii',old:0.98,new:1.1,baseline:'a07afaa',pr:'#9'}]),[]);
 const e=clone(real);e.overrides.wii.overrides.push({name:'REL_Y',value:0.98});
 fails(approvalsOf(e,realSources),/^wii REL_Y: override equals the arcade-1\.0\.3 baseline/,'wii no-op override');
});

test('NEGATIVE: a hand-edited baseline.json is caught by the git re-derivation',t=>{
 if(!commitAvailable(repo,SPEC.commit))return t.skip('baseline commit not in this clone');
 const derived=buildBaseline(data(),baselineSources(repo));
 const edited=clone(real.baseline);edited.values.web.SLICK_MU=0.35;     // would "launder" the slick change
 const {d,s}=slick();
 assert.deepEqual(approvalsOf(d,s,[],edited),[],'test setup: the laundered baseline alone would hide the drift');
 fails(compareBaseline(edited,derived),/^baseline\.json web SLICK_MU is 0\.35 but a07afaa sources give 0\.3/,'laundered baseline');
 const moved=clone(real.baseline);moved.ref='c621a22';
 fails(compareBaseline(moved,derived),/^baseline\.json ref "c621a22"/,'moved ref');
 const hashed=clone(real.baseline);hashed.platforms.web.files.game.sha256='0'.repeat(64);
 fails(compareBaseline(hashed,derived),/file paths\/hashes differ/,'edited hash');
});
