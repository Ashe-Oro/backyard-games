import * as THREE from 'three';

// Shared, deterministic scenery. Decoration never participates in shot physics.
export function createBackyard(scene,{farZ=-7,nearZ=5,propZ=-1.5}={}){
 let seed=71293;
 const random=()=>((seed=(seed*1664525+1013904223)>>>0)/4294967296);
 const pick=values=>values[Math.floor(random()*values.length)];
 const material=color=>new THREE.MeshLambertMaterial({color});
 const buckets=new Map(),unit=new THREE.BoxGeometry(1,1,1);
 function voxel(w,h,d,m,x,y,z){let entries=buckets.get(m);if(!entries)buckets.set(m,entries=[]);entries.push([w,h,d,x,y,z]);}
 function pixelMap(size,draw,repeat=1){const canvas=document.createElement('canvas');canvas.width=canvas.height=size;draw(canvas.getContext('2d'),size);const map=new THREE.CanvasTexture(canvas);map.colorSpace=THREE.SRGBColorSpace;map.magFilter=map.minFilter=THREE.NearestFilter;map.generateMipmaps=false;map.wrapS=map.wrapT=THREE.RepeatWrapping;map.repeat.set(repeat,repeat);return map;}
 function mottled(base,colors,count=55){return new THREE.MeshLambertMaterial({map:pixelMap(16,(g,n)=>{g.fillStyle=base;g.fillRect(0,0,n,n);for(let i=0;i<count;i++){g.fillStyle=pick(colors);g.fillRect(random()*n|0,random()*n|0,1+(random()*3|0),1+(random()*2|0));}})});}
 const wood=new THREE.MeshLambertMaterial({map:pixelMap(16,(g,n)=>{g.fillStyle='#be8b54';g.fillRect(0,0,n,n);for(let x=0;x<n;x+=4){g.fillStyle='#8d623e';g.fillRect(x,0,1,n);g.fillStyle='#d8aa6b';g.fillRect(x+1,0,1,n);}for(let i=0;i<18;i++){g.fillStyle=pick(['#a87446','#c7955c']);g.fillRect(random()*n|0,random()*n|0,1,2+(random()*5|0));}})});
 const fenceWood=new THREE.MeshLambertMaterial({map:pixelMap(16,(g,n)=>{g.fillStyle='#b78955';g.fillRect(0,0,n,n);g.fillStyle='#976c43';g.fillRect(0,0,2,n);g.fillStyle='#c49a65';g.fillRect(2,0,1,n);g.fillStyle='#a97b4c';g.fillRect(8,5,2,3);g.fillRect(11,10,1,5);})});
 const woodDark=mottled('#6e4a32',['#79543a','#5f402d'],30),woodEnd=material('#d7aa70');
 const bark=mottled('#735039',['#4d3528','#946343','#65432e'],55);
 const birch=new THREE.MeshLambertMaterial({map:pixelMap(16,(g,n)=>{g.fillStyle='#d6d1b0';g.fillRect(0,0,n,n);for(let i=0;i<17;i++){g.fillStyle=pick(['#434b3b','#8d9276']);g.fillRect(random()*n|0,random()*n|0,2+(random()*4|0),1);}})});
 const leaves=[mottled('#3c7543',['#48844a','#32663c','#518c4b']),mottled('#568e43',['#649c49','#477e3d','#76a64f']),mottled('#71974a',['#82a853','#628942','#8caf5d'])];
 const soil=material('#65442e'),stone=material('#89917d'),stoneEdge=material('#737d6e'),terracotta=material('#ad6241'),terracottaTop=material('#d38b57');
 const grassMap=pixelMap(64,(g,n)=>{g.fillStyle='#65984a';g.fillRect(0,0,n,n);for(let i=0;i<125;i++){g.fillStyle=pick(['#689c4c','#6c9e4e','#619447','#709f51']);g.fillRect(random()*n|0,random()*n|0,2+(random()*6|0),1+(random()*3|0));}},24);
 const ground=new THREE.Mesh(new THREE.PlaneGeometry(60,60),new THREE.MeshLambertMaterial({map:grassMap}));ground.rotation.x=-Math.PI/2;ground.receiveShadow=true;scene.add(ground);
 // A soft sky gradient and a few rectangular clouds add depth beyond the fence.
 const skyGeo=new THREE.SphereGeometry(45,16,10),skyColors=[],skyBottom=new THREE.Color('#f1cba2'),skyTop=new THREE.Color('#84bac9');
 for(let i=0;i<skyGeo.attributes.position.count;i++){const y=skyGeo.attributes.position.getY(i),c=skyBottom.clone().lerp(skyTop,THREE.MathUtils.clamp(y/30,0,1));skyColors.push(c.r,c.g,c.b);}
 skyGeo.setAttribute('color',new THREE.Float32BufferAttribute(skyColors,3));scene.add(new THREE.Mesh(skyGeo,new THREE.MeshBasicMaterial({vertexColors:true,side:THREE.BackSide,fog:false,depthWrite:false})));
 const cloud=new THREE.MeshBasicMaterial({color:'#f6ecd1'}),cloudShade=new THREE.MeshBasicMaterial({color:'#d7dfcf'});
 for(const [x,z]of [[-5,farZ-8],[5,farZ-11],[3,nearZ+9]]){voxel(2.2,.25,.8,cloudShade,x,5.9,z);voxel(1.2,.32,.85,cloud,x-.35,6.16,z-.05);voxel(.9,.2,.7,cloud,x+.65,6.11,z+.05);}
 // Slatted oak fences; raised post caps break up the silhouette.
 function fence(z){for(let x=-6.8;x<=6.8;x+=.255)voxel(.22,1.22,.075,fenceWood,x,.61,z);for(const y of [.34,.94])voxel(13.9,.085,.11,woodDark,0,y,z+.065);for(let x=-6.75;x<=6.8;x+=2.25){voxel(.16,1.37,.16,woodDark,x,.685,z);voxel(.22,.08,.22,woodEnd,x,1.41,z);}}
 fence(farZ);fence(nearZ);
 for(const x of [-6.8,6.8]){for(let z=farZ;z<=nearZ;z+=.255)voxel(.075,1.22,.22,fenceWood,x,.61,z);for(const y of [.34,.94])voxel(.11,.085,nearZ-farZ,woodDark,x-Math.sign(x)*.065,y,(farZ+nearZ)/2);for(let z=farZ;z<=nearZ;z+=2.25){voxel(.16,1.37,.16,woodDark,x,.685,z);voxel(.22,.08,.22,woodEnd,x,1.41,z);}}
 // Vary both canopy silhouette and depth so leaf faces never overlap coplanarly.
 function tree(x,z,s,index){const trunk=index%3===1?birch:bark;voxel(.28*s,2.35*s,.28*s,trunk,x,1.175*s,z);voxel(.15*s,.58*s,.16*s,trunk,x+.2*s,1.65*s,z+.08*s);
  for(const [dx,dy,dz,w,h,d]of [[0,0,0,1.5,.95,1.4],[-.58,.19,.18,.92,.73,.95],[.56,.27,-.21,.96,.82,1.02],[.05,.76,.08,1.08,.72,1.04]])voxel(w*s,h*s,d*s,leaves[index%3],x+dx*s,2.25*s+dy*s,z+dz*s);
 }
 const trees=[[-4.8,farZ-.8,1.05],[-1.8,farZ-1.6,.7],[3.5,farZ-1.2,1.15],[5.5,farZ-.5,.9],[-4.5,nearZ+.9,1.1],[3.8,nearZ+1,1.15],[-4.8,propZ-.4,.9],[4.8,propZ-1.4,1]];
 trees.forEach(([x,z,s],i)=>tree(x,z,s,i));
 // Low hedges and planted flowers stay clear of the central throwing corridor.
 const stem=material('#32633a'),flowers=['#ffe16b','#e98576','#cf9bce'].map(material);
 function flower(x,z,color){voxel(.025,.16,.025,stem,x,.10,z);voxel(.08,.055,.04,color,x,.205,z);voxel(.04,.055,.08,color,x,.205,z);}
 for(const side of [-1,1]){
  for(let i=0;i<5;i++){const x=side*(1.8+i*.72),z=farZ+.55+(i%2)*.22;voxel(.66,.33+(i%2)*.12,.53,leaves[i%3],x,.17+(i%2)*.06,z);}
  for(let i=0;i<24;i++){const x=side*(1.2+random()*4.1),z=farZ+.8+random()*(nearZ-farZ-1.6);if(random()<.35)flower(x,z,pick(flowers));else voxel(.035,.08,.035,stem,x,.04,z);}
  for(let i=0;i<7;i++)voxel(.37,.045,.38,i%2?stone:stoneEdge,side*2.05,.0225,propZ+1.1+i*.47);
 }
 // A miniature picnic corner: slatted table, two benches and a checkered runner.
 const picnicZ=propZ-.9,metal=material('#414f49'),cloth=material('#eee0ba'),clothRed=material('#ba5f45');
 for(const x of [-3.10,-2.22]){voxel(.065,.61,.065,woodDark,x,.305,picnicZ-.23);voxel(.065,.61,.065,woodDark,x,.305,picnicZ+.23);}
 for(let i=0;i<4;i++)voxel(1.18,.065,.17,wood,-2.66,.66,picnicZ-.27+i*.18);
 for(const z of [picnicZ-.58,picnicZ+.58]){voxel(1.26,.065,.2,wood,-2.66,.38,z);for(const x of [-3.08,-2.24])voxel(.07,.35,.12,woodDark,x,.175,z);}
 for(let x=0;x<4;x++)for(let z=0;z<4;z++)voxel(.105,.012,.105,(x+z)%2?cloth:clothRed,-2.82+x*.105,.701,picnicZ-.155+z*.105);
 // Cooler details make this a recognizable object rather than a blue cube.
 const blue=material('#4087a7'),blueShade=material('#286582'),cream=material('#ede4c8');
 function cooler(x,z){voxel(.61,.37,.42,blue,x,.235,z);voxel(.65,.065,.46,cream,x,.452,z);voxel(.53,.045,.35,blueShade,x,.027,z);voxel(.07,.09,.025,cream,x,.36,z+.225);voxel(.26,.025,.025,blueShade,x,.25,z+.226);for(const dx of [-.325,.325]){voxel(.03,.11,.18,cream,x+dx,.3,z);voxel(.035,.035,.14,metal,x+dx,.35,z);}}
 cooler(1.62,propZ+.42);
 // Charcoal grill, plank shelf and warm coals tucked farther into the garden.
 const grillX=2.75,grillZ=propZ-1.35,charcoal=material('#303a37'),grillTop=material('#485550'),coal=new THREE.MeshBasicMaterial({color:'#cb7549'});
 for(const dx of [-.2,.2])for(const dz of [-.16,.16])voxel(.035,.45,.035,metal,grillX+dx,.225,grillZ+dz);
 voxel(.59,.2,.46,charcoal,grillX,.54,grillZ);voxel(.57,.06,.44,grillTop,grillX,.675,grillZ);voxel(.18,.055,.035,cream,grillX,.74,grillZ+.04);
 for(let i=0;i<6;i++)voxel(.025,.018,.36,metal,grillX-.2+i*.08,.713,grillZ);for(const dx of [-.13,.03,.15])voxel(.04,.025,.05,coal,grillX+dx,.704,grillZ+.12);
 voxel(.3,.045,.39,wood,grillX+.49,.57,grillZ);voxel(.14,.025,.17,cloth,grillX+.49,.61,grillZ+.05);
 // Terracotta planters with exposed soil and contrasting flowers.
 for(const [x,z,c]of [[-1.5,propZ+.24,flowers[0]],[1.55,farZ+1,flowers[1]],[-3.8,propZ+1.8,flowers[2]],[-1.05,propZ-1.65,flowers[0]]]){voxel(.29,.25,.29,terracotta,x,.125,z);voxel(.34,.055,.34,terracottaTop,x,.272,z);voxel(.25,.018,.25,soil,x,.306,z);for(const dx of [-.065,.065]){voxel(.025,.22,.025,stem,x+dx,.42,z);voxel(.09,.06,.05,c,x+dx,.55,z);voxel(.05,.06,.09,c,x+dx,.55,z);}}
 // Subtle string lanterns use emissive pixels rather than expensive point lights.
 const wire=new THREE.LineBasicMaterial({color:'#39463f'}),bulbs=['#ffe3a0','#e9977d','#b9d6bb'].map(c=>new THREE.MeshBasicMaterial({color:c}));
 for(const z of [farZ+1.15,nearZ-1.15]){const points=[];for(let i=0;i<=40;i++){const x=-5.7+i*11.4/40;points.push(new THREE.Vector3(x,2.25-.29*(1-(x/5.7)**2),z));}scene.add(new THREE.Line(new THREE.BufferGeometry().setFromPoints(points),wire));for(let i=0;i<14;i++){const x=-5.4+i*.83,y=2.25-.29*(1-(x/5.7)**2);voxel(.015,.09,.015,metal,x,y-.045,z);voxel(.07,.11,.07,bulbs[i%3],x,y-.14,z);voxel(.09,.025,.09,metal,x,y-.085,z);}}
 // Static voxels are batched by material, not one draw call per decorative block.
 const transform=new THREE.Object3D();
 for(const [m,entries]of buckets){const mesh=new THREE.InstancedMesh(unit,m,entries.length);entries.forEach(([w,h,d,x,y,z],i)=>{transform.position.set(x,y,z);transform.scale.set(w,h,d);transform.updateMatrix();mesh.setMatrixAt(i,transform.matrix);});mesh.instanceMatrix.needsUpdate=true;mesh.castShadow=mesh.receiveShadow=true;mesh.computeBoundingSphere();scene.add(mesh);}
 return {wood,woodDark};
}
