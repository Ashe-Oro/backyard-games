// Drift check: shared cornhole data must match the real game sources it describes, and any value
// that differs from the a07afaa baseline needs an exact structured approval.
// Run from anywhere: node --test shared/cornhole/tests/*.test.mjs
//
// This file is the gate: it runs against the real files. guards.test.mjs proves the checks
// themselves reject wrong values and approvals (in-memory mutations of this state).
import {test} from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import {fileURLToPath} from 'node:url';
import {checkRecorded,checkApprovals,currentValues,sha256} from '../lib/drift.mjs';
import {SPEC,loadShared,loadSources,baselineSources,buildBaseline,commitAvailable,compareBaseline} from '../lib/baseline.mjs';

const shared=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'..');
const repo=path.resolve(shared,'..','..');
const real=loadShared(shared);
const realSources=loadSources(repo,real.overrides);
const data=(d=real)=>({common:d.common,overrides:d.overrides});
const approvalsOf=(d,sources,approvals=d.approvals.approvals,baseline=d.baseline)=>checkApprovals({baseline,current:currentValues(data(d),sources),overrides:d.overrides,approvals});

// ------------------------------------------------------------------ real files (must pass)
test('recorded common, scoring, wind and override values equal the values extracted from the game sources',()=>{
 assert.deepEqual(checkRecorded(data(),realSources),[]);
});

test('every recorded snippet is still present verbatim (documentation pointers)',()=>{
 const src=(platform,file)=>realSources[platform]?.[file]??assert.fail(`overrides/${platform}.json has no "${file}" file`);
 const common=[...real.common.constants.map(c=>[c.name,c]),...real.common.rules.map(r=>[r.id,r]),...real.common.wind.snippets.map((s,i)=>['wind#'+i,s])];
 for(const platform of real.common.verifiedAgainst)for(const [name,{snippet,file}] of common)assert.ok(src(platform,file).includes(snippet),`${platform} ${name}: "${snippet}" not found`);
 for(const [platform,d] of Object.entries(real.overrides))for(const o of d.overrides)assert.ok(src(platform,o.file).includes(o.snippet),`${platform} ${o.name}: "${o.snippet}" not found`);
});

test('overrides never redefine a common field and the Wii baseline is arcade 1.0.3',()=>{
 const names=new Set(real.common.constants.map(c=>c.name));
 for(const [platform,d] of Object.entries(real.overrides))for(const o of d.overrides)assert.ok(!names.has(o.name),`${platform} overrides common field ${o.name}`);
 assert.equal(real.overrides.wii.baseline,'arcade-1.0.3');
 assert.ok(real.baseline.values['arcade-1.0.3'],'baseline.json has no arcade-1.0.3 values for the Wii to inherit');
});

test('baseline.json is pinned to a07afaa and the arcade 1.0.3 snapshot files are byte-identical to it',()=>{
 assert.equal(real.baseline.ref,SPEC.ref);
 assert.equal(real.baseline.commit,SPEC.commit);
 assert.equal(real.baseline.tag,SPEC.tag);
 for(const [key,{path:rel,sha256:hash}] of Object.entries(real.baseline.platforms['arcade-1.0.3'].files)){
  assert.equal(real.overrides['arcade-1.0.3'].files[key],rel,`arcade-1.0.3 ${key} must still be read from the pinned snapshot path`);
  assert.equal(sha256(realSources['arcade-1.0.3'][key]),hash,`${rel} differs from ${SPEC.ref}; the 1.0.3 snapshot is immutable`);
 }
});

test('baseline.json equals the values re-extracted from git at a07afaa',t=>{
 if(!commitAvailable(repo,SPEC.commit)){
  if(process.env.DRIFT_ALLOW_NO_BASELINE_COMMIT==='1')return t.skip(`${SPEC.commit} not in this clone (DRIFT_ALLOW_NO_BASELINE_COMMIT=1)`);
  assert.fail(`Baseline commit ${SPEC.commit} (${SPEC.tag}) is not in this clone. Fetch full history (actions/checkout fetch-depth: 0), or set DRIFT_ALLOW_NO_BASELINE_COMMIT=1 to skip only this re-derivation.`);
 }
 assert.deepEqual(compareBaseline(real.baseline,buildBaseline(data(),baselineSources(repo))),[]);
});

test('every value that differs from the baseline has an exact approval (and no approval is stale)',()=>{
 assert.deepEqual(approvalsOf(real,realSources),[]);
});

test('approvals.json is structured',()=>{
 assert.equal(real.approvals.schema,1);
 assert.ok(Array.isArray(real.approvals.approvals));
});

