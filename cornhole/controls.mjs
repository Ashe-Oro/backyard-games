// Spread the useful throwing range over fewer pull pixels without losing either endpoint.
const GAIN=4,EDGE=Math.tanh(GAIN/2),clamp=value=>Math.max(0,Math.min(1,value));
export function throwPower(pull){return (Math.tanh((clamp(pull)-.5)*GAIN)/EDGE+1)/2;}
export function pullPower(power){return Math.atanh((clamp(power)*2-1)*EDGE)/GAIN+.5;}

// A round has one shared, readable crosswind; later rounds switch sides.
export function roundWind({practice=false,circuit=null,previousX=0}={},random=Math.random){
 if(practice)return {mph:0,angle:0,x:0,z:0};
 const [minimum,steps]=[[4,3],[6,4],[9,4],[12,4]][circuit]??[6,5];
 const side=previousX? -Math.sign(previousX):(random()<.5?-1:1);
 const angle=side*Math.PI/2+(random()-.5)*Math.PI/3,mph=minimum+Math.floor(random()*steps);
 return {mph,angle,x:Math.sin(angle)*mph*.025,z:-Math.cos(angle)*mph*.025};
}
