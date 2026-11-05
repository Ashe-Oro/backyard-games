import * as THREE from 'three';

// Decorative gusts use a separate seed and one draw call; they never touch bag physics.
export function createWindStreaks(scene){
 const count=24,positions=new Float32Array(count*4*3),geometry=new THREE.BufferGeometry();
 geometry.setAttribute('position',new THREE.BufferAttribute(positions,3).setUsage(THREE.DynamicDrawUsage));
 const material=new THREE.LineBasicMaterial({color:'#e5efdc',transparent:true,opacity:0,depthWrite:false});
 const mesh=new THREE.LineSegments(geometry,material);mesh.name='wind-streaks';mesh.frustumCulled=false;mesh.visible=false;scene.add(mesh);
 const reducedMotion=matchMedia('(prefers-reduced-motion: reduce)');
 let seed=57413,seconds=0;
 const random=()=>((seed=(seed*1664525+1013904223)>>>0)/4294967296);
 const streaks=Array.from({length:count},()=>({x:(random()-.5)*5.6,y:.25+random()*2.2,z:-2-random()*10,phase:random(),size:.75+random()*.5}));
 function update(dt,wind,enabled=true){
  const magnitude=Math.hypot(wind.x,wind.z);
  mesh.visible=enabled&&wind.mph>0&&magnitude>0&&!reducedMotion.matches;
  if(!mesh.visible)return;
  seconds+=dt;
  const strength=THREE.MathUtils.clamp(wind.mph/15,0,1),active=Math.round(4+strength*20),lifetime=3.6-strength*1.6;
  const dx=wind.x/magnitude,dz=wind.z/magnitude,travel=(.35+wind.mph*.12)*lifetime;
  material.opacity=.035+strength*.095;geometry.setDrawRange(0,active*4);
  for(let i=0;i<active;i++){
   const s=streaks[i],life=(seconds/lifetime+s.phase)%1,envelope=Math.sin(life*Math.PI);
   const length=(.22+strength*.38)*s.size*envelope;
   const x=s.x+dx*(life-.5)*travel,z=s.z+dz*(life-.5)*travel,y=s.y+Math.sin(life*Math.PI*2+s.phase*6)*.035;
   for(const [vertex,along]of [0,.65,.85,1.1].entries()){
    const offset=(i*4+vertex)*3;positions[offset]=x-dx*length*along;positions[offset+1]=y;positions[offset+2]=z-dz*length*along;
   }
  }
  geometry.attributes.position.needsUpdate=true;
 }
 return {mesh,update};
}
