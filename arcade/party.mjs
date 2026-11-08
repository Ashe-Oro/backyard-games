import * as THREE from 'three';
// Cabinet decoration only. Never registers objects with bag collision/scoring.
export function createParty(scene){
 const root=new THREE.Group();root.name='arcade-hangout';scene.add(root);
 const geo=new THREE.BoxGeometry(1,1,1),materials=new Map();
 const mat=c=>{if(!materials.has(c))materials.set(c,new THREE.MeshLambertMaterial({color:c}));return materials.get(c);};
 function cube(w,h,d,c,x,y,z,parent=root){const m=new THREE.Mesh(geo,mat(c));m.scale.set(w,h,d);m.position.set(x,y,z);m.castShadow=m.receiveShadow=true;parent.add(m);return m;}
 const keg=new THREE.Group();keg.name='keg';keg.position.set(3.7,0,-7.15);root.add(keg);
 cube(.68,.13,.62,'#7a5338',0,.13,0,keg);for(const x of [-.23,.23])cube(.09,.24,.48,'#624730',x,.12,0,keg);
 const shell=new THREE.Mesh(new THREE.CylinderGeometry(.29,.29,.65,8),mat('#aebcb5'));shell.position.y=.585;shell.castShadow=shell.receiveShadow=true;keg.add(shell);
 for(const y of [.30,.42,.76,.88]){const band=new THREE.Mesh(new THREE.CylinderGeometry(.31,.31,.07,8),mat('#647b78'));band.position.y=y;keg.add(band);}
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
 const KEG_X=5.4,KEG_Z=-.45;
 const guest=person('cup-filling-guest','#d7a349','#bd835c','#49392c',-2.05+KEG_X,-5.9+KEG_Z);
 const friend=person('chatting-guest','#72a0a0','#e3b085','#523c30',-3.65+KEG_X,-6.65+KEG_Z,false,.9);
 const pongTable = new THREE.Group();
 pongTable.name = 'beer-pong-table';
 pongTable.position.set(-2.77, 0, -7.65);
 pongTable.rotation.y = Math.PI / 2;
 root.add(pongTable);
 cube(2.12,.09,.88,'#705039',0,.725,0,pongTable);
 cube(2.05,.025,.81,'#336b53',0,.782,0,pongTable);
 for (const end of [-1,1]) {
  cube(.023,.004,.76,'#e4d8aa',end*.97,.798,0,pongTable);
  for (const edge of [-1,1]) cube(.09,.69,.09,'#5b4032',end*.88,.345,edge*.33,pongTable);
 }
 for (const edge of [-1,1]) cube(1.95,.004,.022,'#e4d8aa',0,.798,edge*.38,pongTable);

 const pongPlayers = [
  person('pong-red','#a95549','#e8be96','#72503c',-1.29,-.22,false,Math.PI/2),
  person('pong-blue','#52749a','#95674e','#303c35',1.29,.12,false,-Math.PI/2)
 ];
 for (const player of pongPlayers) {
  pongTable.add(player.g);
  player.cup.visible = player.rim.visible = false;
 }
 const rack = [-1,1].map((end, side) => {
  const cups = [];
  const wall = new THREE.CylinderGeometry(.068,.048,.13,8,1,true);
  const lip = new THREE.TorusGeometry(.063,.008,4,8);
  const inside = new THREE.CylinderGeometry(.055,.055,.008,8);
  const plastic = new THREE.MeshLambertMaterial({color:side ? '#387dce' : '#dd493c',side:THREE.DoubleSide,flatShading:true});
  for (let row=0;row<4;row++) for (let slot=0;slot<=row;slot++) {
   const cup = new THREE.Group();
   cup.position.set(end*(.45+row*.146),.799,(slot-row/2)*.146);
   const sides = new THREE.Mesh(wall,plastic);
   sides.position.y=.065;
   const rim = new THREE.Mesh(lip,mat('#fff2ce'));
   rim.rotation.x=Math.PI/2;
   rim.position.y=.13;
   const liquid = new THREE.Mesh(inside,mat('#4b3528'));
   liquid.position.y=.085;
   cup.add(sides,rim,liquid);
   pongTable.add(cup);
   cups.push(cup);
  }
  return cups;
 });
 const pongBall = new THREE.Mesh(new THREE.SphereGeometry(.032,8,6),new THREE.MeshBasicMaterial({color:'#fff8d9'}));
 pongTable.add(pongBall);
 const pongTurns = [];
 const hits = [0,0];
 // The red player takes the last cup after a few misses on both sides.
 for (let round=0;round<11;round++) {
  for (const side of [0,1]) {
   const miss = side===0 ? round===2 : [1,5,8].includes(round);
   const target = rack[1-side][hits[side]].position.clone();
   if (miss) target.z += target.z<0 ? -.22 : .22;
   target.y += miss ? .025 : .105;
   pongTurns.push({side,miss,target,before:[...hits]});
   if (!miss && ++hits[side]===10) break;
  }
 }
 const turnSeconds=2.45, releaseAt=.65, impactAt=1.65;
 const matchSeconds=pongTurns.length*turnSeconds;
 const pongLoopSeconds=matchSeconds+3.5;
 const handPoint=new THREE.Vector3(0,-.30,.02);
 const launchPoints=pongPlayers.map(({arm})=>{
  arm.rotation.x=-1.44;
  const point=pongTable.worldToLocal(arm.localToWorld(handPoint.clone()));
  arm.rotation.x=0;
  return point;
 });
 const pongState={turn:0,winner:null,remaining:[10,10]};
 function updatePong() {
  const elapsed=time%pongLoopSeconds;
  const finished=elapsed>=matchSeconds;
  const turn=Math.min(pongTurns.length-1,Math.floor(elapsed/turnSeconds));
  const shot=pongTurns[turn];
  const phase=elapsed-turn*turnSeconds;
  const landed=phase>=impactAt;
  const sunk=[...shot.before];
  if (landed&&!shot.miss) sunk[shot.side]++;
  pongState.turn=turn;
  pongState.remaining=[10-sunk[1],10-sunk[0]];
  pongState.winner=sunk[shot.side]===10?shot.side:null;
  rack.forEach((cups,side)=>cups.forEach((cup,index)=>{cup.visible=index>=sunk[1-side];}));
  for (const [side,player] of pongPlayers.entries()) {
   const tossing=!finished&&side===shot.side&&phase<1.1;
   const cheering=pongState.winner===side;
   const cheer=cheering?Math.abs(Math.sin((phase-impactAt)*5)):0;
   player.g.position.y=cheer*.13;
   player.arm.rotation.x=cheering?-2.8+cheer*.28:tossing
    ? -.45-.99*Math.max(0,1-Math.abs(phase-releaseAt)/.45) : -.2;
  }
  pongBall.visible=!finished&&phase<impactAt;
  if (!pongBall.visible) return;
  if (phase<releaseAt) {
   pongBall.position.copy(pongTable.worldToLocal(pongPlayers[shot.side].arm.localToWorld(handPoint.clone())));
  } else {
   const flight=(phase-releaseAt)/(impactAt-releaseAt);
   pongBall.position.copy(launchPoints[shot.side]).lerp(shot.target,flight);
   pongBall.position.y+=.58*4*flight*(1-flight);
  }
 }
 const servingCup=cube(.09,.11,.09,'#d15a49',0,.53,.58,keg),servingRim=cube(.095,.015,.095,'#fff1d1',0,.59,.58,keg);servingCup.visible=servingRim.visible=false;
 let time=0,kegStand=false;
 // Perimeter landscaping: no flowers or hazards in the throwing corridor.
 for(const [x,z,h] of [[-5.4,-6.1,.7],[-4.8,-9.5,.5],[5.7,-5.8,.6],[5.9,-9.5,.8],[-5.7,-3,.4],[5.7,-2.5,.5]]){
  const g=new THREE.Group();g.name='perimeter-cactus';g.position.set(x,0,z);root.add(g);
  cube(.14,h,.14,'#477c43',0,h/2,0,g);cube(.06,h*.75,.15,'#65954a',-.045,h*.4,0,g);
  for(const sign of [-1,1]){cube(.2,.1,.11,'#477c43',sign*.13,h*.48,0,g);cube(.1,h*.3,.11,'#477c43',sign*.2,h*.6,0,g);}
 }
 const cook=person('grill-cook','#eee0ba','#c18b64','#3d3830',3.0,-8.85,false,-.5);cook.cup.visible=cook.rim.visible=false;
 cube(.26,.1,.25,'#fff1d1',0,1.16,0,cook.g);cube(.16,.15,.02,'#b75b43',0,.6,.1,cook.g);
 const spatula=new THREE.Group();cook.arm.add(spatula);spatula.position.set(0,-.3,.03);cube(.025,.22,.025,'#6d4b32',0,-.08,0,spatula);cube(.10,.1,.025,'#b7c8c2',0,-.24,0,spatula);
 const foods=[];
 for(const [x,z,type] of [[2.58,-8.25,'steak'],[2.78,-8.45,'steak'],[2.9,-8.25,'burger'],[2.72,-8.25,'burger']]){
  const g=new THREE.Group();g.position.set(x,.75,z);root.add(g);g.name=type;
  cube(type==='steak'?.14:.1,.035,type==='steak'?.09:.1,type==='steak'?'#9e4938':'#75402c',0,0,0,g);
  for(const dx of [-.035,0,.035])cube(.012,.004,.08,'#3c3229',dx,.02,0,g);
  foods.push({g,base:g.position.clone(),type});
 }
 const snack=new THREE.Group();snack.name='dog-burger';snack.visible=false;root.add(snack);cube(.12,.045,.11,'#d89e4d',0,0,0,snack);cube(.115,.025,.105,'#74422d',0,-.025,0,snack);
 let catches=0,lastSnackCycle=-1;
 function updateCook(dt,dog){
  const cycle=time%36,flip=(time%8)/1.3,flipping=flip<1;
  cook.arm.rotation.x=-.8-(flipping?Math.sin(flip*Math.PI)*1.4:0);cook.arm.rotation.z=-.5;
  const food=foods[Math.floor(time/8)%foods.length];
  for(const f of foods){f.g.position.copy(f.base);f.g.rotation.z=0;}
  if(flipping){food.g.position.y+=Math.sin(flip*Math.PI)*.25;food.g.rotation.z=flip*Math.PI*2;}
  const toss=cycle>=25&&cycle<26.4,progress=(cycle-25)/1.4;
  snack.visible=toss&&!!dog;
  if(toss&&dog){
   const toward=dog.g.worldToLocal(new THREE.Vector3(2.9,.8,-8.6));dog.head.rotation.y=Math.atan2(-toward.x,-toward.z);
   const mouth=dog.head.localToWorld(new THREE.Vector3(0,-.06,-.22));
   snack.position.copy(spatula.localToWorld(new THREE.Vector3(0,-.24,0))).lerp(mouth,progress);snack.position.y+=Math.sin(progress*Math.PI)*.75;
   snack.rotation.z=progress*Math.PI*3;cook.arm.rotation.x=-1.9;
   dog.head.rotation.z=-Math.sin(progress*Math.PI)*.18;dog.tail.rotation.y=Math.sin(time*24);
  }
  if(cycle>=26.4&&Math.floor(time/36)!==lastSnackCycle&&dog){lastSnackCycle=Math.floor(time/36);catches++;dog.head.rotation.z=0;}
 }

 const update=(dt,dog)=>{
  time+=dt;const t=time%20,occasion=time%65;const stand=occasion>=48&&occasion<57?Math.min(1,(occasion-48)/1.5,(57-occasion)/1.5):0;kegStand=stand>0;
  // Approach, fill, take a drink, then wander back to a friend, always left of play.
  const fill=t>=4&&t<8,drink=t>=9&&t<13;
  const amount=t<4?t/4:t<13?1:t<17?1-(t-13)/4:0;
  guest.g.position.set(-2.45+KEG_X+.48*amount,0,-5.7+KEG_Z-.38*amount);guest.g.rotation.y=fill?-.45:drink?-.1:.5;
  guest.g.position.y=(t<4||t>=13&&t<17)?Math.sin(t*8)*.012:0;
  guest.arm.rotation.x=fill?-1.05:drink?-2.4:-.15;
  guest.arm.rotation.z=fill?-.65:drink?-.15:0;
  stream.visible=servingCup.visible=servingRim.visible=fill;guest.cup.visible=guest.rim.visible=!fill;friend.g.rotation.y=.8+Math.sin(time*.3)*.13;friend.arm.rotation.x=-.3-Math.max(0,Math.sin(time*.45))*.5;
  plunger.position.y=1.27+(fill?.045*Math.sin(time*8):.025);tap.position.set(0,.79+.09*stand,.5-.141*stand);squeeze.rotation.x=fill||stand>0?-.7:-.35;shapeHose();

  // One short gag per 65-second cycle; blend into/out of an inverted pose.
  guest.g.rotation.x=Math.PI*stand;guest.g.rotation.z=0;
  if(stand>0){
   guest.g.position.lerp(new THREE.Vector3(keg.position.x,1.85,keg.position.z+.48),stand);guest.g.rotation.y*=1-stand;
   friend.g.position.set(keg.position.x-.65+.2*stand,0,keg.position.z-.55+.9*stand);friend.g.rotation.y*=1-stand;
   friend.arm.rotation.z=2.45*stand;friend.arm.scale.y=1+.6*stand;
   guest.cup.visible=guest.rim.visible=servingCup.visible=servingRim.visible=stream.visible=false;
  }else{friend.g.position.set(keg.position.x-.65,0,keg.position.z-.55);friend.arm.rotation.z=0;friend.arm.scale.y=1;}
  updatePong();
  updateCook(dt,dog);

 };
 return {root,keg,pump,plunger,hose,tap,guest,friend,cook,foods,snack,pongTable,pongPlayers,pongBall,pongLoopSeconds,pongState,stream,update,get catches(){return catches;},get time(){return time;},get kegStand(){return kegStand;}};
}
