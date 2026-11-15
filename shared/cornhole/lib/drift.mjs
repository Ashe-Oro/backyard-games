// Drift-check library for shared/cornhole. Dependency-free (node: built-ins only).
//
// The JSON files record values; this module reads the *game sources* and extracts the
// values they actually use, with test-owned locators. Nothing here trusts a JSON `snippet`,
// `expr` or `value` to describe the source: those are what get checked.
//
//   extractField(key, src)        value the source actually uses for a field (undefined = not found)
//   checkRecorded(data, sources)  JSON values/expressions vs extracted source values  (review finding 1)
//   currentValues(data, sources)  {platform: {field: value}} extracted from sources
//   checkApprovals({...})         drift vs baseline must have exact approvals            (review finding 2)
import vm from 'node:vm';
import {createHash} from 'node:crypto';

export const sha256=text=>createHash('sha256').update(text).digest('hex');
export const APPROVAL_KEYS=['field','platform','old','new','baseline','pr'];
export const IMMUTABLE_PLATFORMS=['arcade-1.0.3'];

// ---------------------------------------------------------------- value comparison
export function same(a,b){
 if(typeof a==='number'&&typeof b==='number')return a===b||Math.abs(a-b)<=1e-12*Math.max(1,Math.abs(a),Math.abs(b));
 if(typeof a==='string'&&typeof b==='string')return norm(a)===norm(b);
 if(Array.isArray(a)&&Array.isArray(b))return a.length===b.length&&a.every((v,i)=>same(v,b[i]));
 if(a&&b&&typeof a==='object'&&typeof b==='object'&&!Array.isArray(a)&&!Array.isArray(b)){
  const ka=Object.keys(a).sort(),kb=Object.keys(b).sort();
  return same(ka,kb)&&ka.every(k=>same(a[k],b[k]));
 }
 return a===b;
}
const norm=s=>s.replace(/\s+/g,' ').replace(/\s*([()*/+\-,?:])\s*/g,'$1').trim();
const show=v=>JSON.stringify(v);

// ---------------------------------------------------------------- source declarations
// Top-level `const` / `export const` statements that end on the same line (the style of every
// file read here). Declarator lists (`const IN = 0.0254, FT = 0.3048;`) are split on top-level commas.
function splitTop(text){
 const parts=[];let depth=0,quote=null,start=0;
 for(let i=0;i<text.length;i++){
  const c=text[i];
  if(quote){if(c==='\\')i++;else if(c===quote)quote=null;continue;}
  if(c==='"'||c==="'"||c==='`')quote=c;
  else if('([{'.includes(c))depth++;
  else if(')]}'.includes(c))depth--;
  else if(c===','&&depth===0){parts.push(text.slice(start,i));start=i+1;}
 }
 parts.push(text.slice(start));
 return parts;
}
const declCache=new Map();
function declarations(text){
 if(typeof text!=='string')return new Map();
 if(declCache.has(text))return declCache.get(text);
 const map=new Map(),dupes=new Set();
 for(const m of text.matchAll(/^(?:export\s+)?const\s+([^;\n]*);/gm)){
  for(const part of splitTop(m[1])){
   const d=/^\s*([A-Za-z_$][\w$]*)\s*=\s*([\s\S]*?)\s*$/.exec(part);
   if(!d)continue;
   if(map.has(d[1]))dupes.add(d[1]);
   map.set(d[1],d[2]);
  }
 }
 for(const name of dupes)map.set(name,undefined); // ambiguous: refuse to guess
 if(declCache.size>64)declCache.clear();
 declCache.set(text,map);
 return map;
}
// Evaluate an expression from the source, resolving identifiers from the same file's top-level
// declarations (recursively). Only Math and Object.freeze are available otherwise.
function evaluate(expr,text,extra={},seen=new Set()){
 const decls=declarations(text),context={Math,Object:{freeze:Object.freeze},...extra};
 for(const id of new Set(expr.replace(/(['"`])(?:\\.|(?!\1).)*\1/g,'').match(/(?<![.\w$])[A-Za-z_$][\w$]*/g)??[])){
  if(id in context||!decls.has(id))continue;
  if(seen.has(id))throw new Error(`circular declaration ${id}`);
  const inner=decls.get(id);
  if(inner===undefined)throw new Error(`ambiguous declaration ${id}`);
  context[id]=evaluate(inner,text,extra,new Set([...seen,id]));
 }
 return vm.runInNewContext(`(${expr})`,context,{timeout:1000});
}
// {expr, value} for a top-level const in a source file, or undefined.
export function declared(text,name){
 const expr=declarations(text).get(name);
 if(expr===undefined)return undefined;
 return {expr,value:evaluate(expr,text)};
}
const literal=text=>{
 const t=text.trim();
 return /^-?(?:\d+\.?\d*|\.\d+)$/.test(t)?Number(t):t;
};
// Return the capture groups of a test-owned pattern, or undefined when it doesn't match.
const grab=(text,re)=>{const m=typeof text==='string'?re.exec(text):null;return m?m.slice(1):undefined;};
const sole=values=>new Set(values.map(show)).size===1?values[0]:undefined;

// ---------------------------------------------------------------- field extractors
// Each extractor gets {game, controls, spin?, trackball?} (source text) and returns the value the
// game uses, plus (for constants) the source expression. Every recorded field must have one.
const decl=(name,file='game')=>src=>declared(src[file],name);
const member=(name,pathKeys)=>src=>{
 const d=declared(src.game,name);if(!d)return undefined;
 let v=d.value;for(const k of pathKeys)v=v?.[k];
 return v===undefined?undefined:{value:v};
};
const index=(name,i)=>src=>{const d=declared(src.game,name);return d&&d.value[i]!==undefined?{value:d.value[i]}:undefined;};
const num=(file,re,pick=g=>g[0])=>src=>{const g=grab(src[file],re);if(!g)return undefined;const v=pick(g);return v===undefined?undefined:{value:Number(v)};};

const CONSTANTS={
 ...Object.fromEntries(['G','IN','FT','W','L','FRONT_H','BACK_H','TH','BOARD_Z','HOLE_FROM_TOP','HOLE_R','BAG','BAG_H','BAG_R','MU_GROUND','BAG_E','ELEV','V_MIN','AIM_MAX','REL_Z'].map(n=>[n,decl(n)])),
 STICKY_MU:member('SIDES',['sticky','mu']),
 STICKY_MS:member('SIDES',['sticky','ms']),
 STICKY_KEEP:member('SIDES',['sticky','keep']),
 ARC_PUSH_DEG:index('arcs',0),
 ARC_NORMAL_DEG:index('arcs',1),
 ARC_KOBE_DEG:index('arcs',2),
 LAND_KEEP_KOBE:num('game',/S\.keep\s*\*\s*\(\s*b\.arc\s*===\s*2\s*\?\s*([\d.]+)\s*:\s*b\.arc\s*===\s*0\s*\?\s*([\d.]+)\s*:\s*1\s*\)/,g=>g[0]),
 LAND_KEEP_PUSH:num('game',/S\.keep\s*\*\s*\(\s*b\.arc\s*===\s*2\s*\?\s*([\d.]+)\s*:\s*b\.arc\s*===\s*0\s*\?\s*([\d.]+)\s*:\s*1\s*\)/,g=>g[1]),
 SWISH_R_FRAC:num('game',/holeDist\(lp\)\s*<\s*HOLE_R\s*\*\s*([\d.]+)/),
 SLIDE_IN_MARGIN:num('game',/holeDist\(lp\)\s*<\s*HOLE_R\s*-\s*([\d.]+)/),
 // The fixed-step loop (`while (acc >= X) { acc -= X; ... step(b, X)`) and every literal
 // step(b, N / M) call (AI previews) must agree on one step.
 PHYSICS_DT:src=>{
  if(typeof src.game!=='string')return undefined;
  const loop=grab(src.game,/while\s*\(\s*acc\s*>=\s*([^)]+?)\s*\)\s*\{\s*acc\s*-=\s*([^;]+?)\s*;[^}]*?\bstep\(b,\s*([^)]+?)\s*\)/);
  if(!loop)return undefined;
  const literals=[...src.game.matchAll(/\bstep\(b,\s*([\d.]+\s*\/\s*[\d.]+)\s*\)/g)].map(m=>m[1]);
  const v=sole([...loop,...literals].map(e=>evaluate(e,src.game)));
  return v===undefined?undefined:{value:v};
 },
 WIND_MPH_TO_ACCEL:num('controls',/x\s*:\s*Math\.sin\(angle\)\s*\*\s*mph\s*\*\s*([\d.]+)\s*,\s*z\s*:\s*-\s*Math\.cos\(angle\)\s*\*\s*mph\s*\*\s*([\d.]+)/,g=>g[0]===g[1]||Number(g[0])===Number(g[1])?g[0]:undefined),
};
const flag=(file,...patterns)=>src=>typeof src[file]==='string'?{value:patterns.every(re=>re.test(src[file]))}:undefined;
const RULES={
 'points-in-hole':num('game',/if\s*\(\s*b\.state\s*===\s*'in'\s*\)\s*\{\s*pts\[b\.team\]\s*\+=\s*(\d+)/),
 'points-on-board':num('game',/b\.state\s*===\s*'board'\s*&&\s*!b\.dead\s*\)\s*\{\s*pts\[b\.team\]\s*\+=\s*(\d+)/),
 'cancellation-scoring':src=>{
  const ok=/const\s+net\s*=\s*Math\.abs\(\s*pts\[0\]\s*-\s*pts\[1\]\s*\)/.test(src.game)&&/scorer\s*=\s*pts\[0\]\s*>\s*pts\[1\]\s*\?\s*0\s*:\s*pts\[1\]\s*>\s*pts\[0\]\s*\?\s*1\s*:\s*-1/.test(src.game);
  return {value:ok?'net = |red - blue|, awarded to the higher side; a tie is a wash':null};
 },
 'target-score':num('game',/M\.exact\s*\?\s*M\.scores\[scorer\]\s*===\s*(\d+)\s*:\s*M\.scores\[scorer\]\s*>=\s*(\d+)/,g=>g[0]===g[1]?g[0]:undefined),
 'exact-21-bust-to':num('game',/if\s*\(\s*M\.exact\s*&&\s*M\.scores\[scorer\]\s*>\s*\d+\s*\)\s*\{\s*M\.scores\[scorer\]\s*=\s*(\d+)/),
 'last-scorer-throws-first':flag('game',/\bM\.first\s*=\s*scorer\s*;/),
 'bags-per-inning':num('game',/M\.k\s*>=\s*(\d+)\s*\)\s*\{\s*M\.phase\s*=\s*'settle'/),
 'alternating-throws':flag('game',/k\s*%\s*2\s*===\s*0\s*\?\s*M\.first\s*:\s*1\s*-\s*M\.first/),
 'grounder-is-dead':flag('game',/else\s+if\s*\(\s*b\.dead\s*\)\s*say\('GROUNDER'/,/b\.state\s*===\s*'board'\s*&&\s*!b\.dead\s*\)\s*\{\s*pts/),
};
const TIERS=/=\s*(\[\s*\[[\d,\s[\]]+\]\s*\])\s*\[circuit\]\s*\?\?\s*(\[[\d,\s]+\])/;
const WIND={
 tiersByCircuit:src=>{const g=grab(src.controls,TIERS);return g&&{value:JSON.parse(g[0])};},
 defaultTier:src=>{const g=grab(src.controls,TIERS);return g&&{value:JSON.parse(g[1])};},
 practiceIsCalm:flag('controls',/if\s*\(\s*practice\s*\)\s*return\s*\{\s*mph\s*:\s*0\s*,\s*angle\s*:\s*0\s*,\s*x\s*:\s*0\s*,\s*z\s*:\s*0\s*\}/),
 laterRoundsSwitchSides:flag('controls',/side\s*=\s*previousX\s*\?\s*-\s*Math\.sign\(\s*previousX\s*\)/),
};
// Platform overrides. Same field name, platform-specific source shape.
const LAUNCH_SPIN=/\bb\.spin\s*=\s*spin\s*\*\s*([^;]+?)\s*;\s*b\.curl\s*=\s*([^;]+?)\s*;/;
const OVERRIDES={
 SLICK_MU:member('SIDES',['slick','mu']),
 SLICK_MS:member('SIDES',['slick','ms']),
 SLICK_KEEP:member('SIDES',['slick','keep']),
 REL_Y:decl('REL_Y'),
 CAMERA_FOV:src=>{const g=grab(src.game,/\bcamera\.fov\s*=\s*([^;]+);/);return g&&{value:norm(g[0])};},
 // The multiplier actually used in launch(); an identifier is resolved from game or spin module.
 AIR_SPIN_RATE:src=>{
  const g=grab(src.game,LAUNCH_SPIN);if(!g)return undefined;
  const v=literal(g[0]);if(typeof v==='number')return {value:v};
  const d=declared(src.game,v)??declared(src.spin,v);
  return d&&{value:d.value};
 },
 FLIGHT_CURL:src=>{const g=grab(src.game,LAUNCH_SPIN);return g&&{value:literal(norm(g[1]))};},
 CURL:decl('CURL','spin'),
 // Human throw: is the player's spin passed straight to launch(), or zeroed for humans?
 HUMAN_SPIN:src=>{
  const g=grab(src.game,/\blaunch\(b,\s*releaseX\(team\),[^;\n]*?,\s*([^,;\n]+?)\s*\);/);if(!g)return undefined;
  const arg=norm(g[0]);
  return {value:arg==='M.spin'?true:/^isHuman\(team\)\?0:M\.spin$/.test(arg)?false:null};
 },
 POWER_INPUT:src=>{
  if(typeof src.trackball==='string'&&/peakSpeed\s*\/\s*this\.config\.powerSpeed/.test(src.trackball)&&declared(src.trackball,'SPEED_WINDOW_MS'))return {value:'swing-speed'};
  if(typeof src.controls==='string'&&/distance\s*\/\s*FULL_POWER_PULL/.test(src.controls)&&declared(src.controls,'FULL_POWER_PULL'))return {value:'pull-distance'};
  return {value:null};
 },
 SPEED_WINDOW_MS:decl('SPEED_WINDOW_MS','trackball'),
 POWER_SPEED_FULL:src=>{const d=declared(src.trackball,'DEFAULTS');return d&&d.value.powerSpeed!==undefined?{value:d.value.powerSpeed}:undefined;},
};

const WIND_METADATA=['tierMeaning','snippets'];
// Field keys used by baseline.json and approvals: constants and overrides by name,
// rules as `rule:<id>`, wind data as `wind:<key>`.
export function commonFields(common){
 return [
  ...common.constants.map(c=>({key:c.name,kind:'constant',entry:c})),
  ...common.rules.map(r=>({key:'rule:'+r.id,kind:'rule',entry:r})),
  ...Object.keys(common.wind).filter(k=>!WIND_METADATA.includes(k)).map(k=>({key:'wind:'+k,kind:'wind',entry:{value:common.wind[k]}})),
 ];
}
const extractors=kind=>({constant:CONSTANTS,rule:RULES,wind:WIND,override:OVERRIDES}[kind]);
const bare=key=>key.replace(/^(rule|wind):/,'');
export function extractField(kind,key,src){
 const fn=extractors(kind)[bare(key)];
 if(!fn)throw new Error(`no test-owned extractor for ${kind} ${key}: add one to shared/cornhole/lib/drift.mjs before recording it`);
 try{return fn(src);}catch(error){return {value:undefined,error:error.message};}
}
export const hasExtractor=(kind,key)=>!!extractors(kind)[bare(key)];

// ---------------------------------------------------------------- finding 1: recorded vs source
// sources: {platform: {game, controls, ...}} (text). data: {common, overrides}.
export function checkRecorded({common,overrides},sources){
 const errors=[];
 for(const platform of common.verifiedAgainst)if(!sources[platform])errors.push(`common.json is verified against ${platform}, but no sources were loaded for it`);
 for(const field of commonFields(common)){
  if(!hasExtractor(field.kind,field.key)){errors.push(`${field.key}: no test-owned extractor`);continue;}
  const perPlatform={};
  for(const platform of common.verifiedAgainst){
   if(!sources[platform])continue;
   const got=extractField(field.kind,field.key,sources[platform]);
   if(!got||got.value===undefined){errors.push(`${field.key}: not found in ${platform} source${got?.error?' ('+got.error+')':''}`);continue;}
   perPlatform[platform]=got.value;
   if(!same(field.entry.value,got.value))errors.push(`${field.key}: common.json value ${show(field.entry.value)} but ${platform} source uses ${show(got.value)}`);
   if(field.kind==='constant'&&field.entry.expr!==undefined){
    let fromExpr;
    try{fromExpr=evaluate(field.entry.expr,sources[platform].game);}catch(error){errors.push(`${field.key}: common.json expr ${show(field.entry.expr)} does not evaluate against ${platform} source (${error.message})`);}
    if(fromExpr!==undefined&&!same(fromExpr,got.value))errors.push(`${field.key}: common.json expr ${show(field.entry.expr)} = ${show(fromExpr)} but ${platform} source value is ${show(got.value)}`);
    if(got.expr!==undefined&&norm(got.expr)!==norm(field.entry.expr))errors.push(`${field.key}: common.json expr ${show(field.entry.expr)} but ${platform} source declares ${show(got.expr)}`);
   }
  }
  const values=Object.values(perPlatform);
  if(values.length>1&&!values.every(v=>same(v,values[0])))errors.push(`${field.key}: no longer identical across ${Object.keys(perPlatform).join(', ')} (${Object.entries(perPlatform).map(([p,v])=>p+'='+show(v)).join(', ')}); move it to overrides with an approval`);
 }
 for(const [platform,data] of Object.entries(overrides)){
  if(!sources[platform]){
   // Source-less platforms (wii) are checked against their baseline platform by checkApprovals.
   if(!data.baseline)for(const o of data.overrides)errors.push(`${platform} ${o.name}: platform has no sources and no baseline`);
   continue;
  }
  for(const o of data.overrides){
   if(!hasExtractor('override',o.name)){errors.push(`${platform} ${o.name}: no test-owned extractor`);continue;}
   const got=extractField('override',o.name,sources[platform]);
   if(!got||got.value===undefined||got.value===null){errors.push(`${platform} ${o.name}: not found in ${platform} source${got?.error?' ('+got.error+')':''}`);continue;}
   if(!same(o.value,got.value))errors.push(`${platform} ${o.name}: overrides/${platform}.json value ${show(o.value)} but source uses ${show(got.value)}`);
  }
 }
 // Intentional differences stay different: a field listed by two source-backed platforms must
 // actually differ between them, otherwise it belongs in common.json.
 const backed=Object.entries(overrides).filter(([p])=>sources[p]);
 for(let i=0;i<backed.length;i++)for(let j=i+1;j<backed.length;j++){
  const [pa,a]=backed[i],[pb,b]=backed[j];
  for(const o of a.overrides){const other=b.overrides.find(x=>x.name===o.name);if(other&&same(o.value,other.value))errors.push(`${o.name}: listed as a ${[pa,pb].sort().join('/')} difference but both use ${show(o.value)}; move it to common.json`);}
 }
 return errors;
}

// Values extracted from sources for every tracked field: {platform: {field: value}}.
export function currentValues({common,overrides},sources){
 const out={};
 for(const [platform,src] of Object.entries(sources)){
  const values=out[platform]={};
  if(common.verifiedAgainst.includes(platform))for(const field of commonFields(common))values[field.key]=extractField(field.kind,field.key,src)?.value??null;
  for(const o of overrides[platform]?.overrides??[])values[o.name]=extractField('override',o.name,src)?.value??null;
 }
 return out;
}

// ---------------------------------------------------------------- finding 2: exact approvals
// baseline: parsed baseline.json ({ref, values: {platform: {field: value}}}).
// current: currentValues(...) for the source-backed platforms.
// overrides: parsed overrides files (for source-less platforms such as wii, whose values are
// compared against their declared baseline platform).
export function drifts({baseline,current,overrides}){
 const out=[],errors=[];
 for(const [platform,values] of Object.entries(current)){
  const base=baseline.values[platform];
  if(!base){errors.push(`baseline.json has no values for ${platform}`);continue;}
  for(const [field,value] of Object.entries(values)){
   if(!(field in base)){errors.push(`baseline.json has no ${platform} ${field}: regenerate it from ${baseline.ref} with node shared/cornhole/tools/baseline.mjs --write`);continue;}
   if(!same(base[field],value))out.push({field,platform,old:base[field],new:value});
  }
  for(const field of Object.keys(base))if(!(field in values))out.push({field,platform,old:base[field],new:null});
 }
 for(const [platform,data] of Object.entries(overrides)){
  if(current[platform]||!data.baseline)continue;
  const base=baseline.values[data.baseline];
  if(!base){errors.push(`${platform} baseline ${data.baseline} has no values in baseline.json`);continue;}
  for(const o of data.overrides){
   const old=o.name in base?base[o.name]:null;
   if(!same(old,o.value))out.push({field:o.name,platform,old,new:o.value});
   else errors.push(`${platform} ${o.name}: override equals the ${data.baseline} baseline value ${show(old)}; remove it`);
  }
 }
 return {drifts:out,errors};
}
export function checkApprovals({baseline,current,overrides,approvals}){
 const {drifts:found,errors}=drifts({baseline,current,overrides});
 if(!Array.isArray(approvals))return [...errors,'approvals.json "approvals" must be an array'];
 const used=new Set();
 approvals.forEach((a,i)=>{
  const where=`approvals[${i}] ${show(a)}`;
  if(!a||typeof a!=='object'){errors.push(`${where}: not an object`);used.add(i);return;}
  for(const key of APPROVAL_KEYS)if(!(key in a))errors.push(`${where}: missing "${key}"`);
  for(const key of Object.keys(a))if(!APPROVAL_KEYS.includes(key)&&key!=='note')errors.push(`${where}: unknown key "${key}"`);
  if(!(typeof a.pr==='string'&&a.pr.trim())&&!(Number.isInteger(a.pr)&&a.pr>0))errors.push(`${where}: "pr" must name the PR (e.g. "#12" or a PR URL)`);
  if(a.baseline!==baseline.ref)errors.push(`${where}: baseline ${show(a.baseline)} is not the current baseline ${show(baseline.ref)}`);
  if(approvals.findIndex(b=>b&&b.field===a.field&&b.platform===a.platform)!==i)errors.push(`${where}: duplicate approval for ${a.platform} ${a.field}`);
 });
 for(const d of found){
  if(IMMUTABLE_PLATFORMS.includes(d.platform)){
   errors.push(`${d.platform} ${d.field}: changed from baseline ${show(d.old)} to ${show(d.new)}, but ${d.platform} is an immutable release snapshot; no approval can cover it`);
   continue;
  }
  const i=approvals.findIndex((a,i)=>!used.has(i)&&a&&a.field===d.field&&a.platform===d.platform&&same(a.old,d.old)&&same(a.new,d.new)&&a.baseline===baseline.ref&&((typeof a.pr==='string'&&a.pr.trim())||(Number.isInteger(a.pr)&&a.pr>0)));
  if(i>=0){used.add(i);continue;}
  const near=approvals.find(a=>a&&a.field===d.field&&a.platform===d.platform);
  errors.push(near
   ?`${d.platform} ${d.field}: approval does not match the drift. Needed ${show({field:d.field,platform:d.platform,old:d.old,new:d.new,baseline:baseline.ref})}, found ${show(near)}`
   :`${d.platform} ${d.field}: changed from baseline ${baseline.ref} value ${show(d.old)} to ${show(d.new)} with no approval. Add ${show({field:d.field,platform:d.platform,old:d.old,new:d.new,baseline:baseline.ref,pr:'#…'})} to approvals.json`);
 }
 approvals.forEach((a,i)=>{
  if(used.has(i))return;
  const live=found.some(d=>a&&d.field===a.field&&d.platform===a.platform);
  if(!live)errors.push(`approvals[${i}] ${show(a)}: stale, ${a?.platform} ${a?.field} does not differ from baseline ${baseline.ref}; remove it`);
 });
 return [...new Set(errors)];
}
