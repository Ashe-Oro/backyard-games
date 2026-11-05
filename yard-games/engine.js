import * as THREE from 'three';
import {games,settleRound,roundPoints} from './rules.mjs';
import {createBackyard} from './backyard.js';
const kind=document.body.dataset.game,cfg=games[kind],$=id=>document.getElementById(id),clamp=(v,a,b)=>Math.max(a,Math.min(b,v));
const G=9.8,ANGLE=43*Math.PI/180,RY=1.05,RZ=.25,TZ=RZ-cfg.distance,COLORS=[0xe85f49,0x4b8cdb],RUNG_Y=[.34,.67,1];
const state={mode:'practice',phase:'menu',turn:0,first:0,round:1,scores:[0,0],counts:[0,0],positions:[0,0],paused:false,winner:null,overtime:false,bonus:false,power:0,stats:{throws:0,points:0},delay:0};
const wrap=$('game'),scene=new THREE.Scene();scene.background=new THREE.Color('#edc5a0');scene.fog=new THREE.Fog('#c4d3a7',20,55);
let renderer;try{renderer=new THREE.WebGLRenderer({antialias:false,powerPreference:'high-performance'});}catch(e){$('error').hidden=false;throw e;}
renderer.setPixelRatio(1);renderer.shadowMap.enabled=true;renderer.shadowMap.type=THREE.BasicShadowMap;wrap.prepend(renderer.domElement);
const camera=new THREE.PerspectiveCamera(36,1,.05,100);scene.add(new THREE.HemisphereLight('#e3eee8','#426139',1.4));
const sun=new THREE.DirectionalLight('#ffd7ad',2);sun.position.set(-4,8,4);sun.castShadow=true;sun.shadow.mapSize.set(1024,1024);Object.assign(sun.shadow.camera,{left:-8,right:8,top:8,bottom:-8,near:.5,far:40});sun.shadow.normalBias=.015;scene.add(sun);
const mat=color=>new THREE.MeshLambertMaterial({color});
function box(w,h,d,m,x,y,z,parent=scene){const mesh=new THREE.Mesh(new THREE.BoxGeometry(w,h,d),m);mesh.position.set(x,y,z);mesh.castShadow=mesh.receiveShadow=true;parent.add(mesh);return mesh;}
function texture(w,h,draw,repeat=1){const c=document.createElement('canvas');c.width=w;c.height=h;draw(c.getContext('2d'),w,h);const t=new THREE.CanvasTexture(c);t.colorSpace=THREE.SRGBColorSpace;t.magFilter=t.minFilter=THREE.NearestFilter;t.generateMipmaps=false;t.wrapS=t.wrapT=THREE.RepeatWrapping;t.repeat.set(repeat,repeat);return t;}
const yardMaterials=createBackyard(scene,{farZ:TZ-3,nearZ:6,propZ:TZ+.5});
function label(text,x,y,z,color='#fff1cf',size=.22){const t=texture(256,32,g=>{g.fillStyle=color;g.font='bold 20px monospace';g.textAlign='center';g.fillText(text,128,24)});const s=new THREE.Sprite(new THREE.SpriteMaterial({map:t,transparent:true,depthTest:false}));s.position.set(x,y,z);s.scale.set(size*8,size,1);scene.add(s);return s;}
const targetGroup=new THREE.Group();scene.add(targetGroup);targetGroup.position.z=TZ;
if(kind==='ladder-golf'){
 const frame=mat('#202e2d'),joint=mat('#85968a');for(const x of [-.36,.36]){box(.035,1.15,.035,frame,x,.575,0,targetGroup);box(.035,.04,.8,frame,x,.025,0,targetGroup);}for(const z of [-.38,.38])box(.76,.04,.035,frame,0,.025,z,targetGroup);
 for(let i=0;i<3;i++){box(.73,.035,.035,mat(['#3c9ee8','#8ed139','#ffe16b'][i]),0,RUNG_Y[i],0,targetGroup);for(const x of [-.36,.36]){box(.055,.065,.055,joint,x,RUNG_Y[i],0,targetGroup);box(.018,.018,.007,frame,x,RUNG_Y[i],.031,targetGroup);}label(String(i+1),.49,RUNG_Y[i],TZ,'#fff1cf',.13);}

}else if(kind==='washers'){
 const plywood=yardMaterials.wood,fastener=mat('#c3ced3');box(.68,.035,.68,mat('#477944'),0,.035,0,targetGroup);for(const x of [-.34,.34])box(.035,.18,.72,plywood,x,.09,0,targetGroup);for(const z of [-.34,.34])box(.68,.18,.035,plywood,0,.09,z,targetGroup);
 for(const x of [-.34,.34])for(const z of [-.34,.34]){box(.055,.2,.055,yardMaterials.woodDark,x,.1,z,targetGroup);box(.018,.018,.008,fastener,x,.14,z+Math.sign(z)*.03,targetGroup);}
 const cup=new THREE.Mesh(new THREE.CylinderGeometry(.105,.105,.2,20,1,true),new THREE.MeshLambertMaterial({color:'#e9e1cc',side:THREE.DoubleSide}));cup.position.y=.12;targetGroup.add(cup);const hole=new THREE.Mesh(new THREE.CircleGeometry(.102,20),mat('#172c32'));hole.rotation.x=-Math.PI/2;hole.position.y=.039;targetGroup.add(hole);label('CUP 3 · BOX 1',0,.65,TZ,'#ffe16b',.14);
}else{
 const ring=new THREE.Mesh(new THREE.TorusGeometry(.46,.022,5,40),mat('#ffe16b'));ring.rotation.x=Math.PI/2;ring.position.y=.023;targetGroup.add(ring);label('3 IN · 1 CLOSER',0,.55,TZ-.15,'#ffe16b',.18);
}
const colorMats=COLORS.map(mat),cream=mat('#fff1cf'),silver=mat('#c3ced3'),ropeMat=new THREE.LineBasicMaterial({color:'#fff1cf'});
const sphereGeo=new THREE.SphereGeometry(.045,8,6),washerGeo=new THREE.TorusGeometry(.041,.012,5,16);
function piece(team){
 const g=new THREE.Group();
 if(kind==='ladder-golf'){
  for(const y of [-.14,.14]){const ball=new THREE.Group();ball.position.y=y;ball.add(new THREE.Mesh(sphereGeo,colorMats[team]));for(const d of [-1,1])box(.012,.018,.008,cream,d*.018,.012,.038,ball);g.add(ball);}
  g.add(new THREE.Line(new THREE.BufferGeometry().setFromPoints([new THREE.Vector3(0,-.14,0),new THREE.Vector3(0,.14,0)]),ropeMat));
 }else if(kind==='washers'){const mesh=new THREE.Mesh(washerGeo,colorMats[team]);mesh.rotation.x=Math.PI/2;g.add(mesh);}
 else{const tip=new THREE.Mesh(new THREE.SphereGeometry(.045,8,6),colorMats[team]);tip.scale.y=.7;g.add(tip);box(.018,.28,.018,silver,0,.15,0,g);for(const y of [.11,.15,.19])box(.022,.012,.022,cream,0,y,0,g);for(const angle of [0,Math.PI/2]){const fin=box(.15,.1,.012,colorMats[team],0,.25,0,g);fin.rotation.y=angle;}}
 g.traverse(o=>{if(o.isMesh)o.castShadow=o.receiveShadow=true});return g;
}
let held=piece(0);scene.add(held);let flying=null,shots=[],drag=null,feedbackTimer,messageLeft=0,accumulator=0;
function rx(){return state.positions[state.turn];}
function screen(x=rx()){const p=new THREE.Vector3(x,RY,RZ).project(camera),r=wrap.getBoundingClientRect();return {x:r.left+(p.x+1)*r.width/2,y:r.top+(1-p.y)*r.height/2};}
function resize(){if(drag)cancel();const w=wrap.clientWidth,h=wrap.clientHeight,portrait=clamp((1.1-w/h)/.6,0,1),scale=Math.max(1,Math.round(Math.min(h,w*.9)/420));renderer.setSize(Math.ceil(w/scale),Math.ceil(h/scale),false);camera.aspect=w/h;camera.position.set(0,2.6+cfg.distance*.09,3.2+Math.max(0,cfg.distance-5)*.45);camera.lookAt(0,.45,TZ*.53);camera.fov=32+11*portrait;camera.updateProjectionMatrix();camera.updateMatrixWorld();$('aim').classList.remove('show');}
addEventListener('resize',resize);resize();
function ready(){return state.phase==='aim'&&!state.paused&&!document.querySelector('dialog[open]');}
function message(big,detail=''){const el=$('message');el.replaceChildren(document.createTextNode(big));if(detail){const small=document.createElement('small');small.textContent=detail;el.append(small)}el.classList.add('show');messageLeft=1.5;}
function disposePiece(g){g.traverse(o=>{if(o.geometry&&o.geometry!==sphereGeo&&o.geometry!==washerGeo)o.geometry.dispose()});}
function resetHeld(){scene.remove(held);disposePiece(held);held=piece(state.turn);scene.add(held);held.position.set(rx(),RY,RZ);}
function clearShots(){for(const s of shots){scene.remove(s.mesh);disposePiece(s.mesh);}shots=[];if(flying){scene.remove(flying.mesh);disposePiece(flying.mesh);}flying=null;}
function closeDialogs(){for(const d of document.querySelectorAll('dialog'))if(d.open)d.close();}
function hud(){
 $('mode-label').textContent=state.mode==='practice'?'SOLO PRACTICE':state.mode==='cpu'?'VS CPU':'2 PLAYER';$('turn-label').textContent=(state.mode==='cpu'?(state.turn?'CPU':'YOUR'):state.turn?'BLUE':'RED')+' THROW';$('round-label').textContent=state.mode==='practice'?cfg.name.toUpperCase():(state.overtime?'OVERTIME · ':'ROUND ')+state.round;
 $('red-score').textContent=state.scores[0];$('blue-score').textContent=state.scores[1];for(const [t,id]of [[0,'red-pips'],[1,'blue-pips']])$(id).textContent=Array.from({length:cfg.count},(_,i)=>i<state.counts[t]?'□':'■').join(' ');
 $('scoreboard').hidden=state.mode==='practice';$('practice-stats').hidden=state.mode!=='practice';$('practice-stats').textContent=`${state.stats.points} POINTS\n${state.stats.throws} THROWS`;$('clear').hidden=state.mode!=='practice';$('clear').disabled=!ready()||!shots.length;
 $('control-hint').hidden=!ready()||!$('shot-feedback').hidden;if(state.phase==='handoff'||state.phase==='cpu')$('shot-feedback').hidden=true;
}
function startMatch(mode='practice',first=0){if(!['practice','cpu','two'].includes(mode))return;closeDialogs();cancel();clearShots();Object.assign(state,{mode,phase:'aim',turn:first,first,round:1,scores:[0,0],counts:[0,0],positions:[0,0],paused:false,winner:null,overtime:false,bonus:$('bonus').checked,power:0,stats:{throws:0,points:0}});if(mode==='practice')state.turn=state.first=0;$('menu').hidden=true;$('hud').hidden=false;$('shot-feedback').hidden=true;$('message').classList.remove('show');beginTurn(true);}
function showMenu(){closeDialogs();cancel();clearShots();state.phase='menu';state.paused=false;$('menu').hidden=false;$('hud').hidden=true;held.visible=false;}
function beginTurn(handoff=false){resetHeld();state.phase=state.mode==='cpu'&&state.turn===1?'cpu':'aim';state.delay=state.phase==='cpu'?1:0;if(state.mode==='two'&&handoff){state.phase='handoff';$('handoff-title').textContent=(state.turn?'BLUE':'RED')+'’S TURN';$('handoff-copy').textContent=kind==='ladder-golf'?'Throw 3 bolas, then pass.':'Throw once, then pass.';$('handoff').showModal();}hud();}
function powerFor(x,y,z=TZ){const d=Math.hypot(x-rx(),z-RZ),dy=y-RY,speed=Math.sqrt(G*d*d/(2*Math.cos(ANGLE)**2*(d*Math.tan(ANGLE)-dy)));return clamp((speed-3)/10,0,1);}
function body(power,slope=0,team=state.turn){const yaw=clamp(Math.atan(slope)*.65,-.24,.24),speed=3+clamp(power,0,1)*10;return {team,power,age:0,p:new THREE.Vector3(state.positions[team],RY,RZ),prev:new THREE.Vector3(),v:new THREE.Vector3(Math.sin(yaw)*Math.cos(ANGLE)*speed,Math.sin(ANGLE)*speed,-Math.cos(yaw)*Math.cos(ANGLE)*speed),rung:-1,value:0,alive:true,inside:false,distance:Infinity,bounces:0,state:'fly',mesh:null};}
function classifyGround(b){const d=Math.hypot(b.p.x,b.p.z-TZ);b.distance=d;if(kind==='lawn-darts'){b.inside=d<=.46;b.value=b.inside?3:0;}b.state='rest';b.p.y=.035;}
function step(b,dt,collisions=true){
 if(b.state!=='fly')return;b.age+=dt;b.prev.copy(b.p);b.v.y-=G*dt;b.p.addScaledVector(b.v,dt);
 if(kind==='ladder-golf'&&b.prev.z>TZ&&b.p.z<=TZ){
  const f=(b.prev.z-TZ)/(b.prev.z-b.p.z),y=THREE.MathUtils.lerp(b.prev.y,b.p.y,f),x=THREE.MathUtils.lerp(b.prev.x,b.p.x,f);
  const rung=RUNG_Y.findIndex(h=>Math.abs(h-y)<.14);
  if(rung>=0&&Math.abs(x)<.34){b.rung=rung;b.value=rung+1;b.state='hanging';b.p.set(clamp(x,-.3,.3),RUNG_Y[rung]-.12,TZ);if(collisions)for(const old of shots)if(old.alive&&old.rung===rung&&Math.abs(old.p.x-x)<.09){old.alive=false;old.rung=-1;old.value=0;old.state='rest';old.p.y=.04;old.p.z+=.15;old.mesh.position.copy(old.p);old.mesh.rotation.z=Math.PI/2;}return;}
 }
 if(kind==='washers'&&b.v.y<0){
  const d=Math.hypot(b.p.x,b.p.z-TZ),inside=Math.abs(b.p.x)<.305&&Math.abs(b.p.z-TZ)<.305;
  if(inside&&b.p.y<=.22&&b.prev.y>.22&&d<.073){b.value=3;b.state='cup';b.p.y=.055;return;}
  if(b.p.y<.22&&b.p.y>.06&&d<.145&&Math.hypot(b.prev.x,b.prev.z-TZ)>=.145){const nx=b.p.x/Math.max(d,.001),nz=(b.p.z-TZ)/Math.max(d,.001),vn=b.v.x*nx+b.v.z*nz;b.p.x=nx*.146;b.p.z=TZ+nz*.146;if(vn<0){b.v.x-=1.4*vn*nx;b.v.z-=1.4*vn*nz;}}
  if(inside&&b.p.y<=.06){b.value=1;b.state='box';b.p.y=.06;return;}
 }
 if(b.p.y<=.045){
  if(kind!=='lawn-darts'&&b.bounces===0&&b.v.y<-.7){b.p.y=.045;b.v.y=-b.v.y*.32;b.v.x*=.72;b.v.z*=.72;b.bounces++;}
  else classifyGround(b);
 }
 if(b.age>6||b.p.z<TZ-12||Math.abs(b.p.x)>8)classifyGround(b);
}
function visual(b){b.mesh.position.copy(b.p);if(kind==='ladder-golf'){
 if(b.state==='hanging'){b.mesh.rotation.set(0,0,Math.sin(performance.now()/700+b.p.x)*.08);const balls=b.mesh.children;balls[0].position.set(-.025,-.05,.04);balls[1].position.set(.025,-.07,-.04);balls[2].geometry.dispose();balls[2].geometry=new THREE.BufferGeometry().setFromPoints([balls[0].position,new THREE.Vector3(0,.12,0),balls[1].position]);}
 else if(b.state==='fly')b.mesh.rotation.z=b.age*11;
 }else if(kind==='washers')b.mesh.rotation.y=b.age*9;else if(b.state==='fly')b.mesh.rotation.x=Math.atan2(-b.v.z,b.v.y)*-.25;else b.mesh.rotation.set(.14,0,.1);
}
function feedback(b){const t=(b.v.y+Math.sqrt(b.v.y*b.v.y+2*G*RY))/G,range=-b.v.z*t;let zone='range';if(kind==='ladder-golf'){const time=cfg.distance/(-b.v.z),height=RY+b.v.y*time-G*time*time/2;zone=height<.2?'weak':height>1.17?'strong':'range';}else zone=range<cfg.distance-.35?'weak':range>cfg.distance+.45?'strong':'range';
 $('shot-feedback').hidden=false;$('feedback-direction').textContent=Math.abs(b.v.x)<.04?'↑ STRAIGHT':b.v.x<0?'↖ LEFT':'↗ RIGHT';$('feedback-power').textContent={weak:'TOO WEAK',strong:'TOO STRONG',range:'POWER IN RANGE'}[zone];$('feedback-tip').textContent={weak:'Pull farther.',strong:'Use a shorter pull.',range:kind==='ladder-golf'?'Rungs in range.':'Target in range.'}[zone];const p=screen(),yaw=Math.atan2(b.v.x,-b.v.z);drawAim(p,{x:p.x+Math.sin(yaw)*110,y:p.y-Math.cos(yaw)*110});}
function throwWith(power,slope=0){if(state.paused||!['aim','cpu'].includes(state.phase)||document.querySelector('dialog[open]'))return false;flying=body(power,slope);flying.mesh=piece(state.turn);scene.add(flying.mesh);flying.mesh.position.copy(flying.p);state.phase='flight';state.power=power;held.visible=false;if(!(state.mode==='cpu'&&state.turn===1))feedback(flying);hud();return true;}
function finish(){const b=flying;flying=null;if(kind==='washers'&&b.value===3){b.p.y+=shots.filter(s=>s.value===3).length*.012;b.mesh.position.y=b.p.y;}shots.push(b);if(b.value>0&&!(state.mode==='cpu'&&b.team===1)){$('feedback-power').textContent='POWER IN RANGE';$('feedback-tip').textContent=kind==='ladder-golf'?'Rung hit.':'Target hit.';}state.counts[b.team]++;if(state.mode==='practice'){state.stats.throws++;state.stats.points+=b.value;}
 const text=kind==='ladder-golf'?(b.rung>=0?`RUNG ${b.rung+1} · ${b.value} POINT${b.value===1?'':'S'}`:'NO WRAP'):kind==='washers'?(b.value===3?'IN THE CUP · 3':b.value===1?'IN THE BOX · 1':'OUTSIDE'):b.inside?'IN THE RING · 3':'OUTSIDE THE RING';message(text,kind==='ladder-golf'?'Hanging bolas score at round end.':state.mode==='practice'&&kind==='lawn-darts'?'Closer darts score in matches.':'');state.phase='wait';state.delay=.85;hud();}
function endRound(){const result=settleRound(kind,state.scores,shots,{bonus:state.bonus,overtime:state.overtime});state.scores=result.scores;state.winner=result.winner;state.overtime=result.overtime;if(result.first!==null)state.first=result.first;state.phase=state.winner===null?'round':'over';$('round-tag').textContent=state.winner===null?`ROUND ${state.round} COMPLETE`:'BACKYARD CHAMPION';$('result-title').textContent=state.winner===null?(state.overtime?'OVERTIME':'ROUND SCORED'):(state.mode==='cpu'?(state.winner===0?'YOU WIN!':'CPU WINS!'):(state.winner?'BLUE':'RED')+' WINS!');$('result-copy').textContent=`Red: ${state.scores[0]} total (${result.points[0]} this round${result.bust[0]?' · bust':''}) · Blue: ${state.scores[1]} total (${result.points[1]} this round${result.bust[1]?' · bust':''})`;$('next').textContent=state.winner===null?'NEXT ROUND':'PLAY AGAIN';$('round-result').showModal();hud();}
function nextTurn(){if(state.mode==='practice'){state.turn=0;beginTurn();return;}if(state.counts.every(n=>n>=cfg.count)){endRound();return;}const previous=state.turn;if(kind==='ladder-golf')state.turn=state.counts[state.first]<cfg.count?state.first:1-state.first;else state.turn=(state.first+state.counts[0]+state.counts[1])%2;beginTurn(previous!==state.turn);}
function cpu(){let x=0,y=kind==='ladder-golf'?RUNG_Y[Math.random()*3|0]:kind==='washers'?.22:.04;if(kind==='lawn-darts')x=(Math.random()-.5)*.45;else x=(Math.random()-.5)*.15;const p=powerFor(x,y)+(Math.random()-.5)*.035;const yaw=Math.atan2(x-rx(),cfg.distance);throwWith(p,Math.tan(yaw/.65));}
function drawAim(from,to,pulling=false){const svg=$('aim'),r=wrap.getBoundingClientRect();clearTimeout(feedbackTimer);svg.setAttribute('viewBox',`0 0 ${r.width} ${r.height}`);for(const id of ['arrow','arrow-shadow'])for(const [key,value]of Object.entries({x1:from.x-r.left,y1:from.y-r.top,x2:to.x-r.left,y2:to.y-r.top}))$(id).setAttribute(key,value);svg.classList.add('show');svg.classList.toggle('pulling',pulling);if(!pulling)feedbackTimer=setTimeout(()=>svg.classList.remove('show'),2500);}
function pullAt(point){const r=wrap.getBoundingClientRect(),dy=Math.max(0,point.y-drag.anchor.y),dx=point.x-drag.anchor.x,max=Math.max(36,Math.min(150,Math.min(r.width,r.height)*.22,r.bottom-drag.anchor.y-24));return {power:clamp((dy-10)/(max-10),0,1),slope:-dx/Math.max(24,dy)};}
function cancel(){drag=null;wrap.classList.remove('drag');$('aim').classList.remove('show','pulling');$('control-hint').textContent='DRAG · PULL · RELEASE';}
function updatePull(point){const p=pullAt(point),origin=screen(),yaw=clamp(Math.atan(p.slope)*.65,-.24,.24),length=45+p.power*100;state.power=p.power;drawAim(origin,{x:origin.x+Math.sin(yaw)*length,y:origin.y-Math.cos(yaw)*length},true);const r=wrap.getBoundingClientRect();$('grip').setAttribute('cx',point.x-r.left);$('grip').setAttribute('cy',point.y-r.top);for(const [key,value]of Object.entries({x1:origin.x-r.left,y1:origin.y-r.top,x2:point.x-r.left,y2:point.y-r.top}))$('tether').setAttribute(key,value);$('control-hint').textContent=p.power>0?'RELEASE · RETURN TO CANCEL':'RETURN TO START TO CANCEL';}
wrap.addEventListener('pointerdown',e=>{if(e.button!==0||drag||!ready()||e.target.closest('button,a,dialog,input,label,#menu'))return;const p=screen(),point={x:e.clientX,y:e.clientY};if(Math.hypot(point.x-p.x,point.y-p.y)>Math.max(34,Math.abs(screen(rx()+.1).x-p.x)))return;$('shot-feedback').hidden=true;$('control-hint').hidden=false;$('aim').classList.remove('show');drag={id:e.pointerId,mode:'pending',anchor:point,offset:point.x-p.x};wrap.classList.add('drag');try{wrap.setPointerCapture(e.pointerId);}catch{}});
wrap.addEventListener('pointermove',e=>{if(!drag||e.pointerId!==drag.id)return;const point={x:e.clientX,y:e.clientY},dx=point.x-drag.anchor.x,dy=point.y-drag.anchor.y;if(drag.mode==='pending'&&Math.hypot(dx,dy)>=8){if(dy>0&&dy>=Math.abs(dx)*.55)drag.mode='pull';else if(Math.abs(dx)>Math.abs(dy))drag.mode='position';}if(drag.mode==='position'){if(dy>12&&dy>Math.abs(dx)*1.3){drag.mode='pull';drag.anchor={...drag.anchor,x:point.x};}else{const a=screen(-.4),b=screen(.4);state.positions[state.turn]=clamp(-.4+(point.x-drag.offset-a.x)/(b.x-a.x)*.8,-.4,.4);held.position.x=rx();if(Math.abs(dx)>Math.abs(dy))drag.anchor=point;return;}}if(drag.mode==='pull')updatePull(point);});
wrap.addEventListener('pointerup',e=>{if(!drag||e.pointerId!==drag.id)return;const p=drag.mode==='pull'?pullAt({x:e.clientX,y:e.clientY}):null;drag=null;wrap.classList.remove('drag');$('aim').classList.remove('pulling');$('control-hint').textContent='DRAG · PULL · RELEASE';if(!(p&&p.power>0&&ready()&&throwWith(p.power,p.slope)))cancel();});
wrap.addEventListener('pointercancel',cancel);addEventListener('blur',cancel);
function pause(){if(state.phase==='menu'||state.paused||document.querySelector('dialog[open]'))return;cancel();state.paused=true;$('pause').showModal();}
function resume(){state.paused=false;$('pause').close();hud();}
$('pause-button').onclick=pause;$('resume').onclick=resume;$('pause').oncancel=e=>{e.preventDefault();resume()};addEventListener('keydown',e=>{if(e.key==='Escape'&&!document.querySelector('dialog[open]')){e.preventDefault();pause()}});
for(const id of ['pause-menu','handoff-menu','result-menu'])$(id).onclick=showMenu;
$('ready').onclick=()=>{$('handoff').close();state.phase='aim';hud();};$('handoff').oncancel=e=>e.preventDefault();$('round-result').oncancel=e=>e.preventDefault();
$('next').onclick=()=>{if(state.winner!==null){startMatch(state.mode);return;}$('round-result').close();clearShots();state.round++;state.counts=[0,0];state.turn=state.first;$('shot-feedback').hidden=true;beginTurn(state.mode==='two');};
$('clear').onclick=()=>{if(state.mode!=='practice'||!ready())return;clearShots();$('shot-feedback').hidden=true;hud();message('FRESH TARGET','Practice totals kept.');};
$('rules-copy').textContent=cfg.rules;$('rules-source').href=cfg.source;$('bonus-option').hidden=kind!=='ladder-golf';
let helpFromPause=false;$('rules-button').onclick=()=>{$('rules').showModal()};$('pause-rules').onclick=()=>{helpFromPause=true;$('pause').close();$('rules').showModal()};
function closeRules(){$('rules').close();if(helpFromPause){helpFromPause=false;$('pause').showModal();}}$('rules-done').onclick=closeRules;$('rules').oncancel=e=>{e.preventDefault();closeRules()};
document.querySelectorAll('[data-mode]').forEach(b=>b.onclick=()=>startMatch(b.dataset.mode));
const clock=new THREE.Clock();
function frame(){const dt=Math.min(clock.getDelta(),.05);if(!state.paused){if(messageLeft>0){messageLeft-=dt;if(messageLeft<=0)$('message').classList.remove('show');}if(state.phase==='cpu'||state.phase==='wait'){state.delay-=dt;if(state.delay<=0){if(state.phase==='cpu')cpu();else nextTurn();}}accumulator+=dt;while(accumulator>=1/240){if(flying)step(flying,1/240);accumulator-=1/240;}if(flying){visual(flying);if(flying.state!=='fly')finish();}for(const s of shots)if(s.state==='hanging')s.mesh.rotation.z=Math.sin(performance.now()/700+s.p.x)*.08;}
 held.visible=['aim','cpu'].includes(state.phase);if(held.visible){held.position.set(rx(),RY,RZ);held.rotation.z=kind==='ladder-golf'?.25:0;}renderer.render(scene,camera);requestAnimationFrame(frame);}
showMenu();try{const key=`tailgate-${kind}-help-seen`;if(localStorage.getItem(key)!=='1'){localStorage.setItem(key,'1');$('rules').showModal();}}catch{$('rules').showModal();}requestAnimationFrame(frame);
window.__yard={get state(){return {...state,scores:[...state.scores],counts:[...state.counts],positions:[...state.positions],stats:{...state.stats},shots:shots.map(({team,rung,value,alive,inside,distance,state,p})=>({team,rung,value,alive,inside,distance,state,p:p.toArray()})),flying:flying?{p:flying.p.toArray(),v:flying.v.toArray(),state:flying.state}:null,held:held.position.toArray()};},startMatch,throwWith,screen:()=>screen(),powerFor:(y=.05,x=0)=>powerFor(x,y),_probe:(power,slope=0)=>{const b=body(power,slope);for(let i=0;i<1920&&b.state==='fly';i++)step(b,1/240,false);return {rung:b.rung,value:b.value,state:b.state,p:b.p.toArray(),inside:b.inside,bounces:b.bounces,distance:b.distance};},_rules:{settleRound,roundPoints},_setScores:(a,b,overtime=false)=>{state.scores=[a,b];state.overtime=overtime;hud();}};
