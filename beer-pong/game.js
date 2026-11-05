import * as THREE from 'three';
import {layoutsFor,layoutPoints} from './rerack.mjs';
import {OPPONENTS,SwatDefender} from './opponents.mjs';
import {createGlove} from './glove.js';
import {FEATURES} from './features.mjs';
import {createBackyard} from '../yard-games/backyard.js';

const $=id=>document.getElementById(id);
const clamp=(n,a,b)=>Math.max(a,Math.min(b,n));
const G=9.8, BALL_R=.02, TABLE_Y=.76, TABLE_W=1.16, TABLE_L=3.5, TABLE_Z=-1.2;
const CUP_R=.084, CUP_H=.17, RIM_THICKNESS=.004, RIM_Y=TABLE_Y+CUP_H, HOLE_R=CUP_R-BALL_R-RIM_THICKNESS;
const RIM_RESTITUTION=.30, WALL_RESTITUTION=.25;
const RELEASE_Y=1.09, RELEASE_Z=[TABLE_Z+TABLE_L/2+.35,TABLE_Z-TABLE_L/2-.35], ANGLE=52*Math.PI/180;
const COLORS=[0xe75c46,0x448fc4], LABELS=['RED','BLUE'];
const state={mode:'practice',phase:'menu',turn:0,paused:false,shots:[0,0],hits:[0,0],positions:[0,0],shotStyle:'arc',opponent:'regular',reracksUsed:[false,false],winner:null,power:0,delay:0,cpuTime:0};
const releaseX=team=>state.positions[team]*(team===0?1:-1);
let ball=null,drag=null,viewTeam=0,accumulator=0,messageTime=0,swipeTimer,helpFromPause=false,humanSwing=null,humanPointer=null;
const wrap=$('game'), scene=new THREE.Scene();
scene.background=new THREE.Color('#edc5a0');scene.fog=new THREE.Fog('#c9d6ac',12,35);
let renderer;
try{renderer=new THREE.WebGLRenderer({antialias:false,powerPreference:'high-performance'});}catch(error){$('error').hidden=false;throw error;}
renderer.setPixelRatio(1);renderer.shadowMap.enabled=true;renderer.shadowMap.type=THREE.BasicShadowMap;
wrap.prepend(renderer.domElement);
const camera=new THREE.PerspectiveCamera(40,1,.05,80);
const defender=new SwatDefender('regular',()=>Math.random(),{homeZ:TABLE_Z-TABLE_L*.16,nearZ:TABLE_Z+.05,farZ:TABLE_Z-TABLE_L/2+.12}),glove=createGlove();scene.add(glove);
const playerGlove=createGlove();scene.add(playerGlove);
scene.add(new THREE.HemisphereLight('#e3eee8','#426139',1.4));
const sun=new THREE.DirectionalLight('#ffd3a5',2.1);sun.position.set(-4,7,3);sun.castShadow=true;
sun.shadow.mapSize.set(1024,1024);Object.assign(sun.shadow.camera,{left:-5,right:5,top:5,bottom:-5,near:.5,far:20});
sun.shadow.bias=-.0006;sun.shadow.normalBias=.012;scene.add(sun);
const mat=color=>new THREE.MeshLambertMaterial({color});
function box(w,h,d,material,x,y,z,parent=scene){const mesh=new THREE.Mesh(new THREE.BoxGeometry(w,h,d),material);mesh.position.set(x,y,z);mesh.castShadow=mesh.receiveShadow=true;parent.add(mesh);return mesh;}
function texture(w,h,draw,repeat=1){
 const canvas=document.createElement('canvas');canvas.width=w;canvas.height=h;draw(canvas.getContext('2d'),w,h);
 const map=new THREE.CanvasTexture(canvas);map.colorSpace=THREE.SRGBColorSpace;map.magFilter=map.minFilter=THREE.NearestFilter;map.generateMipmaps=false;
 map.wrapS=map.wrapT=THREE.RepeatWrapping;map.repeat.set(repeat,repeat);return map;
}
const {wood,woodDark}=createBackyard(scene,{farZ:-7,nearZ:5,propZ:-1.5});
// One readable playing surface. Its markings stay on the table, never on the lawn.
const tableMap=texture(128,256,(g,w,h)=>{
 g.fillStyle='#286d62';g.fillRect(0,0,w,h);
 for(let i=0;i<500;i++){g.fillStyle=i%2?'#2b7568':'#25675e';g.fillRect(Math.random()*w|0,Math.random()*h|0,2,3);}
 g.strokeStyle='#dfd9b6';g.lineWidth=2;g.strokeRect(4,4,w-8,h-8);g.fillRect(w/2-1,4,2,h-8);
 g.fillStyle='#dfd9b6';g.fillRect(4,h/2-1,w-8,2);
 g.fillStyle='#8bab80';for(const [x,y,ww,hh]of [[w/2-5,h/2-3,10,6],[w/2-3,h/2-5,6,10]])g.fillRect(x,y,ww,hh);g.fillStyle='#286d62';g.fillRect(w/2-2,h/2-2,4,4);
});
const tableSide=wood,tableTop=new THREE.MeshLambertMaterial({map:tableMap});
box(TABLE_W,.075,TABLE_L,[tableSide,tableSide,tableTop,tableSide,tableSide,tableSide],0,TABLE_Y-.0375,TABLE_Z);
for(const x of [-.43,.43])for(const z of [TABLE_Z-TABLE_L/2+.38,TABLE_Z+TABLE_L/2-.38])box(.055,TABLE_Y-.08,.055,mat('#24413f'),x,(TABLE_Y-.08)/2,z);
box(.9,.045,.055,woodDark,0,.25,TABLE_Z-TABLE_L/2+.38);box(.9,.045,.055,woodDark,0,.25,TABLE_Z+TABLE_L/2-.38);
// Keep the original cup silhouette independent of forgiving collision tolerances.
const CUP_ART_R=.078;
const cupOuter=new THREE.CylinderGeometry(CUP_ART_R,.052,CUP_H,12,1,true);
const cupInner=new THREE.CylinderGeometry(CUP_ART_R-.006,.046,CUP_H-.012,12,1,true);
const rimGeo=new THREE.TorusGeometry(CUP_ART_R-.002,.006,4,16);
const rimMat=mat('#fff1cf'),innerMat=new THREE.MeshLambertMaterial({color:'#ffffff',side:THREE.BackSide});
const cups=[];
for(let team=0;team<2;team++)for(let row=0;row<4;row++)for(let col=0;col<=row;col++){
 const x=(col-row/2)*.18,farZ=TABLE_Z-TABLE_L/2+.34+(3-row)*.17,z=team===1?farZ:2*TABLE_Z-farZ;
 const group=new THREE.Group();group.position.set(x,TABLE_Y,z);scene.add(group);
 const outer=new THREE.Mesh(cupOuter,mat(COLORS[team]));outer.position.y=CUP_H/2;outer.castShadow=outer.receiveShadow=true;group.add(outer);
 const inner=new THREE.Mesh(cupInner,innerMat);inner.position.y=CUP_H/2+.004;group.add(inner);
 const rim=new THREE.Mesh(rimGeo,rimMat);rim.rotation.x=Math.PI/2;rim.position.y=CUP_H;group.add(rim);
 const liquid=new THREE.Mesh(new THREE.CircleGeometry(.052,12),mat('#bc7d3e'));liquid.rotation.x=-Math.PI/2;liquid.position.y=.045;group.add(liquid);
 cups.push({id:cups.length,team,x,z,initialX:x,initialZ:z,alive:true,enabled:true,group});
}
const ballGeo=new THREE.SphereGeometry(BALL_R,10,8),ballMat=mat('#fff5d9');
const held=new THREE.Mesh(ballGeo,ballMat);held.castShadow=held.receiveShadow=true;scene.add(held);
const flying=new THREE.Mesh(ballGeo,ballMat);flying.castShadow=flying.receiveShadow=true;flying.visible=false;scene.add(flying);
const sparkGeo=new THREE.BoxGeometry(.015,.015,.015),sparks=[];
for(let i=0;i<24;i++){const mesh=new THREE.Mesh(sparkGeo,new THREE.MeshBasicMaterial({color:i%2?'#ffe16b':'#fff1cf'}));mesh.visible=false;scene.add(mesh);sparks.push({mesh,v:new THREE.Vector3(),life:0});}
function celebrate(cup){for(const p of sparks){p.mesh.position.set(cup.x,RIM_Y,cup.z);p.v.set((Math.random()-.5)*.7,.5+Math.random()*.6,(Math.random()-.5)*.7);p.life=.65;p.mesh.visible=true;}}
function remaining(team){return cups.filter(c=>c.team===team&&c.alive).length;}
function resetCups(){for(const c of cups){c.x=c.initialX;c.z=c.initialZ;c.group.position.set(c.x,TABLE_Y,c.z);c.alive=true;c.enabled=state.mode!=='practice'||c.team===1;c.group.visible=c.enabled;c.group.scale.setScalar(1);}for(const s of sparks){s.life=0;s.mesh.visible=false;}}
function powerForPoint(x,z,team=state.turn){const d=Math.hypot(x-releaseX(team),z-RELEASE_Z[team]),dy=RIM_Y+BALL_R-RELEASE_Y;
 const speed=Math.sqrt(G*d*d/(2*Math.cos(ANGLE)**2*(d*Math.tan(ANGLE)-dy)));return clamp((speed-3.2)/3.4,0,1);}
function newBall(team,power,slope=0,style=team===0&&state.mode!=='two'?state.shotStyle:'arc'){
 const direction=team===0?-1:1,screenRight=team===0?1:-1,yaw=clamp(Math.atan(slope)*.65,-.35,.35),angle=style==='bounce'?36*Math.PI/180:ANGLE,speed=(style==='bounce'?2.8:3.2)+clamp(power,0,1)*3.4;
 return {team,power,slope,style,bounceAllowed:true,state:'fly',p:new THREE.Vector3(releaseX(team),RELEASE_Y,RELEASE_Z[team]),prev:new THREE.Vector3(),v:new THREE.Vector3(Math.sin(yaw)*Math.cos(angle)*speed*screenRight,Math.sin(angle)*speed,Math.cos(yaw)*Math.cos(angle)*speed*direction),age:0,sinkAge:0,swatAge:0,hitCup:null,bounces:0,justBounced:false,deadReason:null,rimHits:0};
}
function stepBall(b,dt){
 if(b.state!=='fly')return;b.age+=dt;b.prev.copy(b.p);b.v.y-=G*dt;b.p.addScaledVector(b.v,dt);
 for(const c of cups){
  if(!c.alive||!c.enabled)continue;
  if(b.v.y<0&&b.prev.y>=RIM_Y+BALL_R&&b.p.y<=RIM_Y+BALL_R){
   const fraction=(b.prev.y-RIM_Y-BALL_R)/(b.prev.y-b.p.y),x=THREE.MathUtils.lerp(b.prev.x,b.p.x,fraction),z=THREE.MathUtils.lerp(b.prev.z,b.p.z,fraction);
   const dx=x-c.x,dz=z-c.z,d=Math.hypot(dx,dz);
   if(d<HOLE_R){b.state='sink';b.hitCup=c;b.p.set(x,RIM_Y+BALL_R,z);return;}
   if(d<CUP_R+BALL_R+RIM_THICKNESS){
    const n=new THREE.Vector3(dx/Math.max(d,.001)*(d-CUP_R),BALL_R,dz/Math.max(d,.001)*(d-CUP_R)).normalize();
    const vn=b.v.dot(n);if(vn<0)b.v.addScaledVector(n,-(1+RIM_RESTITUTION)*vn);
    b.p.set(x,RIM_Y+BALL_R+.004,z);b.rimHits++;return;
   }
  }
  const dx=b.p.x-c.x,dz=b.p.z-c.z,d=Math.hypot(dx,dz);
  if(b.p.y>TABLE_Y+BALL_R&&b.p.y<RIM_Y&&d<CUP_R+BALL_R&&d>HOLE_R){
   const nx=dx/Math.max(d,.001),nz=dz/Math.max(d,.001),vn=b.v.x*nx+b.v.z*nz;
   b.p.x=c.x+nx*(CUP_R+BALL_R+.002);b.p.z=c.z+nz*(CUP_R+BALL_R+.002);
   if(vn<0){b.v.x-=(1+WALL_RESTITUTION)*vn*nx;b.v.z-=(1+WALL_RESTITUTION)*vn*nz;}b.rimHits++;
  }
 }
 const onTable=Math.abs(b.p.x)<TABLE_W/2+BALL_R&&Math.abs(b.p.z-TABLE_Z)<TABLE_L/2+BALL_R;
 if(onTable&&b.p.y<=TABLE_Y+BALL_R&&b.prev.y>=TABLE_Y+BALL_R){
  b.p.y=TABLE_Y+BALL_R;if(b.bounceAllowed&&b.bounces===0){b.bounces=1;b.justBounced=true;b.v.y=Math.abs(b.v.y)*.72;b.v.x*=.86;b.v.z*=.86;return;}
  b.v.set(0,0,0);b.deadReason=b.bounces?'second-bounce':'table';b.state='done';return;}
 if(b.p.y<=BALL_R){b.p.y=BALL_R;b.v.set(0,0,0);b.deadReason='ground';b.state='done';return;}
 if(b.age>5||Math.abs(b.p.x)>7||b.p.z>8||b.p.z<-10)b.state='done';
}
function setCamera(){
 const w=wrap.clientWidth,h=wrap.clientHeight,portrait=clamp((1.05-w/h)/.55,0,1);
 viewTeam=state.mode==='two'?state.turn:0;
 camera.position.set(0,2.2,viewTeam===0?2.65:2*TABLE_Z-2.65);camera.lookAt(0,.75+.15*portrait,TABLE_Z);
 camera.aspect=w/h;camera.fov=38+12*portrait;camera.updateProjectionMatrix();camera.updateMatrixWorld();
}
function resize(){humanPointer=null;if(drag)cancelPull();const w=wrap.clientWidth,h=wrap.clientHeight,scale=Math.max(1,Math.round(Math.min(h,w*.9)/430));renderer.setSize(Math.ceil(w/scale),Math.ceil(h/scale),false);setCamera();$('swipe-feedback').classList.remove('show');}
addEventListener('resize',resize);resize();
function setPower(power){state.power=clamp(power,0,1);}
function showThrowDirection(b){
 const horizontal=b.v.x*(b.team===0?1:-1),origin=ballScreen(),yaw=Math.atan2(horizontal,Math.abs(b.v.z));
 showAim(origin,{x:origin.x+Math.sin(yaw)*100,y:origin.y-Math.cos(yaw)*100});
}
function message(text,detail='',seconds=1.1){$('message').replaceChildren(document.createTextNode(text));if(detail){const small=document.createElement('small');small.textContent=detail;$('message').append(small);}$('message').classList.add('show');messageTime=seconds;}
function updateHud(){
 $('mode-name').textContent={practice:'SOLO PRACTICE',cpu:'VS CPU',two:'2 PLAYER'}[state.mode];
 $('turn-label').textContent=state.mode==='practice'?`${remaining(1)} CUPS LEFT`:state.mode==='cpu'?(state.turn===0?'YOUR SHOT':'CPU’S SHOT'):`${LABELS[state.turn]}’S SHOT`;
 $('red-label').textContent=state.mode==='cpu'?'YOUR CUPS':'RED CUPS';$('blue-label').textContent=state.mode==='cpu'?'CPU CUPS':'BLUE CUPS';
 for(const [team,id]of [[0,'red'],[1,'blue']]){$(`${id}-count`).textContent=remaining(team);$(`${id}-pips`).innerHTML=Array.from({length:10},(_,i)=>`<i class="${i>=remaining(team)?'out':''}"></i>`).join('');}
 $('scoreboard').hidden=state.mode==='practice';$('practice-stats').hidden=state.mode!=='practice';$('practice-hits').textContent=`${state.hits[0]} / ${state.shots[0]}`;
 $('control-hint').hidden=state.phase!=='aim';$('shot-count').textContent=`${state.mode==='practice'?'':LABELS[state.turn]+' · '}THROW ${state.shots[state.turn]+(['aim','cpu','handoff'].includes(state.phase)?1:0)}`;
 $('shot-style').hidden=state.mode==='two'||state.turn!==0||!['aim','flight','wait'].includes(state.phase);
 document.querySelectorAll('[data-shot-style]').forEach(b=>{b.setAttribute('aria-pressed',String(b.dataset.shotStyle===state.shotStyle));b.disabled=!humanReady()||!!drag;});
 const canOffer=layoutsFor(remaining(1-state.turn)).length>0;
 $('rerack-button').hidden=state.reracksUsed[state.turn]||!['aim','flight','wait'].includes(state.phase)||state.mode==='cpu'&&state.turn===1;
 $('rerack-button').disabled=!humanReady()||!canOffer||!!drag;$('rerack-button').title=canOffer?'Choose a formation for the cups you’re shooting at':'Available with 6, 4, 3 or 2 target cups left';
 $('refill').hidden=state.mode!=='practice';$('refill').disabled=state.phase!=='aim'||state.paused;
}
function closeDialogs(){for(const dialog of document.querySelectorAll('dialog'))if(dialog.open)dialog.close();}
function showMenu(){resetHumanDefense();defender.reset();glove.visible=false;closeDialogs();state.phase='menu';state.paused=false;ball=null;drag=null;wrap.classList.remove('drag');state.turn=0;state.delay=0;$('menu').hidden=false;$('hud').hidden=true;flying.visible=held.visible=false;$('swipe-feedback').classList.remove('show');$('message').classList.remove('show');state.mode='cpu';resetCups();setCamera();}
function startMatch(mode='practice',opponent='regular'){
 if(!['practice','cpu','two'].includes(mode))return;resetHumanDefense();closeDialogs();state.mode=mode;state.opponent=OPPONENTS[opponent]?opponent:'regular';defender.setProfile(state.opponent);glove.visible=false;state.shotStyle='arc';state.phase='wait';state.paused=false;state.turn=0;state.shots=[0,0];state.hits=[0,0];state.positions=[0,0];state.reracksUsed=[false,false];state.winner=null;state.power=0;state.delay=0;ball=null;drag=null;wrap.classList.remove('drag');flying.visible=false;
 $('menu').hidden=true;$('hud').hidden=false;$('message').classList.remove('show');$('swipe-feedback').classList.remove('show');resetCups();setCamera();beginTurn();
}
function beginTurn(){
 resetHumanDefense();ball=null;flying.visible=false;defender.reset();glove.visible=false;messageTime=0;$('message').classList.remove('show');setCamera();setPower(0);
 if(state.mode==='two'){
  state.phase='handoff';$('handoff-title').textContent=`${LABELS[state.turn]}’S SHOT`;$('handoff-copy').textContent=`Aim at the ${state.turn===0?'blue':'red'} cups.`;$('handoff').showModal();
 }else if(state.mode==='cpu'&&state.turn===1){state.phase='cpu';state.cpuTime=0;}
 else state.phase='aim';updateHud();
}
function launchShot(power,slope=0,style){
 if(state.paused||!['aim','cpu'].includes(state.phase)||document.querySelector('dialog[open]'))return false;
 resetHumanDefense();defender.reset();glove.visible=false;ball=newBall(state.turn,power,slope,style);state.phase='flight';state.shots[state.turn]++;flying.position.copy(ball.p);flying.scale.setScalar(1);flying.visible=true;held.visible=false;setPower(power);if(!(state.mode==='cpu'&&state.turn===1))showThrowDirection(ball);updateHud();return true;
}
// Search the same table/cup physics used by real throws, including the first rebound.
function bouncePlan(x,z,team){
 const yaw=Math.atan2((x-releaseX(team))*(team===0?1:-1),Math.abs(z-RELEASE_Z[team])),slope=Math.tan(yaw/.65);let best=null;
 for(let power=.1;power<=.8;power+=.0025){
  const probe=newBall(team,power,slope,'bounce');
  for(let i=0;i<720&&probe.state==='fly';i++){
   stepBall(probe,1/240);
   if(probe.bounces!==1||probe.rimHits||probe.v.y>=0||probe.prev.y<RIM_Y+BALL_R||probe.p.y>RIM_Y+BALL_R)continue;
   const f=(probe.prev.y-RIM_Y-BALL_R)/(probe.prev.y-probe.p.y),px=THREE.MathUtils.lerp(probe.prev.x,probe.p.x,f),pz=THREE.MathUtils.lerp(probe.prev.z,probe.p.z,f),cost=Math.hypot(px-x,pz-z);
   if(!best||cost<best.cost)best={power,slope,cost};break;
  }
 }
 return best;
}
function cpuShot(){
 if(!state.reracksUsed[1]){const option=layoutsFor(remaining(0))[0];if(option)applyRerack(1,option.id);}
 const rack=cups.filter(c=>c.team===0&&c.alive),cup=rack[Math.floor(Math.random()*rack.length)];
 if(!cup)return;const shooting=OPPONENTS[state.opponent].shoot,precise=Math.random()<shooting.precise,jitter=precise?.025:shooting.jitter,x=cup.x+(Math.random()-.5)*jitter*2,z=cup.z+(Math.random()-.5)*(precise?.035:.24);
 const d=Math.abs(z-RELEASE_Z[1]),yaw=Math.atan2(-(x-releaseX(1)),d),slope=Math.tan(yaw/.65);
 if(Math.random()<shooting.bounce){const plan=bouncePlan(x,z,1);if(plan){launchShot(plan.power,plan.slope,'bounce');return;}}
 launchShot(powerForPoint(x,z,1),slope);
}
function finishShot(scoredCup=null){
 const team=state.turn;
 if(scoredCup){
  scoredCup.alive=false;scoredCup.group.visible=false;celebrate(scoredCup);if(scoredCup.team!==team)state.hits[team]++;
  message(scoredCup.team===team?'OWN CUP!':'CUP!',scoredCup.team===team?'Wrong rack.':`${remaining(scoredCup.team)} cups left.`,1.2);
 }
 else if(ball?.deadReason==='swat')message(team===1?'YOU SWATTED!':'SWATTED!',team===0?`${OPPONENTS[state.opponent].name} blocked your bounce.`:'',1.2);
 else if(ball?.deadReason==='second-bounce')message('DEAD BALL','Second bounce.',.95);
 else if(ball?.deadReason==='table')message('DEAD BALL','Touched the table.',.95);
 else message('MISS',ball?.rimHits?'Off the rim.':'',.95);
 ball=null;flying.visible=false;
 if(state.mode==='practice'&&remaining(1)===0){state.phase='over';$('result-tag').textContent='RACK CLEARED';$('result-title').textContent='Ten down.';$('result-copy').textContent=`${state.hits[0]} cups in ${state.shots[0]} throws.`;$('play-again').textContent='NEW RACK';$('result').showModal();}
 else if(state.mode!=='practice'&&(remaining(0)===0||remaining(1)===0)){
  state.winner=remaining(1)===0?0:1;state.phase='over';$('result-tag').textContent='TABLE CHAMPION';$('result-title').textContent=state.mode==='cpu'?(state.winner===0?'YOU WIN!':'CPU WINS'):LABELS[state.winner]+' WINS!';$('result-copy').textContent=`${state.shots[state.winner]} throws to clear the rack.`;$('play-again').textContent='PLAY AGAIN';$('result').showModal();
 }else{state.phase='wait';state.delay=.85;}
 updateHud();
}
function refillCups(){if(state.mode!=='practice'||!['aim','over'].includes(state.phase)||state.paused)return;closeDialogs();ball=null;flying.visible=false;defender.reset();glove.visible=false;resetCups();state.reracksUsed=[false,false];state.phase='aim';setPower(0);updateHud();message('FRESH RACK','Practice stats kept.',.9);}
let selectedLayout=null;
function applyRerack(team,id){
 if(state.paused||team!==state.turn||!['aim','cpu'].includes(state.phase)||state.reracksUsed[team])return false;
 const rack=cups.filter(c=>c.team===1-team&&c.alive),points=layoutPoints(id,rack.length);if(!points)return false;
 const spacing=CUP_R*2+.01,back=TABLE_Z-TABLE_L/2+CUP_R+.025;
 rack.forEach((cup,i)=>{const point=points[i],farZ=back+point.depth*spacing;cup.x=point.x*spacing;cup.z=cup.team===1?farZ:2*TABLE_Z-farZ;cup.group.position.set(cup.x,TABLE_Y,cup.z);});
 state.reracksUsed[team]=true;return true;
}
function openRerack(){
 if(!humanReady()||drag||state.reracksUsed[state.turn])return;const count=remaining(1-state.turn),options=layoutsFor(count);if(!options.length)return;
 cancelPull();selectedLayout=options[0].id;$('rerack-copy').textContent=`${count} cups. Choose a formation.`;const container=$('rerack-options');container.replaceChildren();
 for(const option of options){
  const button=document.createElement('button');button.type='button';button.dataset.layout=option.id;button.setAttribute('aria-pressed',String(option.id===selectedLayout));button.setAttribute('aria-label',option.label);
  const svg=document.createElementNS('http://www.w3.org/2000/svg','svg');svg.setAttribute('viewBox','0 0 120 90');svg.setAttribute('aria-hidden','true');
  const points=layoutPoints(option.id,count);
  for(const p of points){const circle=document.createElementNS(svg.namespaceURI,'circle');circle.setAttribute('cx',String(60+p.x*24));circle.setAttribute('cy',String(18+p.depth*24));circle.setAttribute('r','10');circle.setAttribute('fill','#'+COLORS[1-state.turn].toString(16));circle.setAttribute('stroke','#fff1cf');circle.setAttribute('stroke-width','2');svg.append(circle);}
  const label=document.createElement('span');label.textContent=option.label;button.append(svg,label);button.onclick=()=>{selectedLayout=option.id;for(const b of container.children)b.setAttribute('aria-pressed',String(b===button));};container.append(button);
 }
 $('rerack-dialog').showModal();updateHud();
}
function cancelRerack(){$('rerack-dialog').close();selectedLayout=null;updateHud();$('rerack-button').focus();}
$('rerack-button').onclick=openRerack;$('rerack-cancel').onclick=cancelRerack;$('rerack-dialog').oncancel=e=>{e.preventDefault();cancelRerack();};
$('rerack-confirm').onclick=()=>{if(!applyRerack(state.turn,selectedLayout))return;$('rerack-dialog').close();selectedLayout=null;setPower(0);updateHud();$('pause-button').focus();message('RE-RACKED','',1.2);};
function pauseGame(){humanPointer=null;if(state.phase==='menu'||state.paused||document.querySelector('dialog[open]'))return;cancelPull();state.paused=true;drag=null;wrap.classList.remove('drag');$('swipe-feedback').classList.remove('show');$('pause').showModal();updateHud();}
function resumeGame(){state.paused=false;$('pause').close();$('pause-button').focus();updateHud();}
document.querySelectorAll('[data-shot-style]').forEach(button=>button.onclick=()=>{if(!humanReady()||drag||state.mode==='two')return;state.shotStyle=button.dataset.shotStyle;setPower(0);updateHud();});
document.querySelectorAll('[data-mode]').forEach(button=>button.onclick=()=>startMatch(button.dataset.mode));
$('pause-button').onclick=pauseGame;$('resume').onclick=resumeGame;$('pause').oncancel=event=>{event.preventDefault();resumeGame();};
for(const id of ['pause-menu','handoff-menu','result-menu'])$(id).onclick=showMenu;
$('ready').onclick=()=>{$('handoff').close();state.phase='aim';updateHud();};$('handoff').oncancel=event=>event.preventDefault();
$('refill').onclick=refillCups;$('play-again').onclick=()=>state.mode==='practice'?refillCups():startMatch(state.mode);
$('result').oncancel=event=>event.preventDefault();
function openHelp(fromPause=false){helpFromPause=fromPause;if(fromPause)$('pause').close();$('help-rules').open=false;$('help').showModal();}
function closeHelp(){$('help').close();if(helpFromPause){helpFromPause=false;$('pause').showModal();}else $('menu-help').focus();}
$('menu-help').onclick=()=>openHelp();$('pause-help').onclick=()=>openHelp(true);$('help-done').onclick=closeHelp;
$('help').oncancel=event=>{event.preventDefault();closeHelp();};
function humanReady(){return state.phase==='aim'&&!state.paused&&!document.querySelector('dialog[open]');}
function showAim(from,to,pulling=false){
 const rect=wrap.getBoundingClientRect(),svg=$('swipe-feedback');clearTimeout(swipeTimer);svg.setAttribute('viewBox',`0 0 ${rect.width} ${rect.height}`);
 for(const id of ['swipe-shadow','swipe-line'])for(const [key,value]of Object.entries({x1:from.x-rect.left,y1:from.y-rect.top,x2:to.x-rect.left,y2:to.y-rect.top}))$(id).setAttribute(key,value);
 svg.classList.add('show');svg.classList.toggle('pulling',pulling);
 if(!pulling)swipeTimer=setTimeout(()=>svg.classList.remove('show'),2400);
}
function pullAt(point){
 const dx=point.x-drag.anchor.x,dy=Math.max(0,point.y-drag.anchor.y),rect=wrap.getBoundingClientRect();
 const maxPull=Math.max(36,Math.min(150,Math.min(rect.width,rect.height)*.22,rect.bottom-drag.anchor.y-24));
 return {power:clamp((dy-10)/(maxPull-10),0,1),slope:-dx/Math.max(24,dy),dy,dx};
}
function updatePull(point){
 const pull=pullAt(point),origin=ballScreen(),yaw=clamp(Math.atan(pull.slope)*.65,-.35,.35),length=45+pull.power*100;
 setPower(pull.power);showAim(origin,{x:origin.x+Math.sin(yaw)*length,y:origin.y-Math.cos(yaw)*length},true);
 const rect=wrap.getBoundingClientRect();$('pull-grip').setAttribute('cx',point.x-rect.left);$('pull-grip').setAttribute('cy',point.y-rect.top);
 for(const [key,value]of Object.entries({x1:origin.x-rect.left,y1:origin.y-rect.top,x2:point.x-rect.left,y2:point.y-rect.top}))$('pull-tether').setAttribute(key,value);
 $('control-hint').textContent=pull.power>0?'RELEASE · RETURN TO CANCEL':'RETURN TO BALL TO CANCEL';return pull;
}
function cancelPull(){drag=null;wrap.classList.remove('drag');$('swipe-feedback').classList.remove('show','pulling');$('control-hint').textContent='DRAG TO MOVE · PULL TO THROW';if(humanReady())setPower(0);}
function ballScreen(x=releaseX(state.turn)){
 const p=new THREE.Vector3(x,RELEASE_Y,RELEASE_Z[state.turn]).project(camera),rect=wrap.getBoundingClientRect();
 return {x:rect.left+(p.x+1)*rect.width/2,y:rect.top+(1-p.y)*rect.height/2};
}
function resetHumanDefense(){humanSwing=null;humanPointer=null;playerGlove.visible=false;$('swat-target').hidden=true;$('defense-hint').hidden=true;}
function humanDefenseTurn(){return FEATURES.swatting&&state.mode==='cpu'&&state.turn===1&&state.phase==='flight'&&!state.paused&&!document.querySelector('dialog[open]');}
function canHumanSwat(){
 if(!humanDefenseTurn()||ball?.state!=='fly'||ball.bounces!==1||!ball.bounceAllowed||ball.p.z<TABLE_Z-.05)return false;
 // A ball already inside a cup cannot be pulled back out by a late tap.
 return !cups.some(c=>c.alive&&c.enabled&&ball.p.y<RIM_Y+BALL_R&&Math.hypot(ball.p.x-c.x,ball.p.z-c.z)<CUP_R);
}
function incomingScreen(){const p=ball.p.clone().project(camera),r=wrap.getBoundingClientRect();return {x:r.left+(p.x+1)*r.width/2,y:r.top+(1-p.y)*r.height/2};}
function humanHitRadius(){return wrap.clientWidth<=600?44:38;}
function swatBall(b,side){b.state='swatted';b.deadReason='swat';b.swatAge=0;b.v.set(-side*4.5,1.6,b.team===0?-.2:.2);}
function attemptHumanSwat(point){
 if(!canHumanSwat()||humanSwing&&humanSwing.age<.25)return false;
 const center=incomingScreen(),r=wrap.getBoundingClientRect();
 const hit=Math.hypot(point.x-center.x,point.y-center.y)<=humanHitRadius(),side=point.x<r.left+r.width/2?-1:1;
 // Unproject the actual tap at ball depth, so misses animate where the player aimed.
 const ray=new THREE.Raycaster();ray.setFromCamera(new THREE.Vector2((point.x-r.left)/r.width*2-1,1-(point.y-r.top)/r.height*2),camera);
 const impact=ray.ray.intersectPlane(new THREE.Plane(new THREE.Vector3(0,0,1),-ball.p.z),new THREE.Vector3());
 if(!impact)return false;
 humanSwing={age:0,impact,side,hit};
 if(hit){swatBall(ball,side);message('YOU SWATTED!','',.9);}
 return hit;
}
function swipeHumanSwat(from,to){
 if(!canHumanSwat())return false;
 const p=incomingScreen(),dx=to.x-from.x,dy=to.y-from.y,f=clamp(((p.x-from.x)*dx+(p.y-from.y)*dy)/(dx*dx+dy*dy||1),0,1),near={x:from.x+dx*f,y:from.y+dy*f};
 return Math.hypot(near.x-p.x,near.y-p.y)<=humanHitRadius()&&attemptHumanSwat(near);
}
function humanDefenseUi(){
 const defending=humanDefenseTurn()&&ball?.state==='fly',legal=canHumanSwat();
 $('defense-hint').hidden=!defending;$('defense-hint').textContent=legal?'TAP / CLICK THE BALL':ball?.bounces===0?'WAIT FOR THE BOUNCE':'WATCH THE BALL';
 const target=$('swat-target');target.hidden=!legal;
 if(legal){const p=incomingScreen(),r=wrap.getBoundingClientRect();target.style.left=`${p.x-r.left}px`;target.style.top=`${p.y-r.top}px`;target.disabled=!!humanSwing&&humanSwing.age<.25;}
 playerGlove.visible=FEATURES.swatting&&!!humanSwing&&humanSwing.age<.28;
 if(playerGlove.visible){const {age,impact,side}=humanSwing,f=clamp(age/.28,0,1);playerGlove.position.copy(impact).add(new THREE.Vector3(side*(.12-f*.55),f*.12,0));playerGlove.rotation.set(-.15,side===1?0:Math.PI,side*(f-.5)*.35);}
}
$('swat-target').onclick=event=>{if(event.detail===0&&canHumanSwat())attemptHumanSwat(incomingScreen());};
function moveBallTo(clientX){
 const a=ballScreen(-.28),b=ballScreen(.28),worldX=clamp(-.28+(clientX-a.x)/(b.x-a.x)*.56,-.28,.28);
 state.positions[state.turn]=worldX*(state.turn===0?1:-1);held.position.set(worldX,RELEASE_Y,RELEASE_Z[state.turn]);updateHud();setPower(0);
}
wrap.addEventListener('pointerdown',event=>{
 if(event.button===0&&humanDefenseTurn()&&!event.target.closest('a,input,label,dialog,#menu')&&(!event.target.closest('button')||event.target.closest('#swat-target'))){
  const point={x:event.clientX,y:event.clientY};humanPointer={id:event.pointerId,last:point,swung:swipeHumanSwat(point,point)};event.preventDefault();return;
 }
 if(event.button!==0||event.target.closest('button,a,input,label,dialog,#menu')||!humanReady()||drag)return;
 const point={x:event.clientX,y:event.clientY},p=ballScreen();
 if(Math.hypot(point.x-p.x,point.y-p.y)>Math.max(32,Math.abs(ballScreen(releaseX(state.turn)+BALL_R).x-p.x)+16))return;
 $('control-hint').hidden=false;$('swipe-feedback').classList.remove('show');drag={id:event.pointerId,mode:'pending',anchor:point,offset:point.x-p.x};
 wrap.classList.add('drag');$('rerack-button').disabled=true;try{wrap.setPointerCapture(event.pointerId);}catch{}
});
wrap.addEventListener('pointermove',event=>{
 if(humanPointer?.id===event.pointerId&&humanDefenseTurn()&&event.buttons){const point={x:event.clientX,y:event.clientY};if(!humanPointer.swung)humanPointer.swung=swipeHumanSwat(humanPointer.last,point);humanPointer.last=point;return;}
 if(!drag||event.pointerId!==drag.id)return;
 const point={x:event.clientX,y:event.clientY},dx=point.x-drag.anchor.x,dy=point.y-drag.anchor.y;
 if(drag.mode==='pending'&&Math.hypot(dx,dy)>=8){if(dy>0&&dy>=Math.abs(dx)*.55)drag.mode='pull';else if(Math.abs(dx)>Math.abs(dy))drag.mode='position';}
 if(drag.mode==='position'){
  if(dy>12&&dy>Math.abs(dx)*1.3){drag.mode='pull';drag.anchor={...drag.anchor,x:point.x};}
  else{moveBallTo(point.x-drag.offset);if(Math.abs(dx)>Math.abs(dy))drag.anchor=point;return;}
 }
 if(drag.mode==='pull')updatePull(point);
});
wrap.addEventListener('pointerup',event=>{
 if(humanPointer?.id===event.pointerId){if(!humanPointer.swung)attemptHumanSwat({x:event.clientX,y:event.clientY});humanPointer=null;}
 if(!drag||event.pointerId!==drag.id)return;
 const pull=drag.mode==='pull'?pullAt({x:event.clientX,y:event.clientY}):null;
 drag=null;wrap.classList.remove('drag');$('swipe-feedback').classList.remove('pulling');$('control-hint').textContent='DRAG TO MOVE · PULL TO THROW';
 if(!(pull&&pull.power>0&&humanReady()&&launchShot(pull.power,pull.slope))){cancelPull();updateHud();}
});
wrap.addEventListener('pointercancel',()=>{humanPointer=null;cancelPull();updateHud();});
addEventListener('blur',()=>{humanPointer=null;cancelPull();updateHud();});
addEventListener('keydown',event=>{if(document.querySelector('dialog[open]'))return;if(event.code==='Space'&&humanDefenseTurn()&&!event.target.closest('button:not(#swat-target),a')){event.preventDefault();if(!event.repeat&&canHumanSwat())attemptHumanSwat(incomingScreen());}if(event.key==='Escape'){event.preventDefault();pauseGame();}});
const clock=new THREE.Clock();
function frame(){
 const dt=Math.min(clock.getDelta(),.05);
 if(!state.paused){
  if(humanSwing)humanSwing.age+=dt;
  if(messageTime>0){messageTime-=dt;if(messageTime<=0)$('message').classList.remove('show');}
  if(state.phase==='cpu'){state.cpuTime+=dt;if(state.cpuTime>1.1)cpuShot();}
  if(state.phase==='wait'){state.delay-=dt;if(state.delay<=0){if(state.mode!=='practice')state.turn=1-state.turn;beginTurn();}}
  accumulator+=dt;
  while(accumulator>=1/240){
   accumulator-=1/240;if(ball&&ball.state==='fly')stepBall(ball,1/240);
   if(FEATURES.swatting&&state.mode==='cpu'){if(ball?.justBounced){ball.justBounced=false;defender.notice(ball);}
    if(defender.update(1/240,ball)&&ball?.state==='fly'){swatBall(ball,defender.side);message('SWATTED!','',.9);}
   }
  }
  if(ball){
   if(ball.state==='swatted'){ball.swatAge+=dt;ball.v.y-=G*dt;ball.p.addScaledVector(ball.v,dt);if(ball.swatAge>.4)finishShot();}
   else if(ball.state==='sink'){
    ball.sinkAge+=dt;const f=clamp(ball.sinkAge/.3,0,1),cup=ball.hitCup;ball.p.x=THREE.MathUtils.lerp(ball.p.x,cup.x,f);ball.p.z=THREE.MathUtils.lerp(ball.p.z,cup.z,f);ball.p.y=RIM_Y+BALL_R-f*(CUP_H-.025);flying.scale.setScalar(1-f*.4);
    if(f>=1)finishShot(cup);
   }else if(ball.state==='done')finishShot();
   if(ball)flying.position.copy(ball.p);
  }
  for(const p of sparks)if(p.life>0){p.life-=dt;p.v.y-=G*dt;p.mesh.position.addScaledVector(p.v,dt);p.mesh.visible=p.life>0;}
 }
 held.visible=['aim','cpu'].includes(state.phase);if(held.visible)held.position.set(releaseX(state.turn),RELEASE_Y,RELEASE_Z[state.turn]);
 const defense=defender.snapshot();glove.visible=FEATURES.swatting&&state.mode==='cpu'&&['windup','swing','recover'].includes(defense.phase);
 if(glove.visible){glove.position.set(defense.position.x,defense.position.y,defense.position.z);glove.rotation.set(-.15,0,defense.phase==='windup'?-.2:defense.phase==='recover'?.18:0);}
 humanDefenseUi();
 renderer.render(scene,camera);requestAnimationFrame(frame);
}
showMenu();
requestAnimationFrame(frame);
window.__pong={
 get state(){return {...state,remaining:[remaining(0),remaining(1)],ball:ball?{team:ball.team,state:ball.state,p:ball.p.toArray(),v:ball.v.toArray(),rimHits:ball.rimHits,bounces:ball.bounces,style:ball.style,deadReason:ball.deadReason}:null,cups:cups.map(c=>({id:c.id,team:c.team,x:c.x,z:c.z,alive:c.alive,enabled:c.enabled})),held:{p:held.position.toArray(),visible:held.visible},camera:{position:camera.position.toArray(),fov:camera.fov}};},
 startMatch,throwWith:launchShot,refill:refillCups,
 court:{length:TABLE_L,center:TABLE_Z,width:TABLE_W,releaseZ:[...RELEASE_Z]},
 get defense(){return {...defender.snapshot(),visible:glove.visible};},
 get humanDefense(){return {active:canHumanSwat(),swing:humanSwing?{age:humanSwing.age,hit:humanSwing.hit,impact:humanSwing.impact.toArray()}:null,visible:playerGlove.visible};},
 _screen:(x,y,z)=>{const p=new THREE.Vector3(x,y,z).project(camera);return {x:(p.x+1)*wrap.clientWidth/2,y:(1-p.y)*wrap.clientHeight/2};},
 _slopeFor:(id)=>{const c=cups[id],yaw=Math.atan2((c.x-releaseX(state.turn))*(state.turn===0?1:-1),Math.abs(c.z-RELEASE_Z[state.turn]));return Math.tan(yaw/.65);},
 _powerFor:(id)=>{const c=cups[id];return powerForPoint(c.x,c.z);},
 _probe:(power,slope=0,team=0,style)=>{const b=newBall(team,power,slope,style);const start=b.p.clone(),velocity=b.v.clone();for(let i=0;i<1440&&b.state==='fly';i++)stepBall(b,1/240);return {state:b.state,cup:b.hitCup?.id??null,p:b.p.toArray(),v:b.v.toArray(),rimHits:b.rimHits,bounces:b.bounces,deadReason:b.deadReason,start:start.toArray(),velocity:velocity.toArray()};},
 _dropAt:(id)=>{if(!humanReady())return false;const c=cups[id];if(!c.alive||!c.enabled)return false;ball=newBall(state.turn,.4);ball.p.set(c.x,RIM_Y+.25,c.z);ball.v.set(0,-.8,0);state.shots[state.turn]++;state.phase='flight';flying.visible=true;flying.scale.setScalar(1);flying.position.copy(ball.p);updateHud();return true;}
};
