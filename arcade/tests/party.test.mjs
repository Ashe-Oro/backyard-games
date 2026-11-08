import {test} from 'node:test';
import assert from 'node:assert/strict';
import * as THREE from 'three';
import {createParty} from '../party.mjs';

test('chess players move complete pieces on alternating turns',()=>{
 const p=createParty(new THREE.Scene()),white=p.pieces.find(x=>x.side===1&&x.col===4&&x.rank===1),black=p.pieces.find(x=>x.side===-1&&x.col===4&&x.rank===1);
 p.update(2);assert.equal(p.chessTurn,0);assert(white.g.position.z<white.start.z);assert(white.g.position.y>white.start.y);assert.equal(black.g.position.z,black.start.z);
 p.update(6);assert.equal(p.chessTurn,1);assert(black.g.position.z>black.start.z);assert.equal(white.g.position.y,white.start.y);
});
test('keg stand faces the keg with tap at the mouth, guests stay outside play',()=>{
 const p=createParty(new THREE.Scene());for(let i=0;i<100;i++){p.update(.5);for(const person of [p.guest,p.friend,p.cook])assert(person.g.position.x>2.5);}
 const mouth=p.guest.g.localToWorld(new THREE.Vector3(0,.97,.121)),tap=p.keg.localToWorld(p.tap.position.clone());assert(mouth.distanceTo(tap)<.04);assert.equal(p.guest.g.rotation.x,Math.PI);assert(p.keg.position.x>p.cook.g.position.x);
});
test('cook flips food and tosses a burger that reaches the dog',()=>{
 const p=createParty(new THREE.Scene()),dog={g:new THREE.Group(),head:new THREE.Group(),tail:new THREE.Group()};dog.g.position.set(2.3,0,-8.7);dog.head.position.set(0,.55,-.15);dog.g.add(dog.head,dog.tail);
 p.update(.5,dog);assert(p.foods.some(f=>f.g.position.y>f.base.y));p.update(25.5,dog);assert(p.snack.visible);p.update(.5,dog);assert(!p.snack.visible);assert.equal(p.catches,1);
});
