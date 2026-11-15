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
import {checkRecorded,checkApprovals,currentValues,sha256,same,codeView} from '../lib/drift.mjs';
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

// Review finding (4e10231): extraction took the first textual match, so a commented-out old scoring
// line hid the active one. Extraction now reads executable code only and rejects ambiguous matches.
const SCORE_IN="if (b.state === 'in') { pts[b.team] += 3; inC[b.team]++; }";
test('NEGATIVE: old scoring kept as a comment while the active statement changes fails (review repro)',()=>{
 const s=editSource(realSources,'web','game',SCORE_IN,`// ${SCORE_IN}\n    if (b.state === 'in') { pts[b.team] += 2; inC[b.team]++; }`);
 assert.ok(s.web.game.includes(rule(real,'points-in-hole').snippet),'test setup: the old snippet is still in the file');
 const recorded=checkRecorded(data(),s);
 fails(recorded,/^rule:points-in-hole: common\.json value 3 but web source uses 2/,'web points-in-hole vs JSON');
 fails(recorded,/^rule:points-in-hole: no longer identical across web, arcade-1\.0\.3 \(web=2, arcade-1\.0\.3=3\)/,'web vs arcade');
 const approvals=approvalsOf(real,s);
 fails(approvals,/^web rule:points-in-hole: changed from baseline a07afaa value 3 to 2 with no approval/,'baseline drift');

 // Same for a block comment, a string literal and a template literal holding the old line,
 // and for the immutable arcade snapshot.
 for(const [label,platform,keep] of [
  ['block comment','web',`/* ${SCORE_IN} */`],
  ['string literal','web',`const OLD_SCORING = "${SCORE_IN}";`],
  ['template literal','web',`const OLD_SCORING = \`${SCORE_IN}\`;`],
  ['arcade comment','arcade-1.0.3',`// ${SCORE_IN}`],
 ]){
  const t=editSource(realSources,platform,'game',SCORE_IN,`${keep}\n    if (b.state === 'in') { pts[b.team] += 2; inC[b.team]++; }`);
  fails(checkRecorded(data(),t),new RegExp(`^rule:points-in-hole: common\\.json value 3 but ${platform.replace(/\./g,'\\.')} source uses 2`),label);
  fails(approvalsOf(real,t),platform==='web'?/^web rule:points-in-hole: changed from baseline a07afaa value 3 to 2/:/^arcade-1\.0\.3 rule:points-in-hole: changed from baseline 3 to 2, .*immutable/,label+' baseline');
 }
});

test('NEGATIVE: two active matches for one field are ambiguous and fail instead of taking the first',()=>{
 const s=editSource(realSources,'web','game',SCORE_IN,`${SCORE_IN}\n    if (b.state === 'in') { pts[b.team] += 2; inC[b.team]++; }`);
 fails(checkRecorded(data(),s),/^rule:points-in-hole: not found in web source \(ambiguous: 2 executable matches .*lines 636, 637/,'ambiguous scoring');
 fails(approvalsOf(real,s),/^web rule:points-in-hole: changed from baseline a07afaa value 3 to null/,'ambiguous baseline');
 const w=editSource(realSources,'web','controls','const [minimum,steps]=','const [lo,hi]=[[4,3],[6,4],[9,4],[12,9]][circuit]??[6,5];\n const [minimum,steps]=');
 fails(checkRecorded(data(),w),/^wind:tiersByCircuit: not found in web source \(ambiguous/,'ambiguous wind');
});

test('NEGATIVE: commented-out old wind, override and rule-flag statements cannot stand in for the active code',()=>{
 const w=editSource(realSources,'web','controls','const [minimum,steps]=[[4,3],[6,4],[9,4],[12,4]][circuit]??[6,5];','// const [minimum,steps]=[[4,3],[6,4],[9,4],[12,4]][circuit]??[6,5];\n const [minimum,steps]=[[4,3],[6,4],[9,4],[12,6]][circuit]??[6,4];');
 const ww=checkRecorded(data(),w);
 fails(ww,/^wind:tiersByCircuit: common\.json value .* but web source uses \[\[4,3\],\[6,4\],\[9,4\],\[12,6\]\]/,'wind tiers');
 fails(ww,/^wind:defaultTier: common\.json value \[6,5\] but web source uses \[6,4\]/,'wind default');
 fails(approvalsOf(real,w),/^web wind:tiersByCircuit: changed from baseline a07afaa/,'wind baseline');

 const f=editSource(realSources,'web','game','  camera.fov = 14 + 8 * portrait;','  // camera.fov = 14 + 8 * portrait;\n  camera.fov = 16 + 8 * portrait;');
 fails(checkRecorded(data(),f),/^web CAMERA_FOV: overrides\/web\.json value "14 \+ 8 \* portrait" but source uses "16\+8\*portrait"/,'override');
 fails(approvalsOf(real,f),/^web CAMERA_FOV: changed from baseline a07afaa/,'override baseline');

 const g=editSource(realSources,'web','game','    M.first = scorer;','    // M.first = scorer;');
 fails(checkRecorded(data(),g),/^rule:last-scorer-throws-first: common\.json value true but web source uses false/,'flag only in a comment');
});

test('comments and literals that repeat the active statement are ignored, not counted as matches',()=>{
 let s=editSource(realSources,'web','game',SCORE_IN,`// ${SCORE_IN}\n    /* ${SCORE_IN} */ const OLD = '${SCORE_IN.replace(/'/g,"\\'")}';\n    ${SCORE_IN}`);
 s=editSource(s,'web','controls','const [minimum,steps]=','// const [minimum,steps]=[[4,3],[6,4],[9,4],[12,4]][circuit]??[6,5];\n const [minimum,steps]=');
 assert.deepEqual(checkRecorded(data(),s),[]);
 assert.deepEqual(approvalsOf(real,s),[]);
});

// Review finding (7231d40): a `/` after `)` was always lexed as division, so a regex literal used as
// the statement of `if (...)` was read as code and its text could stand in for a removed statement.
test('NEGATIVE: an old statement kept only inside a regex literal after if/while/for or a block fails (review repro)',()=>{
 const ACTIVE='    M.first = scorer;';
 for(const [label,platform,keep] of [
  ['if (false) regex (review repro)','web',"if (false) /M.first = scorer;/.test('');"],
  ['while regex','web',"while (false) /M.first = scorer;/.test('');"],
  ['for regex','web',"for (;false;) /M.first = scorer;/.test('');"],
  ['nested-paren condition','web',"if ((0)) /M.first = scorer;/.test('');"],
  ['regex statement after a block','web',"if (false) {}\n    /M.first = scorer;/.test('');"],
  ['arcade if (false) regex','arcade-1.0.3',"if (false) /M.first = scorer;/.test('');"],
 ]){
  const s=editSource(realSources,platform,'game',ACTIVE,'    '+keep);
  assert.ok(s[platform].game.includes('M.first = scorer;'),`${label}: test setup keeps the old text`);
  fails(checkRecorded(data(),s),new RegExp(`^rule:last-scorer-throws-first: common\\.json value true but ${platform.replace(/\./g,'\\.')} source uses false`),label);
  fails(approvalsOf(real,s),platform==='web'
   ?/^web rule:last-scorer-throws-first: changed from baseline a07afaa value true to false with no approval/
   :/^arcade-1\.0\.3 rule:last-scorer-throws-first: changed from baseline true to false, .*immutable/,label+' baseline');
 }
});

test('regex-vs-division: control-flow heads and blocks end before a regex; other ) and ] still divide',()=>{
 const kinds=t=>{const {kind}=codeView(t);return [...t].map((_,i)=>kind[i]===1?'S':'c').join('');};
 const at=(t,needle)=>kinds(t).slice(t.indexOf(needle),t.indexOf(needle)+needle.length);
 for(const t of ["if (x) /a=b;/.test(s);","while (f(x)) /a=b;/.test(s);","for await (const v of g) /a=b;/.test(v);","if (x) {} /a=b;/.test(s);"])
  assert.match(at(t,'a=b;'),/^S+$/,`${t}: regex body must be a literal`);
 for(const t of ["y = (a + b) /a=b;/ c;","y = obj.if(x) /a=b;/ c;","y = m[i] /a=b;/ c;","y = f(x) /a=b;/ c;"])
  assert.match(at(t,'a=b;'),/^c+$/,`${t}: division operands must stay code`);
});

// Review finding (4de43ea): a `/` after a word was division unless the word was in a fixed set, so a
// regex statement after automatic semicolon insertion (`debugger\n/re/`, `break\n/re/`) was read as
// code. Each replacement is valid JavaScript that never assigns M.first.
test('NEGATIVE: an old statement kept only inside a regex literal after ASI (debugger/break/continue) fails (review repro)',()=>{
 const ACTIVE='    M.first = scorer;';
 for(const [label,platform,keep] of [
  ['debugger + newline (review repro)','web',"debugger\n    /M.first = scorer;/.test('');"],
  ['break + newline in do/while (review repro)','web',"do { break\n/M.first = scorer;/.test(''); } while (false);"],
  ['labelled break + newline','web',"outer: do { break outer\n/M.first = scorer;/.test(''); } while (false);"],
  ['continue + newline','web',"for (;false;) { continue\n/M.first = scorer;/.test(''); }"],
  ['labelled continue + newline','web',"outer: for (;false;) { continue outer\n/M.first = scorer;/.test(''); }"],
  ['arcade debugger + newline','arcade-1.0.3',"debugger\n    /M.first = scorer;/.test('');"],
 ]){
  assert.doesNotThrow(()=>new Function(keep),`${label}: test setup must be valid JavaScript`);
  const M={first:'unchanged'};new Function('M','scorer',keep)(M,'scorer');
  assert.equal(M.first,'unchanged',`${label}: test setup must not assign M.first`);
  const s=editSource(realSources,platform,'game',ACTIVE,'    '+keep);
  assert.ok(s[platform].game.includes('M.first = scorer;'),`${label}: test setup keeps the old text`);
  fails(checkRecorded(data(),s),new RegExp(`^rule:last-scorer-throws-first: common\\.json value true but ${platform.replace(/\./g,'\\.')} source uses false`),label);
  fails(approvalsOf(real,s),platform==='web'
   ?/^web rule:last-scorer-throws-first: changed from baseline a07afaa value true to false with no approval/
   :/^arcade-1\.0\.3 rule:last-scorer-throws-first: changed from baseline true to false, .*immutable/,label+' baseline');
 }
});

test('regex-vs-division after a word: complete statements (ASI) start a regex; identifiers, properties and numbers divide',()=>{
 const kinds=t=>{const {kind}=codeView(t);return [...t].map((_,i)=>kind[i]===1?'S':'c').join('');};
 const at=(t,needle)=>kinds(t).slice(t.indexOf(needle),t.indexOf(needle)+needle.length);
 for(const t of ["debugger\n/a=b;/.test(s);","do { break\n/a=b;/.test(s); } while (0);","l: do { break l\n/a=b;/.test(s); } while (0);",
  "for (;;) { continue\n/a=b;/.test(s); }","l: for (;;) { continue l\n/a=b;/.test(s); }","return /a=b;/.test(s);"])
  assert.match(at(t,'a=b;'),/^S+$/,`${JSON.stringify(t)}: regex body must be a literal`);
 for(const t of ["y = x\n/a=b;/ c;","y = o.debugger /a=b;/ c;","y = o.break /a=b;/ c;","y = o?.return /a=b;/ c;","y = 1.5 /a=b;/ c;",
  "break\nx\n/a=b;/ c;","y = o.break l /a=b;/ c;"])
  assert.match(at(t,'a=b;'),/^c+$/,`${JSON.stringify(t)}: division operands must stay code`);
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
