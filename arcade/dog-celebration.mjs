// Cabinet-only pose animation; scoring triggers it, and the game clock pauses it.
export class DogCelebration {
 constructor(dog){
  this.dog=dog;this.base={position:{...dog.g.position},rotation:{x:dog.g.rotation.x,y:dog.g.rotation.y,z:dog.g.rotation.z},scale:{...dog.g.scale},headY:dog.head.position.y,headX:dog.head.rotation.x,headZ:dog.head.rotation.z};
  this.index=-1;this.count=0;this.elapsed=0;this.active=false;this.duration=2.2;
 }
 get state(){return {active:this.active,kind:['hops','spin','bow'][this.index]||null,count:this.count,elapsed:this.elapsed};}
 celebrate(){this.index=(this.index+1)%3;this.count++;this.elapsed=0;this.active=true;this.restore();}
 restore(){const {dog:d,base:b}=this;Object.assign(d.g.position,b.position);Object.assign(d.g.rotation,b.rotation);Object.assign(d.g.scale,b.scale);d.head.position.y=b.headY;d.head.rotation.x=b.headX;d.head.rotation.z=b.headZ;}
 reset(){this.active=false;this.elapsed=0;this.restore();}
 update(dt){
  if(!this.active)return;
  this.elapsed=Math.min(this.duration,this.elapsed+Math.max(0,dt));
  const t=this.elapsed/this.duration,e=Math.sin(Math.PI*t),{dog:d,base:b}=this;
  this.restore();d.tail.rotation.y=Math.sin(this.elapsed*30)*1.1;
  if(this.index===0){d.g.position.y=b.position.y+Math.abs(Math.sin(t*Math.PI*3))*.34*e;d.g.rotation.z=b.rotation.z+Math.sin(t*Math.PI*6)*.08*e;d.head.rotation.x=b.headX-.18*e;}
  else if(this.index===1){const turn=t*t*(3-2*t);d.g.rotation.y=b.rotation.y+turn*Math.PI*2;d.g.position.y=b.position.y+e*.15;d.head.rotation.z=b.headZ+Math.sin(t*Math.PI*4)*.16*e;}
  else{const bounce=Math.sin(t*Math.PI*4);d.g.rotation.x=b.rotation.x-.22*e;d.g.position.y=b.position.y+Math.max(0,bounce)*.12*e;d.head.position.y=b.headY-.08*e;d.head.rotation.z=b.headZ+Math.sin(t*Math.PI*6)*.22*e;d.g.scale.y=b.scale.y*(1-.1*e);}
  if(this.elapsed>=this.duration)this.reset();
 }
}
