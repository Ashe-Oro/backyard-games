#!/usr/bin/env node
// Bounded runner for web/tests/MANIFEST. Dependency-free apart from playwright-core for the
// browser checks. Works from any working directory.
//
//   node web/tests/run-manifest.mjs                       run modern + known-fail checks
//   node web/tests/run-manifest.mjs --baseline-ref a07afaa   also run the same checks on that commit and compare
//   node web/tests/run-manifest.mjs --list                print the manifest classification and exit
//
// Everything a run depends on is explicit (flag, then env var, then default):
//   served root   --root DIR          (default <repo>/web). Served over HTTP; checks are read from DIR/tests/
//   base URL      --base-url URL      / BASE_URL. Unset: the runner serves the root itself on
//                                     127.0.0.1:--port (default 0 = any free port), like `python3 -m
//                                     http.server --directory web` (directory URLs redirect to a slash)
//   dependencies  NODE_PATH           (default <repo>/arcade/node_modules, which provides playwright-core;
//                                     install with `npm ci --prefix arcade`)
//   browser       --chrome PATH       / CHROME_PATH. Unset: the first of google-chrome, chromium,
//                                     chromium-browser on PATH; none: Playwright's own Chromium
//                                     (`npx playwright-core install chromium`)
//   working dir   the served root (as web/tests/run_all.py), for every check
//   artifacts     --artifacts DIR     / SHOTS. Default $RUNNER_TEMP/shots, else <tmp>/backyard-web-tests/<time>.
//                                     Checks write screenshots there (SHOTS); the runner writes one log per
//                                     check and results.json. (Two checks also write fixed /tmp/*.png files.)
//   bounds        --timeout SEC       per check (default 300); checks run one at a time
//   selection     --only a,b          run only these manifest files; --skip-known-failures
//
// Exit status: 0 when every modern check passes (and, with --baseline-ref, every outcome matches the
// baseline); 1 otherwise; 2 for setup errors (unclassified files, missing playwright-core, no server).
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import http from 'node:http';
import {spawn,spawnSync} from 'node:child_process';
import {createRequire} from 'node:module';
import {fileURLToPath} from 'node:url';

const here=path.dirname(fileURLToPath(import.meta.url));
const webRoot=path.resolve(here,'..'),repo=path.resolve(webRoot,'..');
const STATUSES=['modern','known-fail','legacy','tool'];
const RUNNABLE=/\.(c|m)?js$/;

function args(argv){
 const out={},flags=new Set(['list','skip-known-failures','help']);
 for(let i=0;i<argv.length;i++){
  const a=argv[i];if(!a.startsWith('--'))throw new Error(`unexpected argument ${a}`);
  const [k,v]=a.slice(2).split(/=(.*)/s);
  if(flags.has(k))out[k]=true;else out[k]=v??argv[++i];
 }
 return out;
}
const fail=(message,code=2)=>{console.error(`run-manifest: ${message}`);process.exit(code);};

export function readManifest(file=path.join(here,'MANIFEST')){
 const entries=[],seen=new Set();
 fs.readFileSync(file,'utf8').split('\n').forEach((line,i)=>{
  const text=line.replace(/^\s+|\s+$/g,'');if(!text||text.startsWith('#'))return;
  const [status,name,...note]=text.split(/\s+/);
  if(!STATUSES.includes(status))throw new Error(`MANIFEST:${i+1}: unknown status ${status}`);
  if(!name)throw new Error(`MANIFEST:${i+1}: missing file`);
  if(seen.has(name))throw new Error(`MANIFEST:${i+1}: ${name} listed twice`);
  seen.add(name);entries.push({status,file:name,note:note.join(' ')});
 });
 return entries;
}
// Every file under web/tests (except MANIFEST) must be classified, and every entry must exist.
export function inventory(entries,testsDir=here){
 const files=[];
 const walk=dir=>{for(const e of fs.readdirSync(dir,{withFileTypes:true})){const p=path.join(dir,e.name);if(e.isDirectory()){if(!['node_modules','__pycache__'].includes(e.name))walk(p);}else files.push(path.relative(testsDir,p).split(path.sep).join('/'));}};
 walk(testsDir);
 const listed=new Set(entries.map(e=>e.file)),errors=[];
 for(const f of files)if(f!=='MANIFEST'&&!listed.has(f))errors.push(`${f} is not classified in MANIFEST`);
 for(const e of entries)if(!files.includes(e.file))errors.push(`MANIFEST lists ${e.file}, which does not exist`);
 for(const e of entries)if(['modern','known-fail'].includes(e.status)&&!RUNNABLE.test(e.file))errors.push(`${e.file}: only .mjs/.cjs/.js checks can be modern/known-fail`);
 return errors;
}

const MIME={'.html':'text/html; charset=utf-8','.js':'text/javascript','.mjs':'text/javascript','.cjs':'text/javascript','.css':'text/css','.json':'application/json','.png':'image/png','.jpg':'image/jpeg','.jpeg':'image/jpeg','.gif':'image/gif','.svg':'image/svg+xml','.ico':'image/x-icon','.webp':'image/webp','.mp3':'audio/mpeg','.ogg':'audio/ogg','.wav':'audio/wav','.woff':'font/woff','.woff2':'font/woff2','.txt':'text/plain'};
function serve(root,port){
 const server=http.createServer((req,res)=>{
  let pathname;
  try{pathname=decodeURIComponent(new URL(req.url,'http://x').pathname);}catch{res.writeHead(400).end();return;}
  let file=path.join(root,pathname);
  if(!file.startsWith(root)){res.writeHead(403).end();return;}
  fs.stat(file,(err,stat)=>{
   if(!err&&stat.isDirectory()){
    if(!pathname.endsWith('/')){res.writeHead(301,{Location:pathname+'/'+(new URL(req.url,'http://x').search)}).end();return;}
    file=path.join(file,'index.html');
   }
   fs.readFile(file,(error,body)=>{
    if(error){res.writeHead(404,{'Content-Type':'text/plain'}).end('not found');return;}
    res.writeHead(200,{'Content-Type':MIME[path.extname(file).toLowerCase()]??'application/octet-stream','Cache-Control':'no-store'});
    res.end(req.method==='HEAD'?undefined:body);
   });
  });
 });
 return new Promise((resolve,reject)=>{server.once('error',reject);server.listen(port,'127.0.0.1',()=>resolve(server));});
}
async function ready(base){
 for(let i=0;i<50;i++){
  try{const r=await fetch(base+'/');if(r.ok)return true;}catch{}
  await new Promise(r=>setTimeout(r,200));
 }
 return false;
}
function pickChrome(explicit){
 if(explicit){if(!fs.existsSync(explicit))fail(`browser ${explicit} does not exist`);return explicit;}
 for(const name of ['google-chrome','google-chrome-stable','chromium','chromium-browser']){
  for(const dir of (process.env.PATH??'').split(path.delimiter)){const p=path.join(dir,name);if(dir&&fs.existsSync(p))return p;}
 }
 return '';
}
function runCheck(entry,{root,testsDir,env,timeout,logDir}){
 const file=path.join(testsDir,entry.file),log=path.join(logDir,entry.file.replace(/[\\/]/g,'_')+'.log');
 const started=Date.now();
 return new Promise(resolve=>{
  const out=fs.createWriteStream(log);
  const child=spawn(process.execPath,[file],{cwd:root,env,stdio:['ignore','pipe','pipe']});
  let tail='';
  const keep=chunk=>{out.write(chunk);tail=(tail+chunk).slice(-2000);};
  child.stdout.on('data',keep);child.stderr.on('data',keep);
  let timedOut=false;
  const timer=setTimeout(()=>{timedOut=true;child.kill('SIGKILL');},timeout*1000);
  child.on('close',code=>{
   clearTimeout(timer);out.end();
   resolve({file:entry.file,status:entry.status,outcome:!timedOut&&code===0?'pass':'fail',exitCode:code,timedOut,seconds:+((Date.now()-started)/1000).toFixed(1),log,tail:tail.trim().split('\n').slice(-4).join('\n')});
  });
 });
}

async function runSuite({label,root,testsDir,entries,opts,artifacts}){
 const logDir=path.join(artifacts,label,'logs'),shots=path.join(artifacts,label,'shots');
 fs.mkdirSync(logDir,{recursive:true});fs.mkdirSync(shots,{recursive:true});
 let server=null,base=opts.baseUrl;
 if(!base){server=await serve(root,opts.port);base=`http://127.0.0.1:${server.address().port}`;}
 if(!await ready(base))fail(`nothing answered ${base}/ (served root ${root})`);
 const env={...process.env,BASE_URL:base,SHOTS:shots,NODE_PATH:opts.nodePath,WEB_TESTS_CHROME:opts.chrome,
  NODE_OPTIONS:[process.env.NODE_OPTIONS,`--require=${path.join(here,'support','playwright-browser.cjs')}`].filter(Boolean).join(' ')};
 if(opts.chrome)env.CHROME_PATH=opts.chrome;
 console.log(`\n[${label}] served root ${root}\n[${label}] checks     ${testsDir}\n[${label}] base URL   ${base}${server?' (runner static server)':''}\n[${label}] artifacts  ${path.join(artifacts,label)}`);
 const results=[];
 for(const entry of entries){
  if(!fs.existsSync(path.join(testsDir,entry.file))){results.push({file:entry.file,status:entry.status,outcome:'missing',seconds:0});console.log(`  MISSING  ${entry.file}`);continue;}
  const r=await runCheck(entry,{root,testsDir,env,timeout:opts.timeout,logDir});
  results.push(r);
  const tag=r.outcome==='pass'?(entry.status==='known-fail'?'PASS?!':'PASS'):(entry.status==='known-fail'?'KNOWN':'FAIL');
  console.log(`  ${tag.padEnd(7)}${entry.file.padEnd(36)}${String(r.seconds).padStart(6)}s${r.timedOut?'  (timeout)':''}`);
  if(r.outcome!=='pass'&&entry.status==='modern')console.log('         '+r.tail.replace(/\n/g,'\n         '));
 }
 if(server)await new Promise(r=>server.close(r));
 return {label,root,base,results};
}
function summarize(suite){
 const by=(status,outcome)=>suite.results.filter(r=>r.status===status&&r.outcome===outcome).map(r=>r.file);
 return {modernPass:by('modern','pass'),modernFail:[...by('modern','fail'),...by('modern','missing')],knownFail:by('known-fail','fail'),knownNowPass:by('known-fail','pass')};
}
function extractRef(ref){
 const dir=fs.mkdtempSync(path.join(os.tmpdir(),`backyard-web-${ref.replace(/[^\w.-]/g,'_')}-`));
 const archive=spawnSync('git',['-C',repo,'archive','--format=tar',ref],{maxBuffer:1<<30});
 if(archive.status!==0)fail(`git archive ${ref} failed: ${archive.stderr}`);
 const tar=spawnSync('tar',['-x','-C',dir],{input:archive.stdout});
 if(tar.status!==0)fail(`tar failed for ${ref}: ${tar.stderr}`);
 const root=fs.existsSync(path.join(dir,'web','index.html'))?path.join(dir,'web'):dir;  // pre-move commits served the repo root
 return {dir,root};
}

async function main(){
 let o;try{o=args(process.argv.slice(2));}catch(e){fail(e.message);}
 if(o.help){console.log(fs.readFileSync(fileURLToPath(import.meta.url),'utf8').split('\n').slice(1,30).join('\n'));return;}
 const entries=readManifest();
 const problems=inventory(entries);
 if(problems.length)fail('MANIFEST does not match web/tests:\n  '+problems.join('\n  '));
 if(o.list){for(const s of STATUSES){const list=entries.filter(e=>e.status===s);console.log(`${s} (${list.length})`);for(const e of list)console.log(`  ${e.file}${e.note?'  — '+e.note:''}`);}return;}
 const only=o.only?new Set(o.only.split(',')):null;
 if(only)for(const f of only)if(!entries.some(e=>e.file===f))fail(`--only ${f}: not in MANIFEST`);
 const selected=entries.filter(e=>(e.status==='modern'||(e.status==='known-fail'&&!o['skip-known-failures']))&&(!only||only.has(e.file)));
 const root=path.resolve(o.root??webRoot);
 const nodePath=[process.env.NODE_PATH,path.join(repo,'arcade','node_modules')].filter(Boolean).join(path.delimiter);
 const needsBrowser=selected.some(e=>/\.c?js$/.test(e.file));
 if(needsBrowser){
  const req=createRequire(path.join(repo,'package.json'));
  try{req.resolve('playwright-core',{paths:nodePath.split(path.delimiter)});}
  catch{fail(`playwright-core not found on NODE_PATH (${nodePath}). Run: npm ci --prefix arcade`);}
 }
 const chrome=pickChrome(o.chrome??process.env.CHROME_PATH);
 const stamp=new Date().toISOString().replace(/[:.]/g,'-');
 const artifacts=path.resolve(o.artifacts??process.env.SHOTS??(process.env.RUNNER_TEMP?path.join(process.env.RUNNER_TEMP,'shots'):path.join(os.tmpdir(),'backyard-web-tests',stamp)));
 const opts={baseUrl:(o['base-url']??process.env.BASE_URL??'').replace(/\/$/,''),port:Number(o.port??0),timeout:Number(o.timeout??300),nodePath,chrome};
 console.log(`run-manifest: ${selected.length} checks (${selected.filter(e=>e.status==='modern').length} modern, ${selected.filter(e=>e.status==='known-fail').length} known-fail); ${entries.filter(e=>e.status==='legacy').length} legacy not run`);
 console.log(`run-manifest: node ${process.version}, NODE_PATH ${nodePath}, browser ${chrome||"Playwright's bundled Chromium"}, cwd = served root, timeout ${opts.timeout}s/check`);

 const suites=[await runSuite({label:'current',root,testsDir:path.join(root,'tests'),entries:selected,opts,artifacts})];
 let baseline=null;
 if(o['baseline-ref']){
  if(opts.baseUrl)fail('--baseline-ref needs the runner static server; drop --base-url/BASE_URL');
  baseline=extractRef(o['baseline-ref']);
  suites.push(await runSuite({label:o['baseline-ref'],root:baseline.root,testsDir:path.join(baseline.root,'tests'),entries:selected,opts,artifacts}));
 }
 const summary=summarize(suites[0]);
 const report={manifest:path.join(here,'MANIFEST'),node:process.version,browser:chrome||'playwright-bundled',nodePath,timeoutSeconds:opts.timeout,legacyNotRun:entries.filter(e=>e.status==='legacy').map(e=>e.file),suites,summary};
 let mismatches=[];
 if(baseline){
  const base=new Map(suites[1].results.map(r=>[r.file,r.outcome]));
  mismatches=suites[0].results.filter(r=>base.get(r.file)!==r.outcome).map(r=>({file:r.file,current:r.outcome,baseline:base.get(r.file)}));
  report.baselineRef=o['baseline-ref'];report.baselineMismatches=mismatches;
  // Identical check files (by content) at the baseline and now: proves the selection didn't change.
  report.changedCheckFiles=selected.filter(e=>{const a=path.join(root,'tests',e.file),b=path.join(baseline.root,'tests',e.file);return !fs.existsSync(b)||!fs.readFileSync(a).equals(fs.readFileSync(b));}).map(e=>e.file);
  fs.rmSync(baseline.dir,{recursive:true,force:true});
 }
 fs.writeFileSync(path.join(artifacts,'results.json'),JSON.stringify(report,null,1)+'\n');

 console.log(`\nmodern:     ${summary.modernPass.length} pass, ${summary.modernFail.length} fail${summary.modernFail.length?' -> '+summary.modernFail.join(', '):''}`);
 console.log(`known-fail: ${summary.knownFail.length} still failing (accepted baseline exceptions)${summary.knownFail.length?': '+summary.knownFail.join(', '):''}`);
 if(summary.knownNowPass.length)console.log(`known-fail now PASSING (move to modern in MANIFEST): ${summary.knownNowPass.join(', ')}`);
 if(baseline){
  const b=summarize(suites[1]);
  console.log(`${o['baseline-ref']}:    modern ${b.modernPass.length} pass / ${b.modernFail.length} fail, known-fail ${b.knownFail.length} failing / ${b.knownNowPass.length} passing`);
  console.log(`check files changed since ${o['baseline-ref']}: ${report.changedCheckFiles.length?report.changedCheckFiles.join(', '):'none'}`);
  console.log(mismatches.length?`OUTCOME DIFFERS from ${o['baseline-ref']}: ${mismatches.map(m=>`${m.file} (${m.baseline} -> ${m.current})`).join(', ')}`:`per-check outcomes identical to ${o['baseline-ref']}`);
 }
 console.log(`results: ${path.join(artifacts,'results.json')}`);
 process.exitCode=summary.modernFail.length||mismatches.length?1:0;
}
if(process.argv[1]&&path.resolve(process.argv[1])===fileURLToPath(import.meta.url))main().catch(e=>fail(e.stack??String(e)));
