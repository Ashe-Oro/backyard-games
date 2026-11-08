// Hardware counts, never cursor position or render-frame distance.
export const DEFAULTS=Object.freeze({sensitivity:1,powerSpeed:6500,releaseGate:220,spin:0,quietMs:450,timeoutMs:2500,shotKey:'KeyH',surfaceKey:'KeyF',menuKey:'Escape',confirmKey:'Enter',resetKey:'KeyR'});
const clamp=(n,lo,hi)=>Math.max(lo,Math.min(hi,n));
export function settings(value={}){
 const out={...DEFAULTS};
 for(const [key,lo,hi] of [['sensitivity',.25,4],['powerSpeed',400,10000],['releaseGate',20,400],['spin',0,1],['quietMs',200,1200],['timeoutMs',1000,5000]]){const n=Number(value[key]);if(Number.isFinite(n))out[key]=clamp(n,lo,hi);}
 for(const key of ['shotKey','surfaceKey','menuKey','confirmKey','resetKey'])if(typeof value[key]==='string'&&/^(Key[A-Z]|Digit[0-9]|Numpad[0-9]|Enter|Escape|Space|Tab|F[1-9]|F1[0-2]|Pad[0-7]Button(?:[0-9]|[1-5][0-9]|6[0-3]))$/.test(value[key]))out[key]=value[key];
 // Never allow a cabinet mapping to shadow another action or the settings key.
 if(new Set([out.shotKey,out.surfaceKey,out.menuKey,out.confirmKey,out.resetKey,'F2']).size!==6){out.shotKey=DEFAULTS.shotKey;out.surfaceKey=DEFAULTS.surfaceKey;out.menuKey=DEFAULTS.menuKey;out.confirmKey=DEFAULTS.confirmKey;out.resetKey=DEFAULTS.resetKey;}
 return out;
}
// Distance calibration cannot measure forward speed. Keep bindings and sensitivity.
export function legacySettings(value = {}) {
 const releaseGate = value.releaseGate === 48 || value.releaseGate === 80
  ? DEFAULTS.releaseGate : value.releaseGate;
 const powerSpeed = Math.max(DEFAULTS.powerSpeed, settings(value).powerSpeed);
 return settings({...value, powerSpeed, releaseGate});
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
  this.forwardX = 0;
  this.forwardY = 0;
  this.forwardStart = null;
  this.firstInterval = null;
  this.preview = null;
 }

 tick(now) {
  if (this.phase === 'forward' &&
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
   const sideways = Math.abs(dx) > Math.abs(dy);
   if (!sideways && Math.abs(dy) <= 2) return null;
   if (this.phase === 'forward') {
    this.reset(now);
    this.phase = 'ready';
    return sideways ? {kind: 'position', dx} : {kind: 'cancel'};
   }
   return sideways ? {kind: 'position', dx} : null;
  }

  if (this.phase === 'ready') {
   this.phase = 'forward';
   this.forwardStart = now;
  }
  this.forwardX += dx;
  this.forwardY -= dy;

  // Estimate the first report's duration from the following report, not from idle time.
  const elapsed = now - this.forwardStart;
  if (this.firstInterval === null && elapsed > 0) this.firstInterval = clamp(elapsed, 1, 16);
  const speed = this.firstInterval === null ? 0 : this.forwardY * 1000 / (elapsed + this.firstInterval);
  const power = clamp(speed / this.config.powerSpeed, 0, 1);
  const angle = Math.atan2(this.forwardX, this.forwardY);
  const aim = clamp(angle * .10, -.10, .10);
  const finishAngle = Math.atan2(dx, -dy);
  const spin = clamp((finishAngle - angle) * this.config.spin, -1, 1);
  this.preview = {power, aim, spin, speed, forward: {x: this.forwardX, y: -this.forwardY}};
  if (this.firstInterval !== null && this.forwardY >= this.config.releaseGate) {
   this.phase = 'spent';
   return {kind: 'throw', ...this.preview};
  }
  return {kind: 'preview', ...this.preview};
 }
}
