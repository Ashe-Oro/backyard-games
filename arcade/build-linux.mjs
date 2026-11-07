// Native Batocera companion: reuse the exact offline game without Wine.
import fs from 'node:fs/promises';
import path from 'node:path';
import {fileURLToPath} from 'node:url';
import {execFileSync} from 'node:child_process';
import {createHash} from 'node:crypto';
import {createWriteStream} from 'node:fs';
import {downloadArtifact} from '@electron/get';
import extract from 'extract-zip';
import yazl from 'yazl';
const root=path.dirname(fileURLToPath(import.meta.url));
execFileSync(process.execPath,['build.mjs','--site'],{cwd:root,stdio:'inherit'});
const pkg=JSON.parse(await fs.readFile(path.join(root,'package.json')));
const dist=path.join(root,'dist'),target=path.join(dist,'BackyardCornhole-linux'),cache=path.join(root,'.cache');
await fs.mkdir(cache,{recursive:true});
const runtime=await downloadArtifact({version:pkg.devDependencies.electron,artifactName:'electron',platform:'linux',arch:'x64',cacheRoot:cache});
await fs.rm(target,{recursive:true,force:true});await fs.mkdir(target,{recursive:true});await extract(runtime,{dir:target});
const app=path.join(target,'resources/app');await fs.mkdir(path.join(app,'dist'),{recursive:true});
await fs.copyFile(path.join(root,'main.cjs'),path.join(app,'main.cjs'));
for(const file of ['preload.cjs','cabinet-input.py','music-library.cjs','release-exit-keys.py'])await fs.copyFile(path.join(root,file),path.join(app,file));
await fs.writeFile(path.join(app,'package.json'),JSON.stringify({name:pkg.name,version:pkg.version,main:'main.cjs'}));
await fs.cp(path.join(dist,'site'),path.join(app,'dist/site'),{recursive:true});
// Batocera 42 has GTK/NSS/OpenGL, but does not ship this Electron-linked library.
// Bundle it privately, including its license; do not alter system libraries.
const cupsName='libcups2_2.4.2-3+deb12u9_amd64.deb';
const expected='8dbfd3b7cd81166a2d79bb7e3898b0203fec05c7e927f60e207122a1e36b95c8';
let cups;try{cups=await fs.readFile(path.join(cache,cupsName));}catch{}
if(!cups||createHash('sha256').update(cups).digest('hex')!==expected){const response=await fetch('https://deb.debian.org/debian/pool/main/c/cups/'+cupsName);if(!response.ok)throw Error('CUPS download failed: '+response.status);cups=Buffer.from(await response.arrayBuffer());if(createHash('sha256').update(cups).digest('hex')!==expected)throw Error('CUPS checksum mismatch');await fs.writeFile(path.join(cache,cupsName),cups);}
if(cups.subarray(0,8).toString()!=='!<arch>\n')throw Error('Invalid Debian archive');
const temporary=await fs.mkdtemp(path.join(cache,'cups-'));
try{
 let payload;for(let offset=8;offset+60<=cups.length;){const header=cups.subarray(offset,offset+60).toString(),name=header.slice(0,16).trim(),size=Number(header.slice(48,58));if(!Number.isFinite(size)||size<0||offset+60+size>cups.length)throw Error('Invalid Debian member');if(name.startsWith('data.tar.'))payload=cups.subarray(offset+60,offset+60+size);offset+=60+size+(size%2);}
 if(!payload)throw Error('Missing Debian payload');const tar=path.join(temporary,'data.tar.xz');await fs.writeFile(tar,payload);
 execFileSync('tar',['-xf',tar,'-C',temporary,'./usr/lib/x86_64-linux-gnu/libcups.so.2','./usr/share/doc/libcups2/copyright']);
 await fs.copyFile(path.join(temporary,'usr/lib/x86_64-linux-gnu/libcups.so.2'),path.join(target,'libcups.so.2'));
 await fs.copyFile(path.join(temporary,'usr/share/doc/libcups2/copyright'),path.join(target,'libcups2-COPYRIGHT.txt'));
}finally{await fs.rm(temporary,{recursive:true,force:true});}
const run=`#!/bin/sh
ARCADE_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd) || exit 1
export DISPLAY="\${DISPLAY:-$(getLocalXDisplay)}"
export LD_LIBRARY_PATH="$ARCADE_DIR\${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
cd "$ARCADE_DIR" || exit 1
exec ./electron --no-sandbox --ozone-platform=x11 --disable-dev-shm-usage "$@"
`;
const port=await fs.readFile(path.join(root,'batocera-port.sh'),'utf8');
await fs.writeFile(path.join(target,'run-cornhole.sh'),run,{mode:0o755});await fs.chmod(path.join(target,'electron'),0o755);
await fs.copyFile(path.join(root,'README.md'),path.join(target,'READ-ME.txt'));await fs.copyFile(path.join(root,'AUDIO.md'),path.join(target,'AUDIO.md'));
const output=path.join(dist,'BackyardCornhole-Trackball-v'+pkg.version+'-Linux-x64.zip'),archive=new yazl.ZipFile();
async function add(directory){for(const entry of await fs.readdir(directory,{withFileTypes:true})){const file=path.join(directory,entry.name);if(entry.isDirectory())await add(file);else archive.addFile(file,'BackyardCornhole-linux/'+path.relative(target,file));}}
await add(target);archive.addBuffer(Buffer.from(port),'BackyardCornhole.sh',{mode:0o100755});
archive.addBuffer(await fs.readFile(path.join(root,'batocera-hotkey-release.sh')),'Batocera-Support/90-release-virtual-hotkeys.sh',{mode:0o100755});
for(const file of await fs.readdir(path.join(root,'media')))archive.addFile(path.join(root,'media',file),'Batocera-Media/'+file);
const stream=createWriteStream(output);archive.outputStream.pipe(stream);archive.end();await new Promise((resolve,reject)=>{stream.on('close',resolve);stream.on('error',reject);archive.outputStream.on('error',reject);});
console.log('Native Linux ZIP: '+output);
