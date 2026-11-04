import * as THREE from 'three';

const $=id=>document.getElementById(id);
const clamp=(n,a,b)=>Math.max(a,Math.min(b,n));
const G=9.8, BALL_R=.02, TABLE_Y=.76, TABLE_W=1.16, TABLE_L=2.8, TABLE_Z=-1.2;
const CUP_R=.078, CUP_H=.17, RIM_Y=TABLE_Y+CUP_H, HOLE_R=CUP_R-BALL_R-.006;
const RELEASE_Y=1.09, RELEASE_Z=[.55,-2.95], ANGLE=52*Math.PI/180;
const COLORS=[0xe75c46,0x448fc4], LABELS=['RED','BLUE'];
const state={mode:'practice',phase:'menu',turn:0,paused:false,shots:[0,0],hits:[0,0],positions:[0,0],winner:null,power:0,delay:0,cpuTime:0};
const releaseX=team=>state.positions[team]*(team===0?1:-1);
let ball=null,drag=null,viewTeam=0,accumulator=0,messageTime=0,swipeTimer;
const wrap=$('game'), scene=new THREE.Scene();
scene.background=new THREE.Color('#edc5a0');scene.fog=new THREE.Fog('#c9d6ac',12,35);
let renderer;
try{renderer=new THREE.WebGLRenderer({antialias:false,powerPreference:'high-performance'});}catch(error){$('error').hidden=false;throw error;}
renderer.setPixelRatio(1);renderer.shadowMap.enabled=true;renderer.shadowMap.type=THREE.BasicShadowMap;
wrap.prepend(renderer.domElement);
const camera=new THREE.PerspectiveCamera(40,1,.05,80);
scene.add(new THREE.HemisphereLight('#fff0cc','#315c43',2));
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
const grass=texture(64,64,(g,w,h)=>{g.fillStyle='#5b934b';g.fillRect(0,0,w,h);for(let i=0;i<480;i++){g.fillStyle=['#659e50','#76aa58','#4a8446','#83b660'][i%4];g.fillRect(Math.random()*w|0,Math.random()*h|0,2+Math.random()*7|0,1+Math.random()*3|0);}},24);
const lawn=new THREE.Mesh(new THREE.PlaneGeometry(50,50),new THREE.MeshLambertMaterial({map:grass}));lawn.rotation.x=-Math.PI/2;lawn.receiveShadow=true;scene.add(lawn);
const wood=mat('#ac7a43'),woodDark=mat('#795432');
for(const z of [-7,5]){
 for(let i=-25;i<=25;i++){const x=i*.27;box(.23,1.35,.08,i%3?wood:woodDark,x,.675,z);}
 box(14,.1,.12,woodDark,0,.38,z);box(14,.1,.12,woodDark,0,1.02,z);
}
const leafMats=['#376445','#44794a','#598c50'].map(mat);
for(const [x,z,s] of [[-3,-4,1.2],[3.2,-4.8,1.5],[-4,2,1.4],[3.8,3,1.1],[-2.4,-6,1],[4,-1,1]]){
 box(.25,2.6,.25,woodDark,x,1.3,z);
 for(let i=0;i<3;i++)box(s*(1.3-i*.12),s*.75,s*(1.2-i*.1),leafMats[i],x+(i-1)*s*.2,2.3+i*.3,z);
 box(s,.5,s*.7,leafMats[1],x,.25,z+.6);
}
for(const z of [-4.2,3.5]){
 const points=[];
 for(let i=0;i<=32;i++){const x=-5+i*10/32;points.push(new THREE.Vector3(x,1.95-.28*(1-(x/5)**2),z));}
 scene.add(new THREE.Line(new THREE.BufferGeometry().setFromPoints(points),new THREE.LineBasicMaterial({color:'#293e3b'})));
 for(let i=1;i<16;i++){const x=-5+i*10/16,y=1.95-.28*(1-(x/5)**2);box(.07,.11,.07,new THREE.MeshBasicMaterial({color:['#ffe486','#ff9d7e','#ace4d0'][i%3]}),x,y-.07,z);}
}
// One readable playing surface. Its markings stay on the table, never on the lawn.
const tableMap=texture(128,256,(g,w,h)=>{
 g.fillStyle='#286d62';g.fillRect(0,0,w,h);
 for(let i=0;i<500;i++){g.fillStyle=i%2?'#2b7568':'#25675e';g.fillRect(Math.random()*w|0,Math.random()*h|0,2,3);}
 g.strokeStyle='#dfd9b6';g.lineWidth=2;g.strokeRect(4,4,w-8,h-8);g.fillRect(w/2-1,4,2,h-8);
 g.fillStyle='#dfd9b6';g.fillRect(4,h/2-1,w-8,2);
 g.save();g.translate(w/2,h/2);g.rotate(-Math.PI/2);g.fillStyle='#edce6f';g.font='bold 11px monospace';g.textAlign='center';g.fillText('TAILGATE GAMES',0,-8);g.restore();
});
const tableSide=mat('#b3a06d'),tableTop=new THREE.MeshLambertMaterial({map:tableMap});
box(TABLE_W,.075,TABLE_L,[tableSide,tableSide,tableTop,tableSide,tableSide,tableSide],0,TABLE_Y-.0375,TABLE_Z);
for(const x of [-.43,.43])for(const z of [-2.22,-.18])box(.055,TABLE_Y-.08,.055,mat('#24413f'),x,(TABLE_Y-.08)/2,z);
box(.9,.045,.055,woodDark,0,.25,-2.22);box(.9,.045,.055,woodDark,0,.25,-.18);
// Nearby party props establish scale without crowding the cup rack.
box(.55,.38,.38,mat('#356f8a'),1.6,.19,-1.5);box(.59,.07,.42,mat('#e4d9b4'),1.6,.415,-1.5);
for(let i=0;i<3;i++)box(.45,.22,.33,mat('#b19a54'),-1.7,.11+i*.22,-2.5);

const cupOuter=new THREE.CylinderGeometry(CUP_R,.052,CUP_H,12,1,true);
const cupInner=new THREE.CylinderGeometry(CUP_R-.006,.046,CUP_H-.012,12,1,true);
const rimGeo=new THREE.TorusGeometry(CUP_R-.002,.006,4,16);
const rimMat=mat('#fff1cf'),innerMat=new THREE.MeshLambertMaterial({color:'#eee1c2',side:THREE.BackSide});
const cups=[];
for(let team=0;team<2;team++)for(let row=0;row<4;row++)for(let col=0;col<=row;col++){
 const x=(col-row/2)*.18,farZ=-1.75-row*.17,z=team===1?farZ:2*TABLE_Z-farZ;
 const group=new THREE.Group();group.position.set(x,TABLE_Y,z);scene.add(group);
 const outer=new THREE.Mesh(cupOuter,mat(COLORS[team]));outer.position.y=CUP_H/2;outer.castShadow=outer.receiveShadow=true;group.add(outer);
 const inner=new THREE.Mesh(cupInner,innerMat);inner.position.y=CUP_H/2+.004;group.add(inner);
 const rim=new THREE.Mesh(rimGeo,rimMat);rim.rotation.x=Math.PI/2;rim.position.y=CUP_H;group.add(rim);
 const liquid=new THREE.Mesh(new THREE.CircleGeometry(.052,12),mat('#bc7d3e'));liquid.rotation.x=-Math.PI/2;liquid.position.y=.045;group.add(liquid);
 cups.push({id:cups.length,team,x,z,alive:true,enabled:true,group});
}
const ballGeo=new THREE.SphereGeometry(BALL_R,10,8),ballMat=mat('#fff5d9');
const held=new THREE.Mesh(ballGeo,ballMat);held.castShadow=held.receiveShadow=true;scene.add(held);
const flying=new THREE.Mesh(ballGeo,ballMat);flying.castShadow=flying.receiveShadow=true;flying.visible=false;scene.add(flying);
const sparkGeo=new THREE.BoxGeometry(.015,.015,.015),sparks=[];
for(let i=0;i<24;i++){const mesh=new THREE.Mesh(sparkGeo,new THREE.MeshBasicMaterial({color:i%2?'#ffe16b':'#fff1cf'}));mesh.visible=false;scene.add(mesh);sparks.push({mesh,v:new THREE.Vector3(),life:0});}
function celebrate(cup){for(const p of sparks){p.mesh.position.set(cup.x,RIM_Y,cup.z);p.v.set((Math.random()-.5)*.7,.5+Math.random()*.6,(Math.random()-.5)*.7);p.life=.65;p.mesh.visible=true;}}
function remaining(team){return cups.filter(c=>c.team===team&&c.alive).length;}
function resetCups(){for(const c of cups){c.alive=true;c.enabled=state.mode!=='practice'||c.team===1;c.group.visible=c.enabled;c.group.scale.setScalar(1);}for(const s of sparks){s.life=0;s.mesh.visible=false;}}
function powerForPoint(x,z,team=state.turn){const d=Math.hypot(x-releaseX(team),z-RELEASE_Z[team]),dy=RIM_Y+BALL_R-RELEASE_Y;
 const speed=Math.sqrt(G*d*d/(2*Math.cos(ANGLE)**2*(d*Math.tan(ANGLE)-dy)));return clamp((speed-3.2)/3.4,0,1);}
function targetWindow(){const rack=cups.filter(c=>c.team===1-state.turn&&c.alive);const powers=rack.map(c=>powerForPoint(c.x,c.z));
 return powers.length?{lo:Math.min(...powers)-.012,hi:Math.max(...powers)+.012}:{lo:.4,hi:.6};}
function newBall(team,power,slope=0){const direction=team===0?-1:1,screenRight=team===0?1:-1,yaw=clamp(Math.atan(slope)*.65,-.35,.35),speed=3.2+clamp(power,0,1)*3.4;
 return {team,power,state:'fly',p:new THREE.Vector3(releaseX(team),RELEASE_Y,RELEASE_Z[team]),prev:new THREE.Vector3(),v:new THREE.Vector3(Math.sin(yaw)*Math.cos(ANGLE)*speed*screenRight,Math.sin(ANGLE)*speed,Math.cos(yaw)*Math.cos(ANGLE)*speed*direction),age:0,sinkAge:0,hitCup:null,bounces:0,deadReason:null,rimHits:0};}
function stepBall(b,dt){
 if(b.state!=='fly')return;b.age+=dt;b.prev.copy(b.p);b.v.y-=G*dt;b.p.addScaledVector(b.v,dt);
 for(const c of cups){
  if(!c.alive||!c.enabled)continue;
  if(b.v.y<0&&b.prev.y>=RIM_Y+BALL_R&&b.p.y<=RIM_Y+BALL_R){
   const fraction=(b.prev.y-RIM_Y-BALL_R)/(b.prev.y-b.p.y),x=THREE.MathUtils.lerp(b.prev.x,b.p.x,fraction),z=THREE.MathUtils.lerp(b.prev.z,b.p.z,fraction);
   const dx=x-c.x,dz=z-c.z,d=Math.hypot(dx,dz);
   if(d<HOLE_R){b.state='sink';b.hitCup=c;b.p.set(x,RIM_Y+BALL_R,z);return;}
   if(d<CUP_R+BALL_R+.006){
    const n=new THREE.Vector3(dx/Math.max(d,.001)*(d-CUP_R),BALL_R,dz/Math.max(d,.001)*(d-CUP_R)).normalize();
    const vn=b.v.dot(n);if(vn<0)b.v.addScaledVector(n,-1.65*vn);
    b.p.set(x,RIM_Y+BALL_R+.004,z);b.rimHits++;return;
   }
  }
  const dx=b.p.x-c.x,dz=b.p.z-c.z,d=Math.hypot(dx,dz);
  if(b.p.y>TABLE_Y+BALL_R&&b.p.y<RIM_Y&&d<CUP_R+BALL_R&&d>HOLE_R){
   const nx=dx/Math.max(d,.001),nz=dz/Math.max(d,.001),vn=b.v.x*nx+b.v.z*nz;
   b.p.x=c.x+nx*(CUP_R+BALL_R+.002);b.p.z=c.z+nz*(CUP_R+BALL_R+.002);
   if(vn<0){b.v.x-=1.5*vn*nx;b.v.z-=1.5*vn*nz;}b.rimHits++;
  }
 }
 const onTable=Math.abs(b.p.x)<TABLE_W/2+BALL_R&&Math.abs(b.p.z-TABLE_Z)<TABLE_L/2+BALL_R;
 if(onTable&&b.p.y<=TABLE_Y+BALL_R&&b.prev.y>=TABLE_Y+BALL_R){b.p.y=TABLE_Y+BALL_R;b.v.set(0,0,0);b.deadReason='table';b.state='done';return;}
 if(b.p.y<=BALL_R){b.p.y=BALL_R;b.v.set(0,0,0);b.deadReason='ground';b.state='done';return;}
 if(b.age>5||Math.abs(b.p.x)>7||b.p.z>8||b.p.z<-10)b.state='done';
}
let meterDirty=true;
function setCamera(){
 const w=wrap.clientWidth,h=wrap.clientHeight,portrait=clamp((1.05-w/h)/.55,0,1);
 viewTeam=state.mode==='two'?state.turn:0;
 camera.position.set(0,2,viewTeam===0?2.2:2*TABLE_Z-2.2);camera.lookAt(0,.75+.15*portrait,TABLE_Z);
 camera.aspect=w/h;camera.fov=38+12*portrait;camera.updateProjectionMatrix();camera.updateMatrixWorld();meterDirty=true;
}
function resize(){if(drag)cancelPull();const w=wrap.clientWidth,h=wrap.clientHeight,scale=Math.max(1,Math.round(Math.min(h,w*.9)/430));renderer.setSize(Math.ceil(w/scale),Math.ceil(h/scale),false);setCamera();$('swipe-feedback').classList.remove('show');}
addEventListener('resize',resize);resize();
function positionMeter(){
 const meter=$('meter'),right=viewTeam===0?1:-1,w=wrap.clientWidth,h=wrap.clientHeight;
 const p=new THREE.Vector3(releaseX(state.turn)+right*.045,RELEASE_Y,RELEASE_Z[state.turn]).project(camera);
 let left=(p.x+1)*w/2+14;const flip=left>w-meter.offsetWidth-64;
 if(flip){const edge=new THREE.Vector3(releaseX(state.turn)-right*.045,RELEASE_Y,RELEASE_Z[state.turn]).project(camera);left=(edge.x+1)*w/2-14-meter.offsetWidth;}
 meter.classList.toggle('left-side',flip);meter.style.left=`${clamp(left,14,w-meter.offsetWidth-14)}px`;
 meter.style.top=`${clamp((1-p.y)*h/2,meter.offsetHeight/2+20,h-meter.offsetHeight/2-34)}px`;meterDirty=false;
}
function setPower(power){state.power=clamp(power,0,1);const w=targetWindow(),meter=$('meter');
 $('meter-fill').style.height=`${state.power*100}%`;$('meter-marker').style.bottom=`${state.power*100}%`;
 meter.dataset.zone=state.power>w.hi?'high':state.power>=w.lo?'target':'low';meter.setAttribute('aria-valuenow',String(Math.round(state.power*100)));
}
function message(text,detail='',seconds=1.1){$('message').replaceChildren(document.createTextNode(text));if(detail){const small=document.createElement('small');small.textContent=detail;$('message').append(small);}$('message').classList.add('show');messageTime=seconds;}
function updateHud(){
 $('mode-name').textContent={practice:'SOLO PRACTICE',cpu:'VS CPU',two:'2 PLAYER'}[state.mode];
 $('turn-label').textContent=state.mode==='practice'?`${remaining(1)} CUPS LEFT`:state.mode==='cpu'?(state.turn===0?'YOUR SHOT':'CPU’S SHOT'):`${LABELS[state.turn]}’S SHOT`;
 $('red-label').textContent=state.mode==='cpu'?'YOUR CUPS':'RED CUPS';$('blue-label').textContent=state.mode==='cpu'?'CPU CUPS':'BLUE CUPS';
 for(const [team,id]of [[0,'red'],[1,'blue']]){$(`${id}-count`).textContent=remaining(team);$(`${id}-pips`).innerHTML=Array.from({length:10},(_,i)=>`<i class="${i>=remaining(team)?'out':''}"></i>`).join('');}
 $('scoreboard').hidden=state.mode==='practice';$('practice-stats').hidden=state.mode!=='practice';$('practice-hits').textContent=`${state.hits[0]} / ${state.shots[0]}`;
 $('control-hint').hidden=state.phase!=='aim';$('shot-count').textContent=`${state.mode==='practice'?'':LABELS[state.turn]+' · '}THROW ${state.shots[state.turn]+(['aim','cpu','handoff'].includes(state.phase)?1:0)}`;
 $('refill').hidden=state.mode!=='practice';$('refill').disabled=state.phase!=='aim'||state.paused;
 $('meter').hidden=state.mode==='cpu'&&state.turn===1||state.phase==='over'||state.phase==='handoff';
 const w=targetWindow();$('meter-target').style.bottom=`${w.lo*100}%`;$('meter-target').style.height=`${(w.hi-w.lo)*100}%`;meterDirty=true;
}
function closeDialogs(){for(const dialog of document.querySelectorAll('dialog'))if(dialog.open)dialog.close();}
function showMenu(){closeDialogs();state.phase='menu';state.paused=false;ball=null;drag=null;wrap.classList.remove('drag');state.turn=0;state.delay=0;$('menu').hidden=false;$('hud').hidden=true;flying.visible=held.visible=false;$('swipe-feedback').classList.remove('show');$('message').classList.remove('show');state.mode='cpu';resetCups();setCamera();}
function startMatch(mode='practice'){
 if(!['practice','cpu','two'].includes(mode))return;closeDialogs();state.mode=mode;state.phase='wait';state.paused=false;state.turn=0;state.shots=[0,0];state.hits=[0,0];state.positions=[0,0];state.winner=null;state.power=0;state.delay=0;ball=null;drag=null;wrap.classList.remove('drag');flying.visible=false;
 $('menu').hidden=true;$('hud').hidden=false;$('message').classList.remove('show');$('swipe-feedback').classList.remove('show');resetCups();setCamera();beginTurn();
}
function beginTurn(){
 ball=null;flying.visible=false;messageTime=0;$('message').classList.remove('show');setCamera();setPower(0);
 if(state.mode==='two'){
  state.phase='handoff';$('handoff-title').textContent=`${LABELS[state.turn]}’S SHOT`;$('handoff-copy').textContent=`Aim at the ${state.turn===0?'blue':'red'} cups. One throw, then pass the device.`;$('handoff').showModal();
 }else if(state.mode==='cpu'&&state.turn===1){state.phase='cpu';state.cpuTime=0;}
 else state.phase='aim';updateHud();
}
function launchShot(power,slope=0){
 if(state.paused||!['aim','cpu'].includes(state.phase))return false;
 ball=newBall(state.turn,power,slope);state.phase='flight';state.shots[state.turn]++;flying.position.copy(ball.p);flying.scale.setScalar(1);flying.visible=true;held.visible=false;setPower(power);updateHud();return true;
}
function cpuShot(){
 const rack=cups.filter(c=>c.team===0&&c.alive),cup=rack[Math.floor(Math.random()*rack.length)];
 if(!cup)return;const precise=Math.random()<.32,jitter=precise?.025:.30,x=cup.x+(Math.random()-.5)*jitter*2,z=cup.z+(Math.random()-.5)*(precise?.035:.24);
 const d=Math.abs(z-RELEASE_Z[1]),yaw=Math.atan2(-(x-releaseX(1)),d),slope=Math.tan(yaw/.65);
 launchShot(powerForPoint(x,z,1),slope);
}
function finishShot(scoredCup=null){
 const team=state.turn;
 if(scoredCup){scoredCup.alive=false;scoredCup.group.visible=false;celebrate(scoredCup);if(scoredCup.team!==team)state.hits[team]++;message(scoredCup.team===team?'OWN CUP!':'CUP!',scoredCup.team===team?'That one came back to bite.':`${remaining(scoredCup.team)} cups left.`,1.2);}
 else if(ball?.deadReason==='table')message('DEAD BALL','Touched the table.',.95);
 else message('NO CUP',ball?.rimHits?'Off the cup. Try a different angle.':'Adjust your pull distance or angle.',.95);
 ball=null;flying.visible=false;
 if(state.mode==='practice'&&remaining(1)===0){state.phase='over';$('result-tag').textContent='RACK CLEARED';$('result-title').textContent='Ten down.';$('result-copy').textContent=`${state.hits[0]} cups in ${state.shots[0]} throws. Refill and keep your practice going.`;$('play-again').textContent='NEW RACK';$('result').showModal();}
 else if(state.mode!=='practice'&&(remaining(0)===0||remaining(1)===0)){
  state.winner=remaining(1)===0?0:1;state.phase='over';$('result-tag').textContent='TABLE CHAMPION';$('result-title').textContent=state.mode==='cpu'?(state.winner===0?'YOU WIN!':'CPU WINS'):LABELS[state.winner]+' WINS!';$('result-copy').textContent=`${state.shots[state.winner]} throws. All ten cups cleared.`;$('play-again').textContent='PLAY AGAIN';$('result').showModal();
 }else{state.phase='wait';state.delay=.85;}
 updateHud();
}
function refillCups(){if(state.mode!=='practice'||!['aim','over'].includes(state.phase)||state.paused)return;closeDialogs();ball=null;flying.visible=false;resetCups();state.phase='aim';setPower(0);updateHud();message('FRESH RACK','Practice stats kept.',.9);}
function pauseGame(){if(state.phase==='menu'||state.paused||document.querySelector('dialog[open]'))return;cancelPull();state.paused=true;drag=null;wrap.classList.remove('drag');$('swipe-feedback').classList.remove('show');$('pause').showModal();updateHud();}
function resumeGame(){state.paused=false;$('pause').close();$('pause-button').focus();updateHud();}
document.querySelectorAll('[data-mode]').forEach(button=>button.onclick=()=>startMatch(button.dataset.mode));
$('pause-button').onclick=pauseGame;$('resume').onclick=resumeGame;$('pause').oncancel=event=>{event.preventDefault();resumeGame();};
for(const id of ['pause-menu','handoff-menu','result-menu'])$(id).onclick=showMenu;
$('ready').onclick=()=>{$('handoff').close();state.phase='aim';updateHud();};$('handoff').oncancel=event=>event.preventDefault();
$('refill').onclick=refillCups;$('play-again').onclick=()=>state.mode==='practice'?refillCups():startMatch(state.mode);
$('result').oncancel=event=>event.preventDefault();$('help-done').onclick=()=>$('help').close();
function humanReady(){return state.phase==='aim'&&!state.paused&&!document.querySelector('dialog[open]');}
function showAim(from,to,pulling=false){
 const rect=wrap.getBoundingClientRect(),svg=$('swipe-feedback');clearTimeout(swipeTimer);svg.setAttribute('viewBox',`0 0 ${rect.width} ${rect.height}`);
 for(const id of ['swipe-shadow','swipe-line'])for(const [key,value]of Object.entries({x1:from.x-rect.left,y1:from.y-rect.top,x2:to.x-rect.left,y2:to.y-rect.top}))$(id).setAttribute(key,value);
 svg.classList.add('show');svg.classList.toggle('pulling',pulling);
 if(!pulling)swipeTimer=setTimeout(()=>svg.classList.remove('show'),900);
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
 $('control-hint').textContent=pull.power>0?`${Math.round(pull.power*100)}% POWER · RELEASE TO THROW`:'RETURN TO BALL TO CANCEL';return pull;
}
function cancelPull(){drag=null;wrap.classList.remove('drag');$('swipe-feedback').classList.remove('show','pulling');$('control-hint').textContent='DRAG SIDEWAYS TO MOVE · PULL DOWN TO THROW';if(humanReady())setPower(0);}
function ballScreen(x=releaseX(state.turn)){
 const p=new THREE.Vector3(x,RELEASE_Y,RELEASE_Z[state.turn]).project(camera),rect=wrap.getBoundingClientRect();
 return {x:rect.left+(p.x+1)*rect.width/2,y:rect.top+(1-p.y)*rect.height/2};
}
function moveBallTo(clientX){
 const a=ballScreen(-.28),b=ballScreen(.28),worldX=clamp(-.28+(clientX-a.x)/(b.x-a.x)*.56,-.28,.28);
 state.positions[state.turn]=worldX*(state.turn===0?1:-1);held.position.set(worldX,RELEASE_Y,RELEASE_Z[state.turn]);updateHud();setPower(0);
}
wrap.addEventListener('pointerdown',event=>{
 if(event.button!==0||event.target.closest('button,a,input,label,dialog,#menu')||!humanReady()||drag)return;
 const point={x:event.clientX,y:event.clientY},p=ballScreen();
 if(Math.hypot(point.x-p.x,point.y-p.y)>Math.max(32,Math.abs(ballScreen(releaseX(state.turn)+BALL_R).x-p.x)+16))return;
 $('swipe-feedback').classList.remove('show');drag={id:event.pointerId,mode:'pending',anchor:point,offset:point.x-p.x};
 wrap.classList.add('drag');try{wrap.setPointerCapture(event.pointerId);}catch{}
});
wrap.addEventListener('pointermove',event=>{
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
 if(!drag||event.pointerId!==drag.id)return;
 const pull=drag.mode==='pull'?pullAt({x:event.clientX,y:event.clientY}):null,origin=ballScreen();
 drag=null;wrap.classList.remove('drag');$('swipe-feedback').classList.remove('pulling');$('control-hint').textContent='DRAG SIDEWAYS TO MOVE · PULL DOWN TO THROW';
 if(pull&&pull.power>0&&humanReady()&&launchShot(pull.power,pull.slope)){
  const yaw=clamp(Math.atan(pull.slope)*.65,-.35,.35);showAim(origin,{x:origin.x+Math.sin(yaw)*100,y:origin.y-Math.cos(yaw)*100});
 }else cancelPull();
});
wrap.addEventListener('pointercancel',cancelPull);
addEventListener('blur',cancelPull);
addEventListener('keydown',event=>{if(document.querySelector('dialog[open]'))return;if(event.key==='Escape'){event.preventDefault();pauseGame();}});
const clock=new THREE.Clock();
function frame(){
 const dt=Math.min(clock.getDelta(),.05);
 if(!state.paused){
  if(messageTime>0){messageTime-=dt;if(messageTime<=0)$('message').classList.remove('show');}
  if(state.phase==='cpu'){state.cpuTime+=dt;if(state.cpuTime>1.1)cpuShot();}
  if(state.phase==='wait'){state.delay-=dt;if(state.delay<=0){if(state.mode!=='practice')state.turn=1-state.turn;beginTurn();}}
  accumulator+=dt;
  while(accumulator>=1/240){accumulator-=1/240;if(ball&&ball.state==='fly')stepBall(ball,1/240);}
  if(ball){
   if(ball.state==='sink'){
    ball.sinkAge+=dt;const f=clamp(ball.sinkAge/.3,0,1),cup=ball.hitCup;ball.p.x=THREE.MathUtils.lerp(ball.p.x,cup.x,f);ball.p.z=THREE.MathUtils.lerp(ball.p.z,cup.z,f);ball.p.y=RIM_Y+BALL_R-f*(CUP_H-.025);flying.scale.setScalar(1-f*.4);
    if(f>=1)finishShot(cup);
   }else if(ball.state==='done')finishShot();
   if(ball)flying.position.copy(ball.p);
  }
  for(const p of sparks)if(p.life>0){p.life-=dt;p.v.y-=G*dt;p.mesh.position.addScaledVector(p.v,dt);p.mesh.visible=p.life>0;}
 }
 held.visible=['aim','cpu'].includes(state.phase);if(held.visible)held.position.set(releaseX(state.turn),RELEASE_Y,RELEASE_Z[state.turn]);
 if(meterDirty&&state.phase!=='menu')positionMeter();renderer.render(scene,camera);requestAnimationFrame(frame);
}
showMenu();
try{if(localStorage.getItem('tailgate-beer-pong-help-seen')!=='1'){localStorage.setItem('tailgate-beer-pong-help-seen','1');$('help').showModal();}}catch{$('help').showModal();}
requestAnimationFrame(frame);
window.__pong={
 get state(){return {...state,remaining:[remaining(0),remaining(1)],ball:ball?{team:ball.team,state:ball.state,p:ball.p.toArray(),v:ball.v.toArray(),rimHits:ball.rimHits,bounces:ball.bounces}:null,cups:cups.map(c=>({id:c.id,team:c.team,x:c.x,z:c.z,alive:c.alive,enabled:c.enabled})),held:{p:held.position.toArray(),visible:held.visible},camera:{position:camera.position.toArray(),fov:camera.fov}};},
 startMatch,throwWith:launchShot,refill:refillCups,
 _screen:(x,y,z)=>{const p=new THREE.Vector3(x,y,z).project(camera);return {x:(p.x+1)*wrap.clientWidth/2,y:(1-p.y)*wrap.clientHeight/2};},
 _powerFor:(id)=>{const c=cups[id];return powerForPoint(c.x,c.z);},
 _probe:(power,slope=0,team=0)=>{const b=newBall(team,power,slope);for(let i=0;i<1440&&b.state==='fly';i++)stepBall(b,1/240);return {state:b.state,cup:b.hitCup?.id??null,p:b.p.toArray(),v:b.v.toArray(),rimHits:b.rimHits,bounces:b.bounces,deadReason:b.deadReason};},
 _dropAt:(id)=>{if(!humanReady())return false;const c=cups[id];if(!c.alive||!c.enabled)return false;ball=newBall(state.turn,.4);ball.p.set(c.x,RIM_Y+.25,c.z);ball.v.set(0,-.8,0);state.shots[state.turn]++;state.phase='flight';flying.visible=true;flying.scale.setScalar(1);flying.position.copy(ball.p);updateHud();return true;}
};
