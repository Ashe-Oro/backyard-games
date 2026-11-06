import * as THREE from 'three';
// Cabinet decoration only. Never registers objects with bag collision/scoring.
export function createParty(scene){
 const root=new THREE.Group();root.name='arcade-hangout';scene.add(root);
 const geo=new THREE.BoxGeometry(1,1,1),materials=new Map();
 const mat=c=>{if(!materials.has(c))materials.set(c,new THREE.MeshLambertMaterial({color:c}));return materials.get(c);};
 function cube(w,h,d,c,x,y,z,parent=root){const m=new THREE.Mesh(geo,mat(c));m.scale.set(w,h,d);m.position.set(x,y,z);m.castShadow=m.receiveShadow=true;parent.add(m);return m;}
 const keg=new THREE.Group();keg.name='keg';keg.position.set(-1.7,0,-6.7);root.add(keg);
 cube(.68,.13,.62,'#7a5338',0,.13,0,keg);for(const x of [-.23,.23])cube(.09,.24,.48,'#624730',x,.12,0,keg);
 const shell=new THREE.Mesh(new THREE.CylinderGeometry(.29,.29,.65,8),mat('#aebcb5'));shell.position.y=.585;shell.castShadow=shell.receiveShadow=true;keg.add(shell);
 for(const y of [.35,.83]){const band=new THREE.Mesh(new THREE.CylinderGeometry(.31,.31,.07,8),mat('#647b78'));band.position.y=y;keg.add(band);}
 // Party-pump reference: top coupler, upright pump and black push handle.
 const rim=new THREE.Mesh(new THREE.TorusGeometry(.255,.025,4,8),mat('#d8decb'));rim.rotation.x=Math.PI/2;rim.position.y=.93;keg.add(rim);
 cube(.16,.065,.15,'#647b78',0,.965,0,keg);
 const pump=new THREE.Mesh(new THREE.CylinderGeometry(.043,.043,.23,6),mat('#d8decb'));pump.position.y=1.11;keg.add(pump);pump.name='top-party-pump';
 const plunger=new THREE.Group();plunger.position.y=1.27;keg.add(plunger);
 cube(.024,.15,.024,'#b7c8c2',0,-.045,0,plunger);cube(.18,.045,.075,'#263c39',0,.035,0,plunger);
 cube(.13,.04,.06,'#647b78',.105,.98,.015,keg);
 // A flexible loop leads from the top coupler to a separate handheld picnic tap.
 const tap=new THREE.Group();tap.name='hose-picnic-tap';tap.position.set(0,.79,.5);keg.add(tap);
 cube(.06,.055,.14,'#263c39',0,0,0,tap);cube(.035,.07,.035,'#263c39',0,-.04,.08,tap);
 const squeeze=cube(.025,.08,.065,'#445750',0,.06,-.015,tap);squeeze.rotation.x=-.35;
 const hose=new THREE.InstancedMesh(new THREE.CylinderGeometry(.014,.014,1,5),mat('#374d48'),18);hose.name='flexible-beer-hose';hose.frustumCulled=false;keg.add(hose);
 const hosePose=new THREE.Object3D(),up=new THREE.Vector3(0,1,0),delta=new THREE.Vector3();
 function shapeHose(){
  const curve=new THREE.CatmullRomCurve3([new THREE.Vector3(.12,.98,.015),new THREE.Vector3(.37,1.02,.13),new THREE.Vector3(.43,.55,.34),new THREE.Vector3(.18,.48,.52),tap.position.clone().add(new THREE.Vector3(0,0,-.07))]);
  for(let i=0;i<18;i++){const a=curve.getPoint(i/18),b=curve.getPoint((i+1)/18);delta.subVectors(b,a);hosePose.position.copy(a).add(b).multiplyScalar(.5);hosePose.scale.set(1,delta.length()+.003,1);hosePose.quaternion.setFromUnitVectors(up,delta.normalize());hosePose.updateMatrix();hose.setMatrixAt(i,hosePose.matrix);}
  hose.instanceMatrix.needsUpdate=true;
 }
 const stream=cube(.018,.13,.018,'#e9be59',0,.655,.58,keg);stream.visible=false;shapeHose();
 function person(name,shirt,skin,hair,x,z,seated=false,angle=0){
  const g=new THREE.Group();g.name=name;g.position.set(x,0,z);g.rotation.y=angle;root.add(g);
  const offset=seated?-.1:0;
  cube(.27,.34,.17,shirt,0,.66+offset,0,g);cube(.24,.23,.23,skin,0,.97+offset,0,g);cube(.25,.075,.24,hair,0,1.095+offset,0,g);
  for(const dx of [-.062,.062]){cube(.028,.026,.012,'#26352d',dx,.985+offset,.121,g);cube(.025,.025,.016,'#dc996e',dx,.94+offset,.121,g);}
  for(const dx of [-.077,.077]){cube(.09,seated?.28:.34,.1,'#34566a',dx,seated?.21:.32,seated?.2:0,g);if(seated)cube(.09,.08,.27,'#34566a',dx,.4,.09,g);cube(.12,.065,.18,'#2b3e39',dx,seated?.065:.12,seated?.26:.035,g);}
  const arm=new THREE.Group();arm.position.set(.175,.79+offset,0);g.add(arm);cube(.085,.17,.09,shirt,0,-.075,0,arm);cube(.075,.15,.075,skin,0,-.23,0,arm);
  cube(.085,.29,.09,shirt,-.175,.65+offset,0,g);cube(.075,.06,.075,skin,-.175,.48+offset,0,g);
  const cup=cube(.09,.11,.09,'#d15a49',0,-.29,.035,arm);const rim=cube(.095,.018,.095,'#fff1d1',0,-.23,.035,arm);
  return {g,arm,cup,rim};
 }
 const guest=person('cup-filling-guest','#d7a349','#bd835c','#49392c',-2.05,-5.9);
 const friend=person('chatting-guest','#72a0a0','#e3b085','#523c30',-3.65,-6.65,false,.9);
 const a=person('chess-white','#af6f66','#e8be96','#72503c',-3.0,-7.9+.6,true,Math.PI);
 const b=person('chess-black','#637b9b','#95674e','#303c35',-2.3,-7.9-.6,true,0);
 // Replace the runner with a real 8x8 board, oriented across the picnic table.
 cube(.64,.025,.64,'#6e4c33',-2.66,.713,-7.9);
 for(let row=0;row<8;row++)for(let col=0;col<8;col++)cube(.075,.012,.075,(row+col)%2?'#49604a':'#e5d9ac',-2.66+(col-3.5)*.075,.734,-7.9+(row-3.5)*.075);
 const pawns=[];
 for(const [side,color] of [[1,'#fff0c9'],[-1,'#2c3b37']])for(let col=0;col<8;col++){
  const x=-2.66+(col-3.5)*.075,z=-7.9+side*.1875;
  const pawn=cube(.034,.055,.034,color,x,.767,z);cube(.046,.014,.046,color,x,.746,z);if(col===4)pawns.push({m:pawn,z,side});
  const height=[.065,.075,.085,.11,.125,.085,.075,.065][col];cube(.042,height,.042,color,x,.746+height/2,-7.9+side*.2625);
 }
 const servingCup=cube(.09,.11,.09,'#d15a49',0,.53,.58,keg),servingRim=cube(.095,.015,.095,'#fff1d1',0,.59,.58,keg);servingCup.visible=servingRim.visible=false;
 let time=0,kegStand=false;
 const update=dt=>{
  time+=dt;const t=time%20,occasion=time%65;const stand=occasion>=48&&occasion<57?Math.min(1,(occasion-48)/1.5,(57-occasion)/1.5):0;kegStand=stand>0;
  // Approach, fill, take a drink, then wander back to a friend, always left of play.
  const fill=t>=4&&t<8,drink=t>=9&&t<13;
  const amount=t<4?t/4:t<13?1:t<17?1-(t-13)/4:0;
  guest.g.position.set(-2.45+.48*amount,0,-5.7-.38*amount);guest.g.rotation.y=fill?-.45:drink?-.1:.5;
  guest.g.position.y=(t<4||t>=13&&t<17)?Math.sin(t*8)*.012:0;
  guest.arm.rotation.x=fill?-1.05:drink?-2.4:-.15;
  guest.arm.rotation.z=fill?-.65:drink?-.15:0;
  stream.visible=servingCup.visible=servingRim.visible=fill;guest.cup.visible=guest.rim.visible=!fill;friend.g.rotation.y=.8+Math.sin(time*.3)*.13;friend.arm.rotation.x=-.3-Math.max(0,Math.sin(time*.45))*.5;
  plunger.position.y=1.27+(fill?.045*Math.sin(time*8):.025);tap.position.set(-.05*stand,.79+.17*stand,.5+.08*stand);squeeze.rotation.x=fill||stand>0?-.7:-.35;shapeHose();
  a.arm.rotation.x=-.8-Math.max(0,Math.sin(time*.4))*.5;b.arm.rotation.x=-.85-Math.max(0,Math.sin(time*.4+Math.PI))*.5;
  // One short gag per 65-second cycle; blend into/out of an inverted pose.
  guest.g.rotation.z=Math.PI*stand;
  if(stand>0){
   guest.g.position.lerp(new THREE.Vector3(-1.75,1.85,-6.16),stand);guest.g.rotation.y*=1-stand;
   friend.g.position.set(-3.65+1.63*stand,0,-6.65+.4*stand);friend.g.rotation.y*=1-stand;
   friend.arm.rotation.z=2.45*stand;friend.arm.scale.y=1+.6*stand;
   guest.cup.visible=guest.rim.visible=servingCup.visible=servingRim.visible=stream.visible=false;
  }else{friend.g.position.set(-3.65,0,-6.65);friend.arm.rotation.z=0;friend.arm.scale.y=1;}
  for(const p of pawns)p.m.position.z=p.z-p.side*.15*Math.min(1,Math.max(0,Math.sin(time*.14)));
 };
 return {root,keg,pump,plunger,hose,tap,guest,chessPlayers:[a,b],stream,update,get time(){return time;},get kegStand(){return kegStand;}};
}
