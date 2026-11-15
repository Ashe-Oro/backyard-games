import * as THREE from 'three';
// Pure scripts: alternating red/blue turns; h/b score, m/s waste a turn.
export const pongCatalog=Object.freeze([
 {id:'red-blowout',name:'Red blowout',shots:['hmhbhhmhbhhh','hmmsmhmhmms']},
 {id:'blue-close',name:'Blue close',shots:['hmhhbhhmhhh','hhhmbhhbhhh']},
 {id:'comeback',name:'Comeback',shots:['mmmmhhbhhhhhhh','hhhhhhmmsmsms']},
 {id:'blue-solid',name:'Blue solid',shots:['hmhhmmhsmhhmmhm','hhmhbhmhhmbhmhh']},
 {id:'red-long',name:'Red long',shots:['hmsmhmsbhmsmhmsbhmsmhmsbhmsmhmsbh','mhsmhmsmhmsbhmsmhmsbhmsmhmsmhmsm']}
].map(entry=>Object.freeze({...entry,shots:Object.freeze(entry.shots)})));
export const pongTiming=Object.freeze({turnSeconds:4.05,releaseAt:1.18,bounceAt:1.83,swatAt:2.12,impactAt:2.53,celebrationSeconds:5.4});
// Six of eight rounds clip the next standing cup; the others land on the table.
export function buildPongMissPath(side,round,cup) {
 const end=side===0?1:-1,kind=round%8;
 if(kind<2) return {missType:kind===0?'short':'long',bounce:null,
  target:{x:end*(kind===0?.34:.99),y:.672,z:cup.z}};
 const direction=round%2===0?-1:1,ground=round%3===0;
 return {missType:'rim',
  bounce:{x:cup.x-end*.025,y:.795,z:cup.z+direction*.055},
  target:{x:cup.x-end*.12,y:ground?.032:.672,z:direction*(ground?.50:.34)}};
}
export function selectNext(previous,random=Math.random) {
 const choices=pongCatalog.filter(entry=>entry.id!==previous);
 return choices[Math.floor(random()*choices.length)];
}
export function buildPongScript(id,random=Math.random) {
 const entry=pongCatalog.find(entry=>entry.id===id);
 if (!entry) throw new Error('Unknown pong variation: '+id);
 const swap=id==='comeback'&&random()<.5;
 const hits=[0,0],turns=[],types={h:'hit',b:'bounce',m:'miss',s:'swat'};
 for(let round=0;round<Math.max(...entry.shots.map(shots=>shots.length));round++) {
  for(const sourceSide of [0,1]) {
   const result=types[entry.shots[sourceSide][round]];
   if(!result) continue;
   const side=swap?1-sourceSide:sourceSide,scores=result==='hit'||result==='bounce';
   turns.push({side,result,scores,round,before:[...hits]});
   if(scores&&++hits[side]===10) return {id,turns,winner:side,hits};
  }
 }
 throw new Error('Pong script has no winner: '+id);
}
export function createPongMatch(previous=null,random=Math.random) {
 const script=buildPongScript(selectNext(previous,random).id,random);
 const matchSeconds=(script.turns.length-1)*pongTiming.turnSeconds+pongTiming.impactAt;
 return {...script,matchSeconds,pongLoopSeconds:matchSeconds+pongTiming.celebrationSeconds};
}
// Cabinet decoration only. Never registers objects with bag collision/scoring.
export function createParty(scene,{random=Math.random}={}){
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
 function person(name,shirt,skin,hair,x,z,seated=false,angle=0,handedness=1){
  const g=new THREE.Group();g.name=name;g.position.set(x,0,z);g.rotation.y=angle;root.add(g);
  const offset=seated?-.1:0;
  cube(.27,.34,.17,shirt,0,.66+offset,0,g);cube(.24,.23,.23,skin,0,.97+offset,0,g);cube(.25,.075,.24,hair,0,1.095+offset,0,g);
  for(const dx of [-.062,.062]){cube(.028,.026,.012,'#26352d',dx,.985+offset,.121,g);cube(.025,.025,.016,'#dc996e',dx,.94+offset,.121,g);}
  for(const dx of [-.077,.077]){cube(.09,seated?.28:.34,.1,'#34566a',dx,seated?.21:.32,seated?.2:0,g);if(seated)cube(.09,.08,.27,'#34566a',dx,.4,.09,g);cube(.12,.065,.18,'#2b3e39',dx,seated?.065:.12,seated?.26:.035,g);}
  const arm=new THREE.Group();arm.position.set(.175*handedness,.79+offset,0);g.add(arm);cube(.085,.17,.09,shirt,0,-.075,0,arm);const forearm=cube(.075,.15,.075,skin,0,-.23,0,arm);
  cube(.085,.29,.09,shirt,-.175*handedness,.65+offset,0,g);cube(.075,.06,.075,skin,-.175*handedness,.48+offset,0,g);
  const cup=cube(.09,.11,.09,'#d15a49',0,-.29,.035,arm);const rim=cube(.095,.018,.095,'#fff1d1',0,-.23,.035,arm);
  return {g,arm,forearm,cup,rim};
 }
 const KEG_X=5.4,KEG_Z=-.45;
 const guest=person('cup-filling-guest','#d7a349','#bd835c','#49392c',-2.05+KEG_X,-5.9+KEG_Z);
 const friend=person('chatting-guest','#72a0a0','#e3b085','#523c30',-3.65+KEG_X,-6.65+KEG_Z,false,.9);
 const pongTable = new THREE.Group();
 pongTable.name = 'beer-pong-table';
 pongTable.position.set(-2.77, 0, -7.65);
 pongTable.rotation.y = Math.PI / 2;
 root.add(pongTable);
 const tableTop=.64;
 cube(2.12,.09,.88,'#705039',0,tableTop-.07,0,pongTable);
 cube(2.05,.025,.81,'#336b53',0,tableTop-.0125,0,pongTable);
 for (const end of [-1,1]) {
  cube(.023,.004,.76,'#e4d8aa',end*.97,tableTop+.002,0,pongTable);
  for (const edge of [-1,1]) cube(.09,tableTop-.07,.09,'#5b4032',end*.88,(tableTop-.07)/2,edge*.33,pongTable);
 }
 for (const edge of [-1,1]) cube(1.95,.004,.022,'#e4d8aa',0,tableTop+.002,edge*.38,pongTable);

 const pongPlayers = [
  person('pong-red','#a95549','#e8be96','#72503c',-1.29,-.22,false,Math.PI/2,-1),
  person('pong-blue','#52749a','#95674e','#303c35',1.29,.12,false,-Math.PI/2)
 ];
 for (const player of pongPlayers) {
  pongTable.add(player.g);
  player.cup.visible = player.rim.visible = false;
  player.elbow = new THREE.Group();
  player.elbow.position.y = -.15;
  player.arm.add(player.elbow);
  player.elbow.add(player.forearm);
  player.forearm.position.y = -.08;
 }
 const rack = [-1,1].map((end, side) => {
  const cups = [];
  const wall = new THREE.CylinderGeometry(.068,.048,.13,8,1,true);
  const lip = new THREE.TorusGeometry(.063,.008,4,8);
  const inside = new THREE.CylinderGeometry(.055,.055,.008,8);
  const plastic = new THREE.MeshLambertMaterial({color:side ? '#387dce' : '#dd493c',side:THREE.DoubleSide,flatShading:true});
  for (let row=0;row<4;row++) for (let slot=0;slot<=row;slot++) {
   const cup = new THREE.Group();
   cup.position.set(end*(.45+row*.146),tableTop,(slot-row/2)*.146);
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
 const ballRadius=.032;
 const pongBall = new THREE.Mesh(new THREE.SphereGeometry(ballRadius,8,6),new THREE.MeshBasicMaterial({color:'#fff8d9'}));
 pongTable.add(pongBall);
 const handPoint=new THREE.Vector3(0,-.16,.02);
 function poseArm(player,shoulder,elbow,sweep=0) {
  player.arm.rotation.set(shoulder,0,sweep);
  player.elbow.rotation.x=elbow;
 }
 function handPosition(player) {
  return pongTable.worldToLocal(player.elbow.localToWorld(handPoint.clone()));
 }
 const releasePose=[-2.12,-.45];
 const swatPose=[-1.62,-.38,.38];
 const launchPoints=pongPlayers.map(player=>{
  poseArm(player,...releasePose);
  return handPosition(player);
 });
 const swatPoints=pongPlayers.map((player,side)=>{
  poseArm(player,swatPose[0],swatPose[1],(side===0?-1:1)*swatPose[2]);
  return handPosition(player);
 });

 const {turnSeconds,releaseAt,bounceAt,swatAt,impactAt}=pongTiming;
 let currentMatch,matchStart=0,pongTurns;
 const pongState={turn:0,winner:null,remaining:[10,10]};
 function startMatch(previous=null) {
  currentMatch=createPongMatch(previous,random);
  pongTurns=currentMatch.turns.map(shot=>{
   const {side,result,round,before}=shot;
   const target=rack[1-side][before[side]].position.clone();
   target.y+=.13;
   const miss=result==='miss'?buildPongMissPath(side,round,target):null;
   if(miss) target.copy(miss.target);
   const bounce=miss?.bounce?new THREE.Vector3().copy(miss.bounce):result==='bounce'||result==='swat'
    ?new THREE.Vector3((side===0?1:-1)*.43,tableTop+ballRadius,target.z*.45):null;
   const contact=result==='swat'?swatPoints[1-side]:null;
   if(contact) target.copy(contact).add(new THREE.Vector3(side===0?.25:-.25,-.37,side===0?.48:-.48));
   return {...shot,target,bounce,contact,missType:miss?.missType};
  });
  Object.assign(pongState,{turn:0,winner:null,remaining:[10,10]});
  rack.forEach(cups=>cups.forEach(cup=>{cup.visible=true;}));
 }
 startMatch();
 function moveBall(from,to,fraction,arc) {
  pongBall.position.copy(from).lerp(to,fraction);
  pongBall.position.y+=arc*4*fraction*(1-fraction);
 }
 function updatePong() {
  while(time-matchStart>=currentMatch.pongLoopSeconds) {
   matchStart+=currentMatch.pongLoopSeconds;
   startMatch(currentMatch.id);
  }
  const elapsed=time-matchStart;
  const finished=elapsed>=currentMatch.matchSeconds;
  const turn=Math.min(pongTurns.length-1,Math.floor(elapsed/turnSeconds));
  const shot=pongTurns[turn];
  const phase=elapsed-turn*turnSeconds;
  const sunk=[...shot.before];
  if (phase>=impactAt&&shot.scores) sunk[shot.side]++;
  pongState.turn=turn;
  pongState.remaining=[10-sunk[1],10-sunk[0]];
  pongState.winner=sunk[shot.side]===10?shot.side:null;
  rack.forEach((cups,side)=>cups.forEach((cup,index)=>{cup.visible=index>=sunk[1-side];}));
  for (const [side,player] of pongPlayers.entries()) {
   const cheering=pongState.winner===side;
   const cheer=cheering?Math.abs(Math.sin((phase-impactAt)*5)):0;
   player.g.position.y=cheer*.13;
   poseArm(player,-.2,0);
   if (cheering) {
    poseArm(player,-2.8+cheer*.28,-.4);
   } else if (!finished&&side===shot.side&&phase<1.58) {
    // Raise the ball beside the face, extend the forearm, then follow through.
    const windup=THREE.MathUtils.smoothstep(phase,.12,.85);
    const release=THREE.MathUtils.smoothstep(phase,.85,releaseAt);
    const settle=THREE.MathUtils.smoothstep(phase,releaseAt,1.58);
    poseArm(player,-.2-2.35*windup+.43*release+1.92*settle,-1.25*windup+.8*release+.45*settle);
   } else if (shot.contact&&side!==shot.side) {
    const reach=phase<=swatAt
     ? THREE.MathUtils.smoothstep(phase,bounceAt,swatAt)
     : 1-THREE.MathUtils.smoothstep(phase,swatAt,swatAt+.34);
    poseArm(player,THREE.MathUtils.lerp(-.2,swatPose[0],reach),swatPose[1]*reach,(side===0?-1:1)*swatPose[2]*reach);
   }
  }
  pongBall.visible=!finished&&phase<impactAt;
  if (!pongBall.visible) return;
  const launch=launchPoints[shot.side];
  if (phase<releaseAt) {
   pongBall.position.copy(handPosition(pongPlayers[shot.side]));
  } else if (!shot.bounce) {
   moveBall(launch,shot.target,(phase-releaseAt)/(impactAt-releaseAt),.28);
  } else if (phase<bounceAt) {
   moveBall(launch,shot.bounce,(phase-releaseAt)/(bounceAt-releaseAt),.14);
  } else if (!shot.contact) {
   moveBall(shot.bounce,shot.target,(phase-bounceAt)/(impactAt-bounceAt),.21);
  } else if (phase<swatAt) {
   moveBall(shot.bounce,shot.contact,(phase-bounceAt)/(swatAt-bounceAt),.17);
  } else {
   moveBall(shot.contact,shot.target,(phase-swatAt)/(impactAt-swatAt),.09);
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
 return {root,keg,pump,plunger,hose,tap,guest,friend,cook,foods,snack,pongTable,pongPlayers,pongBall,get pongLoopSeconds(){return currentMatch.pongLoopSeconds;},get pongTurns(){return pongTurns;},get pongVariationId(){return currentMatch.id;},pongState,stream,update,get catches(){return catches;},get time(){return time;},get kegStand(){return kegStand;}};
}
