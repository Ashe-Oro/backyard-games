// Screen-space motion keeps the swipe horizontal at every camera angle and viewport.
export const SWIPE_DURATION=.28;
export function handSwipe(age,impact,side){
 const f=Math.max(0,Math.min(1,age/SWIPE_DURATION));
 const edge=side*1.35;
 const t=f<.5?f*2:(1-f)*2;
 const ease=t*t*(3-2*t);
 return {x:edge+(impact.x-edge)*ease,y:impact.y,z:impact.z};
}
