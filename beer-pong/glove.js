import * as THREE from 'three';
// A compact floating white glove, built from shaded low-poly segments in the game scene.
export function createGlove(){
 const root=new THREE.Group(),white=new THREE.MeshPhongMaterial({color:'#fff9ef',shininess:14}),seam=new THREE.MeshLambertMaterial({color:'#a3a5a2'}),inside=new THREE.MeshLambertMaterial({color:'#5b6261'});
 const block=(w,h,d,x,y,z,m=white)=>{const mesh=new THREE.Mesh(new THREE.BoxGeometry(w,h,d),m);mesh.position.set(x,y,z);mesh.castShadow=mesh.receiveShadow=true;root.add(mesh);return mesh;};
 block(.24,.25,.075,0,0,0);const pad=new THREE.Mesh(new THREE.SphereGeometry(.13,12,8),white);pad.scale.set(1,1,.40);pad.position.z=.025;pad.castShadow=true;root.add(pad);
 const fingers=[];
 for(let i=0;i<4;i++){
  const finger=new THREE.Group();finger.position.set(-.11,.09-i*.06,0);finger.rotation.z=(1.5-i)*-.075;root.add(finger);fingers.push(finger);
  for(let j=0;j<3;j++){const length=[.075,.095,.10,.08][i],piece=new THREE.Mesh(new THREE.CylinderGeometry(.028-j*.002,.030-j*.002,length,8),white);piece.rotation.z=Math.PI/2;piece.position.x=-length*(j+.5);piece.castShadow=true;finger.add(piece);
   if(j<2){const crease=new THREE.Mesh(new THREE.TorusGeometry(.027-j*.002,.002,3,8),seam);crease.rotation.y=Math.PI/2;crease.position.x=-length*(j+1);finger.add(crease);}
  }
  const tip=new THREE.Mesh(new THREE.SphereGeometry(.024,8,6),white);tip.position.x=-[.075,.095,.10,.08][i]*3;tip.castShadow=true;finger.add(tip);
 }
 const thumb=new THREE.Group();thumb.position.set(.045,-.12,.015);thumb.rotation.z=.55;root.add(thumb);for(let i=0;i<2;i++){const m=new THREE.Mesh(new THREE.CylinderGeometry(.035,.04,.09,8),white);m.rotation.z=.2;m.position.set(-.03-i*.02,-.045-i*.08,0);m.castShadow=true;thumb.add(m);}
 for(const [x,y]of [[-.03,-.005],[-.045,-.085],[-.065,-.165]]){const joint=new THREE.Mesh(new THREE.SphereGeometry(.038,8,6),white);joint.position.set(x,y,0);joint.castShadow=true;thumb.add(joint);}
 block(.09,.29,.11,.155,0,0);block(.015,.22,.075,.205,0,0,inside);for(let i=0;i<3;i++)block(.075,.007,.003,-.02,.045-i*.045,.079,seam);
 // Visual size only: both defenders retain their existing contact rules.
 root.scale.setScalar(.55);
 root.visible=false;root.userData.fingers=fingers;return root;
}
