import assert from 'node:assert/strict';
import {TournamentProgress,RIVALS,CIRCUITS,TOURNAMENT_KEY,portraitSvg} from '../beer-pong/tournament.mjs';
const memory=()=>{const data=new Map();return {getItem:k=>data.get(k),setItem:(k,v)=>data.set(k,v)}};
assert.equal(RIVALS.length,8);assert.equal(CIRCUITS.length,4);assert.equal(new Set(RIVALS.map(r=>r.name)).size,8);assert.equal(new Set(RIVALS.map(portraitSvg)).size,8);
for(const [i,r] of RIVALS.entries()){assert.equal(r.circuit,CIRCUITS[Math.floor(i/2)]);assert(r.shoot.bounce>=0&&r.shoot.bounce<=1);if(i){assert(r.shoot.precise>RIVALS[i-1].shoot.precise);assert(r.shoot.jitter<RIVALS[i-1].shoot.jitter);}}
const store=memory(),progress=new TournamentProgress(store);assert.equal(progress.cleared,0);assert.equal(progress.recordWin(1),false);
for(let i=0;i<8;i++){assert(progress.recordWin(i));assert.equal(progress.recordWin(i),false);assert.equal(new TournamentProgress(store).cleared,i+1);}assert(progress.complete);assert.equal(progress.recordWin(8),false);progress.reset();assert.equal(new TournamentProgress(store).cleared,0);
for(const raw of ['{','null','{}',JSON.stringify({version:2,cleared:3}),...[-1,2.5,9,'3'].map(cleared=>JSON.stringify({version:1,cleared}))]){store.setItem(TOURNAMENT_KEY,raw);assert.equal(new TournamentProgress(store).cleared,0);}
for(const storage of [undefined,{getItem(){throw Error('blocked')},setItem(){throw Error('blocked')}}]){const p=new TournamentProgress(storage);assert.equal(p.canSave,false);assert(p.recordWin(0));assert.equal(p.cleared,1);p.reset();assert.equal(p.cleared,0);}
const shared=memory(),old=new TournamentProgress(shared),newer=new TournamentProgress(shared);newer.recordWin(0);newer.recordWin(1);old.recordWin(0);assert.equal(new TournamentProgress(shared).cleared,2);
console.log('Eight distinct escalating rivals; sequential wins, persistence, resets, corruption, blocked storage and older-tab saves passed.');
