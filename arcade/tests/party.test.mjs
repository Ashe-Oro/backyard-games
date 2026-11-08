import {test} from 'node:test';
import assert from 'node:assert/strict';
import * as THREE from 'three';
import {createParty} from '../party.mjs';

test('beer pong alternates full-length throws, sinks ten cups, celebrates and resets',()=>{
 const p=createParty(new THREE.Scene());
 assert.equal(p.pongTable.rotation.y,Math.PI/2);
 assert(p.pongPlayers[0].g.position.x<-1&&p.pongPlayers[1].g.position.x>1);
 assert.deepEqual(p.pongState.remaining,[10,10]);
 p.update(1);
 assert(p.pongBall.visible&&p.pongBall.position.y>1.1);
 p.update(.75);
 assert.deepEqual(p.pongState.remaining,[10,9]);
 p.update(2.45);
 assert.deepEqual(p.pongState.remaining,[9,9]);
 p.update(4.9);
 assert.deepEqual(p.pongState.remaining,[9,8],'a miss leaves its target rack intact');
 p.update(p.pongLoopSeconds-p.time-.8);
 assert.equal(p.pongState.winner,0);
 assert.deepEqual(p.pongState.remaining,[3,0]);
 assert(p.pongPlayers[0].arm.rotation.x<-2);
 p.update(1);
 assert.deepEqual(p.pongState.remaining,[10,10]);
 assert.equal(p.pongState.winner,null);
});
test('keg stand faces the keg with tap at the mouth, guests stay outside play',()=>{
 const p=createParty(new THREE.Scene());for(let i=0;i<100;i++){p.update(.5);for(const person of [p.guest,p.friend,p.cook])assert(person.g.position.x>2.5);}
 const mouth=p.guest.g.localToWorld(new THREE.Vector3(0,.97,.121)),tap=p.keg.localToWorld(p.tap.position.clone());assert(mouth.distanceTo(tap)<.04);assert.equal(p.guest.g.rotation.x,Math.PI);assert(p.keg.position.x>p.cook.g.position.x);
});
test('cook flips food and tosses a burger that reaches the dog',()=>{
 const p=createParty(new THREE.Scene()),dog={g:new THREE.Group(),head:new THREE.Group(),tail:new THREE.Group()};dog.g.position.set(2.3,0,-8.7);dog.head.position.set(0,.55,-.15);dog.g.add(dog.head,dog.tail);
 p.update(.5,dog);assert(p.foods.some(f=>f.g.position.y>f.base.y));p.update(25.5,dog);assert(p.snack.visible);p.update(.5,dog);assert(!p.snack.visible);assert.equal(p.catches,1);
});
