// Overall opponent profiles: tournament opponents select a profile, never a separate swat setting.
// Metres / seconds. Attention decides whether the bounce is noticed; contact is always geometric.
export const OPPONENTS={
 rookie:{name:'Patio Rookie',shoot:{precise:.15,jitter:.38},swat:{attention:.40,reaction:.18,jitter:.10,speed:3.7,error:.16,tracking:.10,prediction:.65,reach:.65,radius:.11,windup:.065}},
 regular:{name:'Backyard Regular',shoot:{precise:.32,jitter:.30},swat:{attention:.74,reaction:.14,jitter:.065,speed:4.1,error:.09,tracking:.06,prediction:.85,reach:.85,radius:.11,windup:.045}},
 ace:{name:'Cup Captain',shoot:{precise:.57,jitter:.18},swat:{attention:.93,reaction:.085,jitter:.025,speed:5.4,error:.045,tracking:.035,prediction:.95,reach:1,radius:.11,windup:.035}},
 boss:{name:'The Glove',shoot:{precise:.80,jitter:.10},swat:{attention:.995,reaction:.045,jitter:.015,speed:7,error:.018,tracking:.02,prediction:1,reach:1.1,radius:.11,windup:.025}}
};
const clamp=(v,a,b)=>Math.max(a,Math.min(b,v));
const distance=(a,b)=>Math.hypot(a.x-b.x,a.y-b.y,a.z-b.z);
export class SwatDefender{
 constructor(profile='regular',random=Math.random){this.random=random;this.setProfile(profile);}
 setProfile(id){this.id=OPPONENTS[id]?id:'regular';this.config={...OPPONENTS[this.id].swat};this.reset();}
 reset(){this.phase='idle';this.time=0;this.elapsed=0;this.position={x:.58,y:1.13,z:-1.5};this.target=null;this.result=null;this.side=1;this.noticedAt=null;this.contactAt=null;this.trackTime=0;this.error={x:0,y:0};}
 notice(ball){if(this.phase!=='idle'||!legal(ball))return false;
  this.noticedAt=ball.age;if(this.random()>this.config.attention){this.phase='ignored';this.result='unnoticed';return false;}
  this.side=ball.p.x<0?-1:1;this.position={x:this.side*.58,y:1.13,z:-1.5};
  this.phase='reacting';this.time=Math.max(.015,this.config.reaction+(this.random()*2-1)*this.config.jitter);return true;
 }
 update(dt,ball){
  if(['idle','ignored','finished'].includes(this.phase))return false;
  if(!ball||ball.state!=='fly'){if(this.phase!=='recover'){this.phase='recover';this.elapsed=0;}}
  if(this.phase==='reacting'){
   this.time-=dt;if(this.time>0)return false;
   if(!legal(ball)){this.phase='finished';this.result='too-late';return false;}
   this.error={x:(this.random()*2-1)*this.config.error,y:(this.random()*2-1)*this.config.error*.5};this.aim(ball);
   this.phase='windup';this.elapsed=0;
  }
  this.elapsed+=dt;
  if(this.phase==='windup'){if(this.elapsed>=this.config.windup){this.phase='swing';this.elapsed=0;}return false;}
  if(this.phase==='swing'){
   this.trackTime-=dt;if(this.trackTime<=0&&legal(ball)){this.aim(ball);this.trackTime=this.config.tracking;}
   const d=distance(this.position,this.target),step=Math.min(1,this.config.speed*dt/Math.max(d,.0001));for(const axis of ['x','y','z'])this.position[axis]+=(this.target[axis]-this.position[axis])*step;
   if(legal(ball)&&ball.p.z<=-1.05&&distance(this.position,ball.p)<=this.config.radius+.02){this.result='hit';this.contactAt=ball.age;this.phase='recover';this.elapsed=0;return true;}
   if(this.elapsed>.38){this.result='miss';this.phase='recover';this.elapsed=0;}
  }else if(this.phase==='recover'){
   this.position.x-=this.side*dt*.7;this.position.y+=dt*.1;if(this.elapsed>.3)this.phase='finished';
  }
  return false;
 }
 aim(ball){const c=this.config,lead=clamp(distance(this.position,ball.p)/c.speed,.015,.22)*c.prediction;this.target={x:clamp(ball.p.x+ball.v.x*lead+this.error.x,-c.reach,c.reach),y:clamp(ball.p.y+ball.v.y*lead-4.9*lead*lead+this.error.y,.84,1.4),z:clamp(ball.p.z+ball.v.z*lead,-2.22,-1.05)};}
 snapshot(){return {profile:this.id,phase:this.phase,position:{...this.position},target:this.target?{...this.target}:null,result:this.result,noticedAt:this.noticedAt,contactAt:this.contactAt};}
}
export function legal(ball){return !!ball&&ball.state==='fly'&&ball.team===0&&ball.bounces===1&&ball.bounceAllowed;}
