import {test} from 'node:test';
import assert from 'node:assert/strict';
import {throwPower,pullPower,roundWind} from '../app/dist/site/cornhole/controls.mjs';

test('human power stays linear through the cabinet physical-power conversion',()=>{
 for(const value of [0,.1,.25,.5,.75,1]){
  assert.equal(throwPower(value),value);
  assert.equal(pullPower(value),value);
 }
 assert.equal(throwPower(-1),0);assert.equal(throwPower(2),1);
 assert.equal(pullPower(-1),0);assert.equal(pullPower(2),1);
 assert.deepEqual(roundWind({practice:true}),{mph:0,angle:0,x:0,z:0});
});
