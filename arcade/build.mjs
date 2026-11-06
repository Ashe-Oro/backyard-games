import fs from 'node:fs/promises';
import path from 'node:path';
import {fileURLToPath} from 'node:url';
import {createWriteStream} from 'node:fs';
import {downloadArtifact} from '@electron/get';
import extract from 'extract-zip';
import yazl from 'yazl';
const here=path.dirname(fileURLToPath(import.meta.url)),repo=path.dirname(here),dist=path.join(here,'dist'),site=path.join(dist,'site');
const pkg=JSON.parse(await fs.readFile(path.join(here,'package.json')));
// Rebuild only this generated tree; never write into the canonical web sources.
await fs.rm(site,{recursive:true,force:true});
await fs.mkdir(site,{recursive:true});
for(const [folder,files] of Object.entries({'cornhole':['index.html','controls.mjs','wind-streaks.js'],'yard-games':['backyard.js','power-arrow.js','ui.css']})){
 await fs.mkdir(path.join(site,folder),{recursive:true});for(const file of files)await fs.copyFile(path.join(repo,folder,file),path.join(site,folder,file));
}
await fs.mkdir(path.join(site,'arcade'),{recursive:true});for(const file of ['controller.js','trackball.mjs','board-view.mjs','dog-celebration.mjs','arcade.css'])await fs.copyFile(path.join(here,file),path.join(site,'arcade',file));
await fs.mkdir(path.join(site,'vendor'),{recursive:true});
await fs.copyFile(path.join(here,'node_modules/three/build/three.module.min.js'),path.join(site,'vendor/three.module.min.js'));
await fs.copyFile(path.join(here,'node_modules/@fontsource/press-start-2p/files/press-start-2p-latin-400-normal.woff2'),path.join(site,'vendor/pixel.woff2'));
await fs.copyFile(path.join(here,'node_modules/three/LICENSE'),path.join(site,'vendor/THREE-LICENSE.txt'));
await fs.copyFile(path.join(here,'node_modules/@fontsource/press-start-2p/LICENSE'),path.join(site,'vendor/FONT-LICENSE.txt'));
let html=await fs.readFile(path.join(site,'cornhole/index.html'),'utf8');
html=html.replace(/<link[^>]+https:\/\/fonts\.[^>]+>\n?/g,'').replace('https://cdn.jsdelivr.net/npm/three@0.170.0/build/three.module.min.js','/vendor/three.module.min.js');
html=html.replace('<html lang="en">','<html lang="en" data-backyard-arcade>');
html=html.replace('</head>','<link rel="stylesheet" href="/arcade/arcade.css">\n</head>');
// Transform only the bundled copy. The website's input and physics are untouched.
const replace=(from,to)=>{if(!html.includes(from))throw new Error('Game integration anchor changed: '+from);html=html.replace(from,to);};
replace("isHuman(team) ? 0 : M.spin",'M.spin');
replace("arcNames = ['PUSH', 'NORMAL', 'KOBE']","arcNames = ['SLIDER', 'NORMAL', 'KOBE']");
// Cabinet playtest tuning; these replacements never touch the website source.
// Render at display resolution up to Full HD; keep nearest-filtered voxel textures.
replace('const scale = Math.max(1, Math.round(Math.min(h, w * 0.85) / 310));', 'const scale = Math.max(1, w / 1920, h / 1080);');
// Regulation distance stays 27 ft; a wider main lens gives the cabinet a distant view.
replace('camera.fov = 14 + 8 * portrait;', 'camera.fov = 18 + 6 * portrait;');
replace('const dog = {};', "const {DogCelebration}=await import('/arcade/dog-celebration.mjs');\nlet dogParty;\nconst dog = {};");
replace('scene.add(g); Object.assign(dog, { g, head, tail });','scene.add(g); Object.assign(dog, { g, head, tail }); dogParty=new DogCelebration(dog);');
replace("if (b.state === 'in') { say(b.swish", "if (b.state === 'in' || knockedIn.length) dogParty.celebrate();\n  if (b.state === 'in') { say(b.swish");
replace('dog.head.position.y = 0.55 + Math.abs(Math.sin(time * 1.5)) * 0.008;', 'dog.head.position.y = 0.55 + Math.abs(Math.sin(time * 1.5)) * 0.008; dogParty.update(dt);');
// Cornhole announcements: identify the starting player and name a made bag.
replace("say('COIN TOSS', TEAM[M.first] + ' THROWS FIRST', 1500);", "say(TEAM[M.first] + ' THROWS FIRST', '', 1500);");
replace("say(b.swish ? 'SWISH!' : 'CORNHOLE!', '3 POINTS')", "say('CORNHOLE!', '3 POINTS')");
// Bag rotation changes the landing, never creates a golf-like curved flight.
replace('b.curl = drift * 0.12 + spin * 0.10;', 'b.curl = 0;');
replace('b.v.x += (wind.x + b.curl * Math.sin(b.t * 5)) * dt;', 'b.v.x += wind.x * dt;');
replace("b.state = 'slide';\n        return 'land';", "b.state = 'slide';b.curl=clamp(b.english,-1,1)*.12;\n        return 'land';");
// Both live and paused frames render the same board close-up after the main view.
replace('const clock = new THREE.Clock();', "let boardView;\nfunction renderArcadeScene(){renderer.render(scene,camera);boardView?.render();}\nconst clock = new THREE.Clock();");
replace('if(M.paused){renderer.render(scene,camera);', 'if(M.paused){renderArcadeScene();');
replace('  renderer.render(scene, camera);', '  renderArcadeScene();');
const bridge=`
window.__arcadeHost={
 get state(){return window.__game.state;},
 round(){return lie();},
 get celebration(){return dogParty.state;},
 get boardZ(){return BOARD_Z;},
 start(mode){dogParty.reset();startMatch({practice:mode==='practice',vs:'2p',first:0});},
 pause(value){M.paused=value;cancelPull();},
 hideArrow(){cancelPull();},
 wind(enabled){if(!enabled)Object.assign(wind,{mph:0,angle:0,x:0,z:0});else if(!wind.mph)Object.assign(wind,roundWind({practice:false,previousX:wind.x}));renderHud();},
 position(dx){if(!humanTurn())return;M.position[thrower()]=clamp(M.position[thrower()]+dx,-.45,.45);positionHeldBag(baseYaw(releaseX(thrower())));},
 surface(){flipSide();},
 height(){if(!humanTurn())return;M.arc=(M.arc+1)%3;M.heights[thrower()]=M.arc;renderHud();},
 preview(shot){if(!humanTurn())return;M.aim=shot.aim;setMeter(shot.power);const origin=bagScreen(),yaw=baseYaw(releaseX(thrower()))+shot.aim,length=powerArrowLength(physicalPower(shot.power));showAim(origin,{x:origin.x+Math.sin(yaw)*length,y:origin.y-Math.cos(yaw)*length},false,physicalPower(shot.power));},
 throw(shot){if(!humanTurn())return false;M.aim=clamp(shot.aim,-AIM_MAX,AIM_MAX);M.spin=clamp(shot.spin,-1,1);return throwBag(clamp(shot.power,0,1));},
 advance(){if(['summary','over'].includes(M.phase))advance();},
};
await import('/arcade/controller.js');
const {BoardView}=await import('/arcade/board-view.mjs');
boardView=new BoardView(THREE,renderer,scene,BOARD_Z,document.getElementById('arcade-board-viewport'));
window.__arcadeHost.boardView=boardView;
`;
replace('</script>\n</body>',bridge+'</script>\n</body>');
html=html.replace('<title>Cornhole · Backyard Games</title>','<title>Backyard Cornhole Arcade · Trackball Prototype</title>');
await fs.writeFile(path.join(site,'cornhole/index.html'),html);
console.log('Offline site: '+site);
if(!process.argv.includes('--windows')&&!process.argv.includes('--mac'))process.exit(0);
const platform=process.argv.includes('--mac')?'darwin':'win32',arch=platform==='darwin'?process.arch:'x64';
const zip=await downloadArtifact({version:pkg.devDependencies.electron,artifactName:'electron',platform,arch,cacheRoot:path.join(here,'.cache')});
const target=path.join(dist,platform==='win32'?'BackyardCornhole.pc':'mac-smoke');await fs.rm(target,{recursive:true,force:true});await fs.mkdir(target,{recursive:true});await extract(zip,{dir:target});
const app=path.join(target,platform==='darwin'?'Electron.app/Contents/Resources/app':'resources/app');await fs.mkdir(app,{recursive:true});
await fs.writeFile(path.join(app,'package.json'),JSON.stringify({name:pkg.name,version:pkg.version,main:'main.cjs'}));await fs.copyFile(path.join(here,'main.cjs'),path.join(app,'main.cjs'));
for(const file of ['preload.cjs','cabinet-input.py'])await fs.copyFile(path.join(here,file),path.join(app,file));await fs.mkdir(path.join(app,'dist'),{recursive:true});await fs.cp(site,path.join(app,'dist/site'),{recursive:true});
if(platform==='win32'){
 await fs.rename(path.join(target,'electron.exe'),path.join(target,'BackyardCornhole.exe'));
 try{await fs.rename(path.join(target,'electron_wer.dll'),path.join(target,'BackyardCornhole_wer.dll'));}catch(error){if(error.code!=='ENOENT')throw error;}
 await fs.writeFile(path.join(target,'autorun.cmd'),'CMD=BackyardCornhole.exe\n');
 await fs.writeFile(path.join(target,'Software renderer.bat'),'@echo off\r\ncd /d "%~dp0"\r\nBackyardCornhole.exe --software\r\n');
 await fs.writeFile(path.join(target,'autorun-software.example.cmd'),'CMD=BackyardCornhole.exe --software\n');
 await fs.copyFile(path.join(here,'README.md'),path.join(target,'READ-ME.txt'));
 const output=path.join(dist,'BackyardCornhole-Trackball-v0.2.7-Windows-x64.zip'),archive=new yazl.ZipFile();
 async function add(directory){for(const entry of await fs.readdir(directory,{withFileTypes:true})){const file=path.join(directory,entry.name);if(entry.isDirectory())await add(file);else archive.addFile(file,path.posix.join('BackyardCornhole.pc',path.relative(target,file).split(path.sep).join('/')));}}
 await add(target);const stream=createWriteStream(output);archive.outputStream.pipe(stream);archive.end();await new Promise((resolve,reject)=>{stream.on('close',resolve);stream.on('error',reject);archive.outputStream.on('error',reject);});console.log('Windows ZIP: '+output);
}else console.log('Mac smoke executable: '+path.join(target,'Electron.app/Contents/MacOS/Electron'));
