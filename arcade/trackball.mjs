// Hardware counts, never cursor position or render-frame distance.
export const FULL_POWER_COUNTS = 400;
export const DEFAULTS=Object.freeze({sensitivity:1,releaseGate:FULL_POWER_COUNTS,spin:1,quietMs:450,timeoutMs:2500,shotKey:'KeyH',surfaceKey:'KeyF',menuKey:'Escape',confirmKey:'Enter',resetKey:'KeyR'});
const clamp=(n,lo,hi)=>Math.max(lo,Math.min(hi,n));
export function settings(value={}){
 const out={...DEFAULTS};
 for(const [key,lo,hi] of [['sensitivity',.25,4],['releaseGate',FULL_POWER_COUNTS,1200],['spin',0,1],['quietMs',200,1200],['timeoutMs',1000,5000]]){const n=Number(value[key]);if(Number.isFinite(n))out[key]=clamp(n,lo,hi);}
 for(const key of ['shotKey','surfaceKey','menuKey','confirmKey','resetKey'])if(typeof value[key]==='string'&&/^(Key[A-Z]|Digit[0-9]|Numpad[0-9]|Enter|Escape|Space|Tab|F[1-9]|F1[0-2]|Pad[0-7]Button(?:[0-9]|[1-5][0-9]|6[0-3]))$/.test(value[key]))out[key]=value[key];
 // Never allow a cabinet mapping to shadow another action or the settings key.
 if(new Set([out.shotKey,out.surfaceKey,out.menuKey,out.confirmKey,out.resetKey,'F2']).size!==6){out.shotKey=DEFAULTS.shotKey;out.surfaceKey=DEFAULTS.surfaceKey;out.menuKey=DEFAULTS.menuKey;out.confirmKey=DEFAULTS.confirmKey;out.resetKey=DEFAULTS.resetKey;}
 return out;
}
// Retire speed calibration; settings raises old release gates and preserves bindings.
export function legacySettings(value = {}) {
 return settings({...value, spin:1});
}

export class Trackball {
 constructor(config = {}) {
  this.config = settings(config);
  this.reset();
 }

 reset(now = 0) {
  this.phase = 'settling';
  this.lastMotion = now;
  this.lastEvent = now;
  this.backX = 0;
  this.backY = 0;
  this.forwardX = 0;
  this.forwardY = 0;
  this.forwardStart = null;
  this.firstInterval = null;
  this.preview = null;
 }

 tick(now) {
  if ((this.phase === 'backswing' && now - this.lastMotion > this.config.timeoutMs) || this.phase === 'forward' &&
      (now - this.lastMotion > 160 || now - this.forwardStart > this.config.timeoutMs)) {
   this.reset(this.lastMotion);
  }
  if (this.phase === 'settling' && now - this.lastMotion >= this.config.quietMs) {
   this.phase = 'ready';
  }
  return this.phase;
 }

 feed(x, y, now) {
  if (![x, y, now].every(Number.isFinite) || now < this.lastEvent ||
      Math.abs(x) > 3000 || Math.abs(y) > 3000) return null;
  this.tick(now);
  this.lastEvent = now;
  if (x === 0 && y === 0) return null;
  this.lastMotion = now;
  if (this.phase === 'settling' || this.phase === 'spent') return null;

  const dx = x * this.config.sensitivity;
  const dy = y * this.config.sensitivity;
  const forward = -dy > 0 && -dy >= Math.abs(dx) * .65;
  if (!forward) {
   if (dy > 2 && dy >= Math.abs(dx) * .65 && this.phase !== 'forward') {
    this.phase = 'backswing';
    this.backX += dx;
    this.backY += dy;
    return null;
   }
   const sideways = Math.abs(dx) > Math.abs(dy);
   if (!sideways && Math.abs(dy) <= 2) return null;
   if (this.phase === 'forward') {
    this.reset(now);
    this.phase = 'ready';
    return sideways ? {kind: 'position', dx} : {kind: 'cancel'};
   }
   return sideways ? {kind: 'position', dx} : null;
  }

  if (this.phase === 'ready' || this.phase === 'backswing') {
   this.phase = 'forward';
   this.forwardStart = now;
  }
  this.forwardX += dx;
  this.forwardY -= dy;

  // Estimate the first report's duration from the following report, not from idle time.
  const elapsed = now - this.forwardStart;
  if (this.firstInterval === null && elapsed > 0) this.firstInterval = clamp(elapsed, 1, 16);
  const speed = this.firstInterval === null ? 0 : this.forwardY * 1000 / (elapsed + this.firstInterval);
  const power = clamp(this.forwardY / FULL_POWER_COUNTS, 0, 1);
  const angle = Math.atan2(this.forwardX, this.forwardY);
  const aim = clamp(angle * .10, -.10, .10);
  // Negative back X (A) gives positive english: clockwise, curling right.
  const spin = this.backY && this.backX ? clamp(-this.backX / this.backY, -1, 1) * this.config.spin : 0;
  this.preview = {power, aim, spin, speed, forward: {x: this.forwardX, y: -this.forwardY}};
  if (this.firstInterval !== null && this.forwardY >= this.config.releaseGate) {
   this.phase = 'spent';
   return {kind: 'throw', ...this.preview};
  }
  return {kind: 'preview', ...this.preview};
 }
}

// Pad 0 panel: bottom row first. Explicit bindings are handled by the controller.
export const P1_LAYOUT=Object.freeze({0:'shotKey',1:'surfaceKey',2:'NudgeLeft',3:'NudgeRight',5:'resetKey',8:'menuKey',9:'menuKey'});
export const PLACEMENT_STEP=.1;
export function p1Action(index,config,menu=false){
 if(menu)return ({0:config.confirmKey,1:config.menuKey,2:config.resetKey,3:config.confirmKey,4:config.menuKey,5:config.resetKey,8:config.menuKey,9:config.confirmKey,12:'ArrowUp',13:'ArrowDown',14:'ArrowLeft',15:'ArrowRight'})[index];
 const action=P1_LAYOUT[index];return config[action]||action;
}
