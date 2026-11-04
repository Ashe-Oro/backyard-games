import {OPPONENTS} from './opponents.mjs';

// Piecewise interpolation preserves the existing rookie → regular → ace → boss tuning.
const anchors=['rookie','regular','ace','boss'];
export const LADDER=[
 ['Sunny','Just here for the cookout.','☀','#f5bd63'],
 ['Sprout','Small bounces. Big dreams.','♧','#a7efb1'],
 ['Disco','Finds a rhythm at every table.','♫','#e9a1cc'],
 ['Comet','Always shooting for the stars.','★','#8fc6ee'],
 ['Shark','Smells a comeback coming.','▲','#6ed6ce'],
 ['Voltage','Quick hands. No hesitation.','ϟ','#c4a0f4'],
 ['The Glove','Every bounce has met its match.','✋','#ffe16b']
].map(([name,line,avatar,color],i)=>{
 const level=i/(7-1)*(anchors.length-1),lo=Math.floor(level),hi=Math.min(lo+1,3),t=level-lo;
 const profile={name};
 for(const group of ['shoot','swat'])profile[group]=Object.fromEntries(Object.keys(OPPONENTS[anchors[lo]][group]).map(key=>[key,OPPONENTS[anchors[lo]][group][key]*(1-t)+OPPONENTS[anchors[hi]][group][key]*t]));
 const id=`tournament-${i}`;OPPONENTS[id]=profile;
 return {id,name,line,avatar,color,...profile};
});
export const SAVE_KEY='tailgate-beer-pong-tournament-v1';
export function readProgress(storage){
 try{const value=JSON.parse(storage.getItem(SAVE_KEY));if(Number.isInteger(value?.unlocked)&&value.unlocked>=0&&value.unlocked<LADDER.length&&typeof value.completed==='boolean'&&(!value.completed||value.unlocked===LADDER.length-1))return value;}catch{}
 return {unlocked:0,completed:false};
}
