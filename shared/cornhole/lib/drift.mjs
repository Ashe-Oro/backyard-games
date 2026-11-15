// Drift-check library for shared/cornhole.
//
// The JSON files record values; this module reads the *game sources* and extracts the
// values they actually use, with test-owned locators. Nothing here trusts a JSON `snippet`,
// `expr` or `value` to describe the source: those are what get checked.
//
// Sources are parsed with real parsers, never lexed by hand: an HTML file goes through parse5 (the
// WHATWG HTML parser, so only actual JavaScript <script> elements count, not "<script>" text in an
// HTML comment, a <template>, or a comment/string of an already-open script), and every script body
// and .mjs module through acorn. Extractors match AST nodes against small code templates, so a
// comment, string, template or regex literal holding an old statement is never code.
// Dev dependencies (shared/cornhole/package.json): npm ci --prefix shared/cornhole
//
//   extractField(key, src)        value the source actually uses for a field (undefined = not found)
//   checkRecorded(data, sources)  JSON values/expressions vs extracted source values  (review finding 1)
//   currentValues(data, sources)  {platform: {field: value}} extracted from sources
//   checkApprovals({...})         drift vs baseline must have exact approvals            (review finding 2)
import vm from 'node:vm';
import {createHash} from 'node:crypto';

const need=async name=>{
 try{return await import(name);}
 catch(error){
  if(error?.code==='ERR_MODULE_NOT_FOUND')throw new Error(`shared/cornhole/lib/drift.mjs needs the ${name} parser: run \`npm ci --prefix shared/cornhole\` from the repo root (${error.message})`);
  throw error;
 }
};
const acorn=await need('acorn');
const {parse:parseHtml}=await need('parse5');

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

// ---------------------------------------------------------------- parsing
// parsed(text) parses a source once (cached by text):
//   html      true when the text is an HTML document (it starts with `<`, which no JavaScript module can)
//   ranges    [{from, to, module}] executable JavaScript: every <script> element parse5 finds whose type
//             is classic JavaScript or "module" and that has no src (for a .mjs file, the whole text)
//   programs  acorn ASTs, one per range; offsets and line numbers are those of the original file
//   comments  [[start, end]] comment ranges reported by acorn
//   nodes     every AST node; lists: every statement list (Program/block/static block/switch case bodies)
// A script that acorn cannot parse throws: extraction then fails loudly instead of guessing.
const HTML_NS='http://www.w3.org/1999/xhtml';
// HTML "JavaScript MIME type essence match" strings; an absent or empty type is classic JavaScript.
const JS_TYPES=new Set(['','application/ecmascript','application/javascript','application/x-ecmascript','application/x-javascript','text/ecmascript','text/javascript',
 'text/javascript1.0','text/javascript1.1','text/javascript1.2','text/javascript1.3','text/javascript1.4','text/javascript1.5','text/jscript','text/livescript','text/x-ecmascript','text/x-javascript']);
function scriptRanges(html){
 const out=[];
 const visit=node=>{
  if(node.nodeName==='script'&&node.namespaceURI===HTML_NS){
   const attr=name=>node.attrs.find(a=>a.name===name)?.value;
   const type=(attr('type')??(attr('language')?'text/'+attr('language'):'')).trim().toLowerCase();
   const body=node.childNodes[0]?.sourceCodeLocation;
   if((type==='module'||JS_TYPES.has(type))&&attr('src')===undefined&&body)out.push({from:body.startOffset,to:body.endOffset,module:type==='module'});
  }
  for(const child of node.childNodes??[])visit(child); // <template> content is a separate fragment: inert, not visited
 };
 visit(parseHtml(html,{sourceCodeLocationInfo:true}));
 return out;
}
const LINE_END=/[\n\r\u2028\u2029]/;
const blank=s=>s.replace(/[^\n\r\u2028\u2029]/g,' ');
function walk(node,visit,parent=null,key=null){
 visit(node,parent,key);
 for(const k of Object.keys(node)){
  if(k==='loc')continue;          // every child node sits under some other key (values that are nodes have a string `type`)
  const v=node[k];
  if(Array.isArray(v)){for(const c of v)if(c&&typeof c.type==='string')walk(c,visit,node,k);}
  else if(v&&typeof v.type==='string')walk(v,visit,node,k);
 }
}
const parseCache=new Map();
function parsed(text){
 if(typeof text!=='string')return undefined;
 if(parseCache.has(text))return parseCache.get(text);
 const html=/^\s*</.test(text);
 const ranges=html?scriptRanges(text):[{from:0,to:text.length,module:true}];
 const programs=[],comments=[],nodes=[],lists=[];
 for(const {from,to,module} of ranges){
  // Pad everything outside the script with blanks (line ends kept) so acorn's offsets and lines are the file's.
  const input=html?blank(text.slice(0,from))+text.slice(from,to)+blank(text.slice(to)):text;
  let program;
  try{program=acorn.parse(input,{ecmaVersion:'latest',sourceType:module?'module':'script',locations:true,allowHashBang:true,onComment:(block,t,start,end)=>comments.push([start,end])});}
  catch(error){throw new Error(`${html?`<script> at line ${text.slice(0,from).split(LINE_END).length}`:'module'} does not parse as JavaScript: ${error.message}`);}
  programs.push(program);
  walk(program,node=>{
   nodes.push(node);
   if(node.type==='Program'||node.type==='BlockStatement'||node.type==='StaticBlock')lists.push(node.body);
   else if(node.type==='SwitchCase')lists.push(node.consequent);
  });
 }
 let code='',at=0;                    // comments blanked, offsets unchanged
 for(const [a,b] of [...comments].sort((x,y)=>x[0]-y[0])){code+=text.slice(at,a)+blank(text.slice(a,b));at=b;}
 code+=text.slice(at);
 const p={text,html,ranges,programs,comments,nodes,lists,code};
 if(parseCache.size>=8)parseCache.delete(parseCache.keys().next().value);
 parseCache.set(text,p);
 return p;
}
// Source text of a node with any comments inside it blanked.
const textOf=(p,node)=>p.code.slice(node.start,node.end).trim();

// Per-character view, for tests and debugging: kind is CODE, STR (string/template/regex literal text)
// or GONE (comment, HTML outside executable scripts); code is the text with GONE blanked. Derived
// from the acorn AST and comment list, so it shows exactly what the extractors can and cannot see.
const CODE=0,STR=1,GONE=2;
export function codeView(text){
 const p=parsed(text);
 if(p.view)return p.view;
 const kind=new Uint8Array(text.length).fill(p.html?GONE:CODE);
 for(const {from,to} of p.ranges)kind.fill(CODE,from,to);
 for(const node of p.nodes){
  if(node.type==='Literal'&&(typeof node.value==='string'||node.regex))kind.fill(STR,node.start,node.end);
  else if(node.type==='TemplateLiteral')node.quasis.forEach((q,i)=>kind.fill(STR,i?q.start-1:node.start,q.end+(q.tail?1:2)));
 }
 for(const [a,b] of p.comments)kind.fill(GONE,a,b);
 let code='';
 for(let a=0;a<text.length;){let b=a;const gone=kind[a]===GONE;while(b<text.length&&(kind[b]===GONE)===gone)b++;code+=gone?blank(text.slice(a,b)):text.slice(a,b);a=b;}
 return p.view={code,kind};
}

// ---------------------------------------------------------------- AST templates
// A pattern is JavaScript parsed by acorn. In a template:
//   $NAME        (identifier) matches any node and captures it as caps.$NAME; `$NAME;` matches any statement
//   ...$$ / $$;  matches zero or more list items: arguments, array elements, object properties, statements
//   if (…) …     without an else accepts any else branch
// Literals compare by value (`.025` matches `0.025`), parentheses and whitespace don't matter, and
// everything else must be structurally identical. E(code) is an expression, S(code) one or more
// statements (several must be consecutive in one statement list), D(code) the declarator of
// `const x = …;` (matches const/let/var). anyOf(...) accepts any alternative.
const TEMPLATE={ecmaVersion:'latest',sourceType:'module',allowReturnOutsideFunction:true};
const pattern=(code,body)=>({code,alts:[body]});
const E=code=>{
 const node=acorn.parseExpressionAt(code,0,TEMPLATE);
 if(code.slice(node.end).trim())throw new Error(`template is not one expression: ${code}`);
 return pattern(code,[node]);
};
const S=code=>pattern(code,acorn.parse(code,TEMPLATE).body);
const D=code=>pattern(code,[acorn.parse(code,TEMPLATE).body[0].declarations[0]]);
const anyOf=(...ps)=>({code:ps.map(p=>p.code).join(' | '),alts:ps.flatMap(p=>p.alts)});
const REST='$$';
const isHole=n=>n?.type==='Identifier'&&n.name!==REST&&n.name.startsWith('$');
const isRest=n=>(n?.type==='SpreadElement'||n?.type==='ExpressionStatement')&&(n.argument??n.expression)?.type==='Identifier'&&(n.argument??n.expression).name===REST;
const IGNORED=new Set(['type','start','end','loc','range','raw','shorthand']);
function match(t,s,caps){
 if(t==null||s==null)return t==s;
 if(t.type==='ExpressionStatement'&&isHole(t.expression)){if(!/Statement|Declaration$/.test(s.type))return false;caps[t.expression.name]=s;return true;}
 if(isHole(t)){caps[t.name]=s;return true;}
 if(t.type!==s.type)return false;
 if(t.type==='Literal')return t.regex?!!s.regex&&t.regex.pattern===s.regex.pattern&&t.regex.flags===s.regex.flags
  :!s.regex&&(t.bigint!==undefined?t.bigint===s.bigint:t.value===s.value);
 if(t.type==='TemplateElement')return t.tail===s.tail&&t.value.cooked===s.value.cooked;
 for(const key of Object.keys(t)){
  if(IGNORED.has(key)||(key==='alternate'&&t.type==='IfStatement'&&t.alternate===null))continue;
  const tv=t[key],sv=s[key];
  if(Array.isArray(tv)){if(!Array.isArray(sv)||!matchList(tv,sv,caps))return false;}
  else if(tv&&typeof tv==='object'){if(!match(tv,sv,caps))return false;}
  else if(tv!==sv)return false;
 }
 return true;
}
function matchList(ts,ss,caps,i=0,j=0){
 if(i===ts.length)return j===ss.length;
 const tries=isRest(ts[i])?Array.from({length:ss.length-j+1},(_,k)=>[i+1,j+k,null]):j<ss.length?[[i+1,j+1,ts[i]]]:[];
 for(const [ni,nj,t] of tries){
  const c={...caps};
  if(t&&!match(t,ss[j],c))continue;
  if(matchList(ts,ss,c,ni,nj)){Object.assign(caps,c);return true;}
 }
 return false;
}
const TAIL={type:'ExpressionStatement',expression:{type:'Identifier',name:REST}};
// Every match of a pattern in executable code: [{node, caps, line}]. `within` limits the search to
// one node's subtree.
export function findAll(text,pat,within=null){
 const p=parsed(text);
 if(!p)return [];
 const out=[],seen=new Set();
 const add=(node,caps)=>{if(!seen.has(node)){seen.add(node);out.push({node,caps,line:node.loc.start.line});}};
 const inside=n=>!within||(n.start>=within.start&&n.end<=within.end);
 for(const body of pat.alts){
  if(body.length===1){for(const node of p.nodes){const caps={};if(inside(node)&&match(body[0],node,caps))add(node,caps);}}
  else for(const list of p.lists)for(let k=0;k<list.length;k++){const caps={};if(inside(list[k])&&matchList([...body,TAIL],list.slice(k),caps))add(list[k],caps);}
 }
 return out.sort((a,b)=>a.node.start-b.node.start);
}
const has=(text,pat)=>findAll(text,pat).length>0;
// The captures of the one match of a test-owned pattern, or undefined when there is none. More than
// one match is ambiguous and throws (extractField turns that into a "not found ... (ambiguous ...)"
// error) instead of picking the first occurrence.
function grab(text,pat){
 const found=findAll(text,pat);
 if(found.length>1)throw new Error(`ambiguous: ${found.length} executable matches for \`${pat.code}\` (lines ${found.map(m=>m.line).join(', ')})`);
 return found[0]&&{...found[0].caps,$0:found[0].node};
}
const numberOf=node=>node?.type==='Literal'&&typeof node.value==='number'?node.value:undefined;
// Value of a literal-only expression (numbers, strings, arrays, plain objects, unary +/-), else undefined.
function literalValue(node){
 switch(node?.type){
  case 'Literal':return node.regex||node.bigint!==undefined?undefined:node.value;
  case 'UnaryExpression':{const v=literalValue(node.argument);return (node.operator==='-'||node.operator==='+')&&typeof v==='number'?(node.operator==='-'?-v:v):undefined;}
  case 'ArrayExpression':{const out=node.elements.map(literalValue);return out.every(v=>v!==undefined)?out:undefined;}
  case 'ObjectExpression':{
   const out={};
   for(const prop of node.properties){
    if(prop.type!=='Property'||prop.kind!=='init'||prop.computed||prop.method)return undefined;
    const v=literalValue(prop.value);if(v===undefined)return undefined;
    out[prop.key.type==='Identifier'?prop.key.name:String(prop.key.value)]=v;
   }
   return out;
  }
  default:return undefined;
 }
}

// ---------------------------------------------------------------- source declarations
// Top-level `const` / `export const` declarations (Program body, any script or module of the file).
// A name declared more than once is ambiguous and is not resolved.
function declarations(text){
 const p=parsed(text);
 if(!p)return new Map();
 if(p.decls)return p.decls;
 const map=new Map(),dupes=new Set();
 for(const program of p.programs)for(let statement of program.body){
  if(statement.type==='ExportNamedDeclaration'&&statement.declaration)statement=statement.declaration;
  if(statement.type!=='VariableDeclaration'||statement.kind!=='const')continue;
  for(const d of statement.declarations){
   if(d.id.type!=='Identifier'||!d.init)continue;
   if(map.has(d.id.name))dupes.add(d.id.name);
   map.set(d.id.name,textOf(p,d.init));
  }
 }
 for(const name of dupes)map.set(name,undefined); // ambiguous: refuse to guess
 return p.decls=map;
}
// Identifiers an expression reads (not property names or object keys), from its acorn AST.
function freeNames(expr){
 const node=acorn.parseExpressionAt(expr,0,{ecmaVersion:'latest'});
 if(expr.slice(node.end).trim())throw new Error(`not one expression: ${show(expr)}`);
 const names=new Set();
 walk(node,(n,parent,key)=>{
  if(n.type!=='Identifier')return;
  if(key==='property'&&parent.type==='MemberExpression'&&!parent.computed)return;
  if(key==='key'&&!parent.computed&&/^(Property|PropertyDefinition|MethodDefinition)$/.test(parent.type))return;
  names.add(n.name);
 });
 return names;
}
// Evaluate an expression from the source, resolving identifiers from the same file's top-level
// declarations (recursively). Only Math and Object.freeze are available otherwise.
function evaluate(expr,text,extra={},seen=new Set()){
 const decls=declarations(text),context={Math,Object:{freeze:Object.freeze},...extra};
 for(const id of freeNames(expr)){
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
// The numeric literal captured by the one match of `pat` (pick chooses the capture).
const num=(file,pat,pick=c=>c.$N)=>src=>{const c=grab(src[file],pat);if(!c)return undefined;const v=numberOf(pick(c));return v===undefined?undefined:{value:v};};
const equalNumbers=(a,b)=>c=>numberOf(c[a])!==undefined&&numberOf(c[a])===numberOf(c[b])?c[a]:undefined;

const LAND_KEEP=E('S.keep * (b.arc === 2 ? $KOBE : b.arc === 0 ? $PUSH : 1)');
const DT_LOOP=S('while (acc >= $X) { acc -= $Y; $$; }');
const STEP=E('step(b, $DT)');
const STEP_LITERAL=E('step(b, $A / $B)');
const CONSTANTS={
 ...Object.fromEntries(['G','IN','FT','W','L','FRONT_H','BACK_H','TH','BOARD_Z','HOLE_FROM_TOP','HOLE_R','BAG','BAG_H','BAG_R','MU_GROUND','BAG_E','ELEV','V_MIN','AIM_MAX','REL_Z'].map(n=>[n,decl(n)])),
 STICKY_MU:member('SIDES',['sticky','mu']),
 STICKY_MS:member('SIDES',['sticky','ms']),
 STICKY_KEEP:member('SIDES',['sticky','keep']),
 ARC_PUSH_DEG:index('arcs',0),
 ARC_NORMAL_DEG:index('arcs',1),
 ARC_KOBE_DEG:index('arcs',2),
 LAND_KEEP_KOBE:num('game',LAND_KEEP,c=>c.$KOBE),
 LAND_KEEP_PUSH:num('game',LAND_KEEP,c=>c.$PUSH),
 SWISH_R_FRAC:num('game',E('holeDist(lp) < HOLE_R * $N')),
 SLIDE_IN_MARGIN:num('game',E('holeDist(lp) < HOLE_R - $N')),
 // The fixed-step loop (`while (acc >= X) { acc -= X; ... step(b, X) ... }`) and every literal
 // step(b, N / M) call (AI previews) must agree on one step.
 PHYSICS_DT:src=>{
  const loop=grab(src.game,DT_LOOP);
  if(!loop)return undefined;
  const p=parsed(src.game),steps=findAll(src.game,STEP,loop.$0.body).map(m=>m.caps.$DT);
  if(!steps.length)return undefined;
  const literals=findAll(src.game,STEP_LITERAL).filter(m=>numberOf(m.caps.$A)!==undefined&&numberOf(m.caps.$B)!==undefined).map(m=>m.caps.$A.value/m.caps.$B.value);
  const v=sole([...[loop.$X,loop.$Y,...steps].map(n=>evaluate(textOf(p,n),src.game)),...literals]);
  return v===undefined?undefined:{value:v};
 },
 WIND_MPH_TO_ACCEL:num('controls',E('{...$$, x: Math.sin(angle) * mph * $X, z: -Math.cos(angle) * mph * $Z, ...$$}'),equalNumbers('$X','$Z')),
};
// Boolean rules: true when every pattern occurs in executable code (comments and literals don't count).
const flag=(file,...patterns)=>src=>typeof src[file]==='string'?{value:patterns.every(pat=>has(src[file],pat))}:undefined;
const ON_BOARD=S("if (b.state === 'board' && !b.dead) { pts[b.team] += $N; $$; }");
const RULES={
 'points-in-hole':num('game',S("if (b.state === 'in') { pts[b.team] += $N; $$; }")),
 'points-on-board':num('game',ON_BOARD),
 'cancellation-scoring':src=>{
  if(typeof src.game!=='string')return undefined;
  const ok=has(src.game,D('const net = Math.abs(pts[0] - pts[1]);'))
   &&has(src.game,anyOf(D('const scorer = pts[0] > pts[1] ? 0 : pts[1] > pts[0] ? 1 : -1;'),E('scorer = pts[0] > pts[1] ? 0 : pts[1] > pts[0] ? 1 : -1')));
  return {value:ok?'net = |red - blue|, awarded to the higher side; a tie is a wash':null};
 },
 'target-score':num('game',E('M.exact ? M.scores[scorer] === $A : M.scores[scorer] >= $B'),equalNumbers('$A','$B')),
 'exact-21-bust-to':num('game',S('if (M.exact && M.scores[scorer] > $LIMIT) { M.scores[scorer] = $N; $$; }')),
 'last-scorer-throws-first':flag('game',E('M.first = scorer')),
 'bags-per-inning':num('game',S("if (!M.practice && M.k >= $N) { M.phase = 'settle'; $$; }")),
 'alternating-throws':flag('game',E('k % 2 === 0 ? M.first : 1 - M.first')),
 'grounder-is-dead':flag('game',S("if (b.dead) say('GROUNDER', ...$$);"),ON_BOARD),
};
const TIERS=E('$TIERS[circuit] ?? $DEFAULT');
const isNumbers=v=>Array.isArray(v)&&v.every(n=>typeof n==='number');
const WIND={
 tiersByCircuit:src=>{const c=grab(src.controls,TIERS);const v=c&&literalValue(c.$TIERS);return Array.isArray(v)&&v.every(isNumbers)?{value:v}:undefined;},
 defaultTier:src=>{const c=grab(src.controls,TIERS);const v=c&&literalValue(c.$DEFAULT);return isNumbers(v)?{value:v}:undefined;},
 practiceIsCalm:flag('controls',S('if (practice) return {mph: 0, angle: 0, x: 0, z: 0};')),
 laterRoundsSwitchSides:flag('controls',anyOf(D('const side = previousX ? -Math.sign(previousX) : $OTHER;'),E('side = previousX ? -Math.sign(previousX) : $OTHER'))),
};
// Platform overrides. Same field name, platform-specific source shape.
const LAUNCH_SPIN=S('b.spin = spin * $RATE; b.curl = $CURL;');
const nodeText=(file,pick)=>src=>{const c=grab(src[file],pick.pat);return c&&{value:pick.value(textOf(parsed(src[file]),pick.node(c)))};};
const OVERRIDES={
 SLICK_MU:member('SIDES',['slick','mu']),
 SLICK_MS:member('SIDES',['slick','ms']),
 SLICK_KEEP:member('SIDES',['slick','keep']),
 REL_Y:decl('REL_Y'),
 CAMERA_FOV:nodeText('game',{pat:E('camera.fov = $FOV'),node:c=>c.$FOV,value:norm}),
 // The multiplier actually used in launch(); an identifier is resolved from game or spin module.
 AIR_SPIN_RATE:src=>{
  const c=grab(src.game,LAUNCH_SPIN);if(!c)return undefined;
  if(numberOf(c.$RATE)!==undefined)return {value:c.$RATE.value};
  if(c.$RATE.type!=='Identifier')return undefined;
  const d=declared(src.game,c.$RATE.name)??declared(src.spin,c.$RATE.name);
  return d&&{value:d.value};
 },
 FLIGHT_CURL:nodeText('game',{pat:LAUNCH_SPIN,node:c=>c.$CURL,value:t=>literal(norm(t))}),
 CURL:decl('CURL','spin'),
 // Human throw: is the player's spin passed straight to launch(), or zeroed for humans?
 HUMAN_SPIN:nodeText('game',{pat:E('launch(b, releaseX(team), $POWER, ...$$, $SPIN)'),node:c=>c.$SPIN,
  value:t=>{const arg=norm(t);return arg==='M.spin'?true:/^isHuman\(team\)\?0:M\.spin$/.test(arg)?false:null;}}),
 POWER_INPUT:src=>{
  if(typeof src.trackball==='string'&&has(src.trackball,anyOf(E('peakSpeed / this.config.powerSpeed'),E('$OBJ.peakSpeed / this.config.powerSpeed')))&&declared(src.trackball,'SPEED_WINDOW_MS'))return {value:'swing-speed'};
  if(typeof src.controls==='string'&&has(src.controls,E('distance / FULL_POWER_PULL'))&&declared(src.controls,'FULL_POWER_PULL'))return {value:'pull-distance'};
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
