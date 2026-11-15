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

// ---------------------------------------------------------------- executable-code view
// Extraction must only see code the game runs. codeView(text) lexes the JavaScript (for an HTML
// file, only the bodies of its <script> elements) and returns:
//   code  the text with comments and non-script HTML replaced by spaces (newlines kept), so every
//         offset still lines up with the original
//   kind  per character: CODE, STR (string/template/regex literal) or GONE (comment, HTML)
// Matches are only accepted when they start in CODE and none of their capture groups touch a
// literal, so a commented-out or quoted old statement can neither match nor hide the active one.
const CODE=0,STR=1,GONE=2;
const REGEX_AFTER_WORD=new Set(['return','typeof','instanceof','in','of','new','delete','void','throw','case','do','else','yield','await',
 // Statements that are complete on their own: nothing can divide them, so after automatic semicolon
 // insertion (`debugger\n/re/.test(s)`, `break\n/re/`) the next `/` starts a regex statement.
 'debugger','break','continue','default','extends']);
// `(` after one of these opens a statement head: the matching `)` is followed by a statement, so a
// `/` there starts a regex (`if (x) /re/.test(s);`), not a division.
const CONTROL_HEADS=new Set(['if','while','for','with']);
function lexJs(text,from,to,kind){
 const braces=[];                     // template nesting: brace depth at each `${`
 const parens=[];                     // per open `(`: does it start a control-flow head?
 const headEnds=new Set();            // offsets of `)` that close a control-flow head
 let i=from,depth=0;
 const prevSignificant=at=>{let j=at-1;while(j>=from&&(kind[j]===GONE||/\s/.test(text[j])))j--;return j;};
 const wordEndingAt=j=>{let k=j;while(k>=from&&/[\w$]/.test(text[k]))k--;return k<j&&kind[j]===CODE?text.slice(k+1,j+1):'';};
 const opensHead=at=>{                // `(` at `at`: `if (`, `while (`, `for (`, `for await (`, `with (`
  let j=prevSignificant(at),w=wordEndingAt(j);
  if(w==='await'){j=prevSignificant(j-w.length+1);w=wordEndingAt(j);return w==='for';}
  return CONTROL_HEADS.has(w)&&!(j-w.length>=from&&text[j-w.length]==='.'); // not `obj.if(`
 };
 const regexAllowed=at=>{
  const j=prevSignificant(at);if(j<from)return true;
  const c=text[j];
  if(kind[j]===STR)return false;      // a literal just ended: `'a' / 2` (also `${...}` closing a template)
  if(/[\w$]/.test(c)){
   const w=wordEndingAt(j),start=j-w.length+1,before=prevSignificant(start);
   if(!w)return false;
   if(before>=from&&text[before]==='.'&&text[before-1]!=='.')return false; // property `a.return / 2`
   if(REGEX_AFTER_WORD.has(w))return true;
   // `break label` / `continue label` (label on the same line) is also a complete statement, so a
   // `/` after it (only valid after a newline, by ASI) starts a regex: `break outer\n/re/`.
   const v=wordEndingAt(before);
   return (v==='break'||v==='continue')&&/^[A-Za-z_$]/.test(w)&&!text.slice(before+1,start).includes('\n')
    &&!(before-v.length>=from&&text[prevSignificant(before-v.length+1)]==='.');
  }
  if(c===')')return headEnds.has(j);  // `(a + b) / 2` divides; `if (x) /re/` is a statement
  if(c===']')return false;            // `a[i] / 2`, `[1, 2] / 2`: always ends an expression
  // `}` closing a block (`if (x) {} /re/`, `function f() {} /re/`) is followed by a statement.
  // Only an object literal or function expression would be divided here, which no game code
  // does; erring toward "regex" can only hide text from extraction (a loud "not found"), never
  // expose literal text as code.
  if(c==='}')return true;
  return true;
 };
 const quoted=(q,start)=>{            // string literal starting at `start` (the quote)
  let j=start+1;
  while(j<to&&text[j]!==q&&text[j]!=='\n'){if(text[j]==='\\')j++;j++;}
  return Math.min(j+1,to);
 };
 const template=start=>{              // template text from `start` up to the closing ` or `${`
  let j=start;
  while(j<to){if(text[j]==='\\'){j+=2;continue;}if(text[j]==='`')return [j+1,false];if(text[j]==='$'&&text[j+1]==='{')return [j+2,true];j++;}
  return [to,false];
 };
 const mark=(a,b,k)=>{for(let j=a;j<b;j++)kind[j]=k;};
 while(i<to){
  const c=text[i],n=text[i+1];
  if(c==='/'&&n==='/'){let j=i;while(j<to&&text[j]!=='\n')j++;mark(i,j,GONE);i=j;continue;}
  if(c==='/'&&n==='*'){const e=text.indexOf('*/',i+2);const j=e<0||e+2>to?to:e+2;mark(i,j,GONE);i=j;continue;}
  if(c==='"'||c==="'"){const j=quoted(c,i);mark(i,j,STR);i=j;continue;}
  if(c==='`'){const [j,open]=template(i+1);mark(i,j,STR);if(open)braces.push(depth);i=j;continue;}
  if(c==='/'&&regexAllowed(i)){
   let j=i+1,cls=false;
   while(j<to&&text[j]!=='\n'){const d=text[j];if(d==='\\'){j+=2;continue;}if(d==='[')cls=true;else if(d===']')cls=false;else if(d==='/'&&!cls)break;j++;}
   j++;while(j<to&&/[a-z]/i.test(text[j]))j++;
   mark(i,Math.min(j,to),STR);i=Math.min(j,to);continue;
  }
  if(c==='(')parens.push(opensHead(i));
  else if(c===')'){if(parens.pop())headEnds.add(i);}
  else if(c==='{')depth++;
  else if(c==='}'){
   if(braces.length&&braces[braces.length-1]===depth){braces.pop();const [j,open]=template(i+1);mark(i,j,STR);if(open)braces.push(depth);i=j;continue;}
   depth--;
  }
  kind[i]=CODE;i++;
 }
}
const viewCache=new Map();
export function codeView(text){
 if(viewCache.has(text))return viewCache.get(text);
 const kind=new Uint8Array(text.length);
 const scripts=[...text.matchAll(/<script\b[^>]*>/gi)];
 if(scripts.length){
  kind.fill(GONE);                    // HTML: only <script> bodies are code
  for(const m of scripts){
   const from=m.index+m[0].length,end=text.toLowerCase().indexOf('</script',from);
   lexJs(text,from,end<0?text.length:end,kind);
  }
 }else lexJs(text,0,text.length,kind);
 let code='';
 for(let a=0;a<text.length;){let b=a;const gone=kind[a]===GONE;while(b<text.length&&(kind[b]===GONE)===gone)b++;code+=gone?text.slice(a,b).replace(/[^\n]/g,' '):text.slice(a,b);a=b;}
 const view={code,kind};
 if(viewCache.size>64)viewCache.clear();
 viewCache.set(text,view);
 return view;
}
const lineAt=(text,at)=>text.slice(0,at).split('\n').length;
// Every match of `re` in executable code: [{index, groups}] (groups are the capture strings).
// A match must start in code; unless literalCaptures is set (declarations, whose right-hand side
// may hold strings), its capture groups must not contain any string/template/regex literal text.
export function codeMatches(text,re,{literalCaptures=false}={}){
 if(typeof text!=='string')return [];
 const {code,kind}=codeView(text);
 const flags=re.flags.includes('g')?re.flags:re.flags+'g';
 const out=[];
 for(const m of code.matchAll(new RegExp(re.source,flags.includes('d')?flags:flags+'d'))){
  if(kind[m.index]!==CODE)continue;
  const inLiteral=!literalCaptures&&(m.indices??[]).slice(1).some(r=>{if(!r)return false;for(let j=r[0];j<r[1];j++)if(kind[j]===STR)return true;return false;});
  if(inLiteral)continue;
  out.push({index:m.index,groups:m.slice(1)});
 }
 return out;
}
const inCode=(text,re)=>codeMatches(text,re).length>0;

// ---------------------------------------------------------------- source declarations
// Top-level `const` / `export const` statements that end on the same line (the style of every
// file read here), in executable code only (not in comments or literals). Declarator lists
// (`const IN = 0.0254, FT = 0.3048;`) are split on top-level commas.
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
 for(const {groups:[list]} of codeMatches(text,/^(?:export\s+)?const\s+([^;\n]*);/m,{literalCaptures:true})){
  for(const part of splitTop(list)){
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
// Return the capture groups of the one match of a test-owned pattern in executable code, or
// undefined when there is none. More than one match is ambiguous and throws (extractField turns
// that into a "not found ... (ambiguous ...)" error) instead of picking the first occurrence.
const grab=(text,re)=>{
 const found=codeMatches(text,re);
 if(found.length>1)throw new Error(`ambiguous: ${found.length} executable matches for /${re.source}/ (lines ${found.map(m=>lineAt(text,m.index)).join(', ')})`);
 return found[0]?.groups;
};
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
  const literals=codeMatches(src.game,/\bstep\(b,\s*([\d.]+\s*\/\s*[\d.]+)\s*\)/).map(m=>m.groups[0]);
  const v=sole([...loop,...literals].map(e=>evaluate(e,src.game)));
  return v===undefined?undefined:{value:v};
 },
 WIND_MPH_TO_ACCEL:num('controls',/x\s*:\s*Math\.sin\(angle\)\s*\*\s*mph\s*\*\s*([\d.]+)\s*,\s*z\s*:\s*-\s*Math\.cos\(angle\)\s*\*\s*mph\s*\*\s*([\d.]+)/,g=>g[0]===g[1]||Number(g[0])===Number(g[1])?g[0]:undefined),
};
// Boolean rules: true when every pattern occurs in executable code (comments and literals don't count).
const flag=(file,...patterns)=>src=>typeof src[file]==='string'?{value:patterns.every(re=>inCode(src[file],re))}:undefined;
const RULES={
 'points-in-hole':num('game',/if\s*\(\s*b\.state\s*===\s*'in'\s*\)\s*\{\s*pts\[b\.team\]\s*\+=\s*(\d+)/),
 'points-on-board':num('game',/b\.state\s*===\s*'board'\s*&&\s*!b\.dead\s*\)\s*\{\s*pts\[b\.team\]\s*\+=\s*(\d+)/),
 'cancellation-scoring':src=>{
  if(typeof src.game!=='string')return undefined;
  const ok=inCode(src.game,/const\s+net\s*=\s*Math\.abs\(\s*pts\[0\]\s*-\s*pts\[1\]\s*\)/)&&inCode(src.game,/scorer\s*=\s*pts\[0\]\s*>\s*pts\[1\]\s*\?\s*0\s*:\s*pts\[1\]\s*>\s*pts\[0\]\s*\?\s*1\s*:\s*-1/);
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
  if(typeof src.trackball==='string'&&inCode(src.trackball,/peakSpeed\s*\/\s*this\.config\.powerSpeed/)&&declared(src.trackball,'SPEED_WINDOW_MS'))return {value:'swing-speed'};
  if(typeof src.controls==='string'&&inCode(src.controls,/distance\s*\/\s*FULL_POWER_PULL/)&&declared(src.controls,'FULL_POWER_PULL'))return {value:'pull-distance'};
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
