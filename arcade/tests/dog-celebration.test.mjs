import {test} from 'node:test';
import assert from 'node:assert/strict';
import {DogCelebration} from '../dog-celebration.mjs';
const dog=()=>({g:{position:{x:2.3,y:0,z:-8.7},rotation:{x:0,y:2.6,z:0},scale:{x:.85,y:.85,z:.85}},head:{position:{x:0,y:.55,z:0},rotation:{x:0,y:0,z:0}},tail:{rotation:{y:0}}});
test('hole celebrations alternate between hops, a spin and a bow, returning to idle pose',()=>{
 const d=dog(),party=new DogCelebration(d),base=structuredClone(d.g),poses=[];
 for(const kind of ['hops','spin','bow']){
  party.celebrate();assert.equal(party.state.kind,kind);party.update(.55);poses.push(structuredClone(d.g));assert(party.state.active);assert.notDeepEqual(d.g,base);
  party.update(2);assert(!party.state.active);assert.deepEqual(d.g,base);assert.equal(d.head.position.y,.55);
 }
 assert.notDeepEqual(poses[0],poses[1]);assert.notDeepEqual(poses[1],poses[2]);assert.equal(party.state.count,3);
 party.celebrate();assert.equal(party.state.kind,'hops');party.update(.4);party.reset();assert.deepEqual(d.g,base);
});
test('retrigger and zero elapsed time never accumulate transforms or drift',()=>{
 const d=dog(),party=new DogCelebration(d),base=structuredClone(d.g);party.celebrate();party.update(.7);const before=structuredClone(d.g);party.update(0);assert.deepEqual(d.g,before);party.celebrate();party.update(2.2);assert.deepEqual(d.g,base);
});
