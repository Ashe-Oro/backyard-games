import * as THREE from 'three';

export const POWER_ARROW_MAX=145;
const colors=['#8fc6ee','#a7efb1','#ffe16b','#f6a35c','#f1755f'].map(c=>new THREE.Color(c));
const clamp=value=>Math.max(0,Math.min(1,value)),elements=new WeakMap();
export const powerArrowLength=power=>Math.max(16,clamp(power)*POWER_ARROW_MAX);
export function shotPowerColor(power){
 const position=clamp(power)*(colors.length-1),index=Math.min(colors.length-2,Math.floor(position));
 return '#'+colors[index].clone().lerp(colors[index+1],position-index).getHexString();
}
export function paintPowerArrow(svg,power,from,to,rect){
 if(!elements.has(svg))elements.set(svg,Object.fromEntries(['shaft','tip','grip','track','limit'].map(role=>[role,svg.querySelector(`[data-power-${role}]`)])));
 const parts=elements.get(svg),color=shotPowerColor(power);
 parts.shaft.setAttribute('stroke',color);parts.tip.setAttribute('fill',color);parts.grip.setAttribute('fill',color);
 const dx=to.x-from.x,dy=to.y-from.y,length=Math.max(1,Math.hypot(dx,dy));
 const end={x:from.x+dx/length*POWER_ARROW_MAX,y:from.y+dy/length*POWER_ARROW_MAX};
 for(const [key,value]of Object.entries({x1:from.x-rect.left,y1:from.y-rect.top,x2:end.x-rect.left,y2:end.y-rect.top}))parts.track.setAttribute(key,value);
 parts.limit.setAttribute('cx',end.x-rect.left);parts.limit.setAttribute('cy',end.y-rect.top);
}
