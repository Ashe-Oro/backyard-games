import {test} from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs/promises';
import path from 'node:path';
import {fileURLToPath} from 'node:url';
import {createHash} from 'node:crypto';
import {execFileSync} from 'node:child_process';
const arcade=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'..'),repo=path.dirname(arcade);
async function webSources(){
 const result=new Map();
 async function visit(relative){
  const file=path.join(repo,relative),stat=await fs.stat(file);
  if(stat.isDirectory()){for(const child of await fs.readdir(file))await visit(path.join(relative,child));}
  else if(/\.(html|css|m?js|json)$/.test(file))result.set(relative,await fs.readFile(file,'utf8'));
 }
 for(const entry of ['index.html','cornhole','yard-games','beer-pong','ladder-golf','washers','lawn-darts','assets','vercel.json'])await visit(entry);
 return result;
}
const hashes=sources=>Object.fromEntries([...sources].sort().map(([file,text])=>[file,createHash('sha256').update(text).digest('hex')]));
test('arcade packaging cannot mutate web sources or retain stale web output',async()=>{
 const before=await webSources(),site=path.join(arcade,'dist/site');
 await fs.mkdir(site,{recursive:true});await fs.writeFile(path.join(site,'stale-web-file.html'),'not part of the arcade game');
 execFileSync(process.execPath,['build.mjs','--site'],{cwd:arcade,stdio:'pipe'});
 assert.deepEqual(hashes(await webSources()),hashes(before),'The arcade build changed canonical website files');
 await assert.rejects(fs.access(path.join(site,'stale-web-file.html')),{code:'ENOENT'});
 for(const [file,text] of before)assert.doesNotMatch(text,/__arcadeHost|cabinetInput|\/arcade\//,`${file} imports or enables cabinet behavior`);
 const web=before.get(path.join('cornhole','index.html')),generated=await fs.readFile(path.join(site,'cornhole/index.html'),'utf8');
 assert.doesNotMatch(web,/data-backyard-arcade/);
 assert.match(generated,/data-backyard-arcade/);assert.match(generated,/window\.__arcadeHost=/);assert.match(generated,/import\('\/arcade\/controller.js'\)/);
 assert.match(web,/\['PUSH', 'NORMAL', 'KOBE'\]/);assert.match(generated,/\['SLIDER', 'NORMAL', 'KOBE'\]/);
 for(const file of ['backyard.js','power-arrow.js','ui.css'])assert.equal(await fs.readFile(path.join(site,'yard-games',file),'utf8'),before.get(path.join('yard-games',file)));
});
test('web deployment excludes the entire arcade tree',async()=>{
 const rules=(await fs.readFile(path.join(repo,'.vercelignore'),'utf8')).split(/\r?\n/).map(s=>s.trim()).filter(s=>s&&!s.startsWith('#'));
 assert(rules.includes('arcade'),'Arcade code and generated runtimes must be excluded from Vercel');
 assert(!rules.some(rule=>rule.startsWith('!')&&rule.includes('arcade')),'A deployment rule re-includes arcade files');
});
test('accidental arcade import rejects before touching web storage or UI',()=>{
 const controller=new URL('../controller.js',import.meta.url).href;
 const code=`import assert from 'node:assert/strict';
 globalThis.window={__arcadeHost:{}};
 globalThis.document={documentElement:{hasAttribute:()=>false}};
 Object.defineProperty(globalThis,'localStorage',{get(){throw new Error('Touched browser saves');}});
 await assert.rejects(import(${JSON.stringify(controller)}),/require the standalone Cornhole build/);`;
 execFileSync(process.execPath,['--input-type=module','-e',code],{cwd:arcade,stdio:'pipe'});
});
