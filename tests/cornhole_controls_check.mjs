import assert from 'node:assert/strict';
import {throwPower,pullPower,powerFromPull,FULL_POWER_PULL,roundWind} from '../cornhole/controls.mjs';
let last=-1;for(let i=0;i<=1000;i++){const input=i/1000,power=throwPower(input);assert(power>last);assert(Math.abs(pullPower(power)-input)<1e-10);last=power;}
assert.equal(throwPower(-1),0);assert.equal(throwPower(2),1);assert.equal(throwPower(.5),.5);assert.equal(FULL_POWER_PULL,400);for(const [distance,power] of [[0,0],[100,.25],[200,.5],[400,1],[800,1]])assert.equal(powerFromPull(distance),power);assert.equal(pullPower(-1),0);assert.equal(pullPower(2),1);
let seed=19381;const random=()=>((seed=(seed*1664525+1013904223)>>>0)/4294967296);
for(const circuit of [null,0,1,2,3]){let previousX=0;for(let round=0;round<60;round++){const w=roundWind({circuit,previousX},random);const [lo,hi]=circuit===null?[6,10]:[[4,6],[6,9],[9,12],[12,15]][circuit];assert(w.mph>=lo&&w.mph<=hi);assert(Math.abs(w.x)>=w.mph*.025*.86);if(previousX)assert.equal(Math.sign(w.x),-Math.sign(previousX));previousX=w.x;}}
assert.deepEqual(roundWind({practice:true,circuit:3,previousX:1},()=>{throw Error('Calm practice consumes no wind randomness')}),{mph:0,angle:0,x:0,z:0});
console.log('Power is continuous, monotonic and invertible; full range preserved; circuit winds bounded, crosswise and alternate by round; practice stays calm.');
