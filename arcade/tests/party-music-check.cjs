const {_electron}=require('playwright-core'),assert=require('node:assert/strict'),fs=require('node:fs'),path=require('node:path');
(async()=>{const profile=fs.mkdtempSync('/tmp/cornhole-party-audio-'),songs=path.join(profile,'songs');fs.mkdirSync(songs);
 // Original test tone WAVs, only in temporary test data, never packaged as music.
 for(const n of [1,2]){const len=22050*2*10,b=Buffer.alloc(44+len);b.write('RIFF');b.writeUInt32LE(36+len,4);b.write('WAVEfmt ',8);b.writeUInt32LE(16,16);b.writeUInt16LE(1,20);b.writeUInt16LE(1,22);b.writeUInt32LE(22050,24);b.writeUInt32LE(44100,28);b.writeUInt16LE(2,32);b.writeUInt16LE(16,34);b.write('data',36);b.writeUInt32LE(len,40);for(let i=0;i<len/2;i++)b.writeInt16LE(Math.round(Math.sin(i/22050*Math.PI*2*220)*200),44+i*2);fs.writeFileSync(path.join(songs,`${n} Test.wav`),b);}
 fs.writeFileSync(path.join(profile,'music-folder.json'),JSON.stringify({folder:songs}));
 const app=await _electron.launch({executablePath:process.env.ELECTRON_PATH,args:['--windowed'],env:{...process.env,ARCADE_TEST_DATA:profile}});try{
 const p=await app.firstWindow(),errors=[];p.on('pageerror',e=>errors.push(e.message));await p.waitForFunction(()=>window.__arcade?.music.tracks.length===2);await p.setViewportSize({width:1920,height:1080});
 await p.evaluate(()=>__arcade.start('two',false));await p.waitForFunction(()=>__game.state.phase==='aim');await p.waitForTimeout(1700);
 const first=await p.evaluate(()=>{const h=__arcadeHost.party;return {time:h.time,x:h.guest.g.position.x,people:h.pongPlayers.length,keg:h.keg.name}});assert.equal(first.people,2);assert.equal(first.keg,'keg');await p.waitForTimeout(700);assert(await p.evaluate(x=>__arcadeHost.party.time>x,first.time));
 // Keep all decorative actors and their movement left of the playing corridor.
 await p.evaluate(()=>{const h=__arcadeHost.party;for(let i=0;i<100;i++){h.update(.25);if(h.guest.g.position.x<2.5)throw Error('Guest entered throwing lane');}});
 await p.screenshot({path:'/Users/ashe/Documents/Codex/2026-11-03/pleas/outputs/cornhole-party-pump.png'});
 await p.evaluate(()=>{const h=__arcadeHost.party;h.update((6-h.time%20+20)%20);});assert(await p.evaluate(()=>__arcadeHost.party.stream.visible));await p.screenshot({path:'/Users/ashe/Documents/Codex/2026-11-03/pleas/outputs/cornhole-pump-pouring.png'});
 await p.evaluate(()=>{const h=__arcadeHost.party;h.update(50-h.time%65);});assert(await p.evaluate(()=>__arcadeHost.party.kegStand));assert((await p.evaluate(()=>__arcadeHost.party.guest.g.rotation.x))>3);await p.screenshot({path:'/Users/ashe/Documents/Codex/2026-11-03/pleas/outputs/cornhole-keg-stand-pump.png'});
 await p.evaluate(()=>__arcade.showMenu());const stopped=await p.evaluate(()=>__arcadeHost.party.time);await p.waitForTimeout(300);assert.equal(await p.evaluate(()=>__arcadeHost.party.time),stopped);
 await p.locator('#arcade-audio-open').click();assert(await p.locator('#arcade-audio').isVisible());await p.locator('#music-toggle').click();await p.waitForFunction(()=>!__arcade.music.audio.paused);assert.equal(await p.evaluate(()=>__arcade.music.audio.error),null);
 await p.locator('#music-next').click();await p.waitForFunction(()=>__arcade.music.audio.src.endsWith('/1'));await p.locator('[data-music-volume=".1"]').click();assert.equal(await p.evaluate(()=>__arcade.music.audio.volume),.35);
 await p.screenshot({path:'/Users/ashe/Documents/Codex/2026-11-03/pleas/outputs/cornhole-music.png'});
 await p.locator('#music-toggle').click();assert(await p.evaluate(()=>__arcade.music.audio.paused));await p.reload();await p.waitForFunction(()=>window.__arcade?.music.tracks.length===2);assert.equal(await p.evaluate(()=>__arcade.music.audio.volume),.35);assert.equal(await p.evaluate(()=>__arcade.music.enabled),false);assert.deepEqual(errors,[]);
 console.log('Party animation, pause freeze and clear throwing lane; offline audio decoding/play/pause/next, folder scan and volume persistence passed.');
 }finally{await app.close();fs.rmSync(profile,{recursive:true,force:true});}})().catch(e=>{console.error(e);process.exitCode=1});
