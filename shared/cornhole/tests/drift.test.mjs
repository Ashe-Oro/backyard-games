// Drift check: shared cornhole data must match the real game sources it describes.
// Run from anywhere: node --test shared/cornhole/tests/*.test.mjs
import {test} from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import vm from 'node:vm';
import {fileURLToPath} from 'node:url';

const shared=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'..');
const repo=path.resolve(shared,'..','..');
const json=file=>JSON.parse(fs.readFileSync(path.join(shared,file),'utf8'));
const common=json('common.json');
const overrides=Object.fromEntries(fs.readdirSync(path.join(shared,'overrides')).filter(f=>f.endsWith('.json')).map(f=>{const o=json(path.join('overrides',f));return [o.platform,o];}));
const cache=new Map();
function source(platform,key){
 const rel=overrides[platform]?.files?.[key];
 assert.ok(rel,`overrides/${platform}.json has no "${key}" file`);
 if(!cache.has(rel))cache.set(rel,fs.readFileSync(path.join(repo,rel),'utf8'));
 return [rel,cache.get(rel)];
}
const commonSnippets=[...common.constants.map(c=>[c.name,c]),...common.rules.map(r=>[r.id,r]),...common.wind.snippets.map((s,i)=>['wind#'+i,s])];

for(const platform of common.verifiedAgainst){
 test(`common.json values are present verbatim in ${platform}`,()=>{
  for(const [name,{snippet,file}] of commonSnippets){
   const [rel,text]=source(platform,file);
   assert.ok(text.includes(snippet),`${name}: "${snippet}" not found in ${rel}. Update shared data (with an approval) or revert the source change.`);
  }
 });
}

test('common.json numeric values match their expressions',()=>{
 const context=vm.createContext({Math});
 for(const c of common.constants){
  const value=vm.runInContext(`var ${c.name} = (${c.expr}); ${c.name}`,context);
  assert.ok(Math.abs(value-c.value)<=1e-12*Math.max(1,Math.abs(value)),`${c.name}: expr ${c.expr} = ${value}, JSON says ${c.value}`);
 }
});

for(const [platform,data] of Object.entries(overrides)){
 test(`overrides/${platform}.json snippets are present in its sources`,()=>{
  for(const o of data.overrides){
   const [rel,text]=source(platform,o.file);
   assert.ok(text.includes(o.snippet),`${platform} ${o.name}: "${o.snippet}" not found in ${rel}`);
  }
 });
}

test('overrides never redefine a common field and the Wii baseline exists',()=>{
 const names=new Set(common.constants.map(c=>c.name));
 for(const [platform,data] of Object.entries(overrides))for(const o of data.overrides)assert.ok(!names.has(o.name),`${platform} overrides common field ${o.name}`);
 assert.equal(overrides.wii.baseline,'arcade-1.0.3');
 assert.ok(overrides[overrides.wii.baseline],'Wii baseline platform has no overrides file');
});

test('approvals are structured entries',()=>{
 const {approvals}=json('approvals.json');
 assert.ok(Array.isArray(approvals));
 for(const a of approvals)for(const key of ['field','platform','old','new','baseline','pr'])assert.ok(key in a,`approval missing ${key}: ${JSON.stringify(a)}`);
});
