// Hardware counts, never cursor position or render-frame distance.
export const DEFAULTS=Object.freeze({sensitivity:1,pullRange:240,releaseGate:48,mode:'hybrid',spin:0,quietMs:450,timeoutMs:2500,shotKey:'KeyH',surfaceKey:'KeyF',menuKey:'Escape',confirmKey:'Enter'});
const clamp=(n,lo,hi)=>Math.max(lo,Math.min(hi,n));
export function settings(value={}){
 const out={...DEFAULTS};
 for(const [key,lo,hi] of [['sensitivity',.25,4],['pullRange',80,1200],['releaseGate',20,140],['spin',0,1],['quietMs',200,1200],['timeoutMs',1000,5000]]){const n=Number(value[key]);if(Number.isFinite(n))out[key]=clamp(n,lo,hi);}
 if(['distance','hybrid'].includes(value.mode))out.mode=value.mode;
 for(const key of ['shotKey','surfaceKey','menuKey','confirmKey'])if(typeof value[key]==='string'&&/^(Key[A-Z]|Digit[0-9]|Numpad[0-9]|Enter|Escape|Space|Tab|F[1-9]|F1[0-2]|Pad[0-7]Button(?:[0-9]|[1-5][0-9]|6[0-3]))$/.test(value[key]))out[key]=value[key];
 // Never allow a cabinet mapping to shadow another action or the settings key.
 if(new Set([out.shotKey,out.surfaceKey,out.menuKey,out.confirmKey,'F2']).size!==5){out.shotKey=DEFAULTS.shotKey;out.surfaceKey=DEFAULTS.surfaceKey;out.menuKey=DEFAULTS.menuKey;out.confirmKey=DEFAULTS.confirmKey;}
 return out;
}
export class Trackball {
 constructor(config={}){this.config=settings(config);this.reset(0);}
 reset(now=0){this.phase='settling';this.lastMotion=now;this.lastEvent=now;this.backX=this.backY=this.forwardX=this.forwardY=0;this.forwardStart=null;this.lastForwardAngle=0;this.pendingX=this.pendingY=0;this.preview=null;}
 tick(now){
  if(this.phase==='settling'&&now-this.lastMotion>=this.config.quietMs)this.phase='ready';
  if(['back','forward'].includes(this.phase)&&now-this.lastMotion>this.config.timeoutMs)this.reset(now);
  return this.phase;
 }
 feed(x,y,now){
  if(![x,y,now].every(Number.isFinite)||now<this.lastEvent||Math.abs(x)>3000||Math.abs(y)>3000)return null;
  this.lastEvent=now;this.tick(now);if(x===0&&y===0)return null;
  const previousMotion=this.lastMotion;this.lastMotion=now;const dx=x*this.config.sensitivity,dy=y*this.config.sensitivity;
  if(this.phase==='settling'||this.phase==='spent')return null;
  if(this.phase==='ready'){
   if(dy>0){this.pendingY+=dy;this.pendingX+=dx;}else if(dy<0){this.pendingX=this.pendingY=0;}
   if(this.pendingY>=10){this.phase='back';this.backX=this.pendingX;this.backY=this.pendingY;return {kind:'backswing'};}
   return {kind:'position',dx};
  }
  if(this.phase==='back'&&dy>=0){this.backX+=dx;this.backY=clamp(this.backY+dy,0,this.config.pullRange*1.5);}
  else if(dy<0){
   if(this.forwardStart===null)this.forwardStart=Math.max(previousMotion,now-50);
   this.phase='forward';this.forwardX+=dx;this.forwardY-=dy;this.lastForwardAngle=Math.atan2(dx,-dy);
  }else if(dy>6){this.phase='back';this.backX+=dx;this.backY=clamp(this.backY+dy,0,this.config.pullRange*1.5);this.forwardX=this.forwardY=0;this.forwardStart=null;}
  const distance=clamp(this.backY/this.config.pullRange,0,1);
  // A bounded speed contribution rewards a deliberate stroke; a single peak is never sampled.
  const seconds=Math.max(.016,(now-(this.forwardStart??now))/1000);
  const speed=clamp(this.forwardY/seconds/1800,0,1);
  const power=this.config.mode==='distance'?distance:clamp(distance*.8+speed*.2,0,1);
  const angle=Math.atan2(this.forwardX,Math.max(1,this.forwardY));
  const aim=clamp(angle*.18,-.12,.12);
  // Optional wrist-finish approximation: straight strokes never add spin.
  // Backswing angle is deliberately irrelevant; this is not a golf hook/slice model.
  const spin=clamp((this.lastForwardAngle-angle)*this.config.spin,-1,1)||0;
  this.preview={power,aim,spin,distance,backswing:{x:this.backX,y:this.backY},forward:{x:this.forwardX,y:-this.forwardY}};
  if(this.phase==='forward'&&this.forwardY>=this.config.releaseGate){this.phase='spent';return {kind:'throw',...this.preview};}
  return {kind:'preview',...this.preview};
 }
}
