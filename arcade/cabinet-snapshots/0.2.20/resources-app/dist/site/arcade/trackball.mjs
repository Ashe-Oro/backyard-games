// Hardware counts, never cursor position or render-frame distance.
export const SPEED_WINDOW_MS = 80;
const MIN_SAMPLE_MS = 16, MIN_FORWARD_COUNTS = 8;
export const DEFAULTS=Object.freeze({sensitivity:1,powerSpeed:3300,releaseGate:400,spin:1,quietMs:450,timeoutMs:2500,shotKey:'KeyH',surfaceKey:'KeyF',menuKey:'Escape',confirmKey:'Enter',resetKey:'KeyR'});
const clamp=(n,lo,hi)=>Math.max(lo,Math.min(hi,n));
export function settings(value={}){
 const out={...DEFAULTS};
 for(const [key,lo,hi] of [['sensitivity',.25,4],['powerSpeed',400,10000],['releaseGate',20,1200],['spin',0,1],['quietMs',200,1200],['timeoutMs',1000,5000]]){const n=Number(value[key]);if(Number.isFinite(n))out[key]=clamp(n,lo,hi);}
 for(const key of ['shotKey','surfaceKey','menuKey','confirmKey','resetKey'])if(typeof value[key]==='string'&&/^(Key[A-Z]|Digit[0-9]|Numpad[0-9]|Enter|Escape|Space|Tab|F[1-9]|F1[0-2]|Pad[0-7]Button(?:[0-9]|[1-5][0-9]|6[0-3]))$/.test(value[key]))out[key]=value[key];
 // Never allow a cabinet mapping to shadow another action or the settings key.
 if(new Set([out.shotKey,out.surfaceKey,out.menuKey,out.confirmKey,out.resetKey,'F2']).size!==6){out.shotKey=DEFAULTS.shotKey;out.surfaceKey=DEFAULTS.surfaceKey;out.menuKey=DEFAULTS.menuKey;out.confirmKey=DEFAULTS.confirmKey;out.resetKey=DEFAULTS.resetKey;}
 return out;
}
// Profile loading only: settings() still accepts explicit calibrated factory numbers.
export function profileSettings(value = {}) {
 return settings({...value,powerSpeed:value.powerSpeed == null || [1050,1650,2100,6500].includes(value.powerSpeed) ? DEFAULTS.powerSpeed : value.powerSpeed});
}
// Upgrade factory values once; retain custom gates, spin, sensitivity and bindings.
// v8 omitted speed; older factory speed values all move to the v9 default.
export function previousSettings(value = {}) {
 return profileSettings({...value,
  releaseGate:[220,48,80].includes(value.releaseGate) ? DEFAULTS.releaseGate : value.releaseGate,
  spin:value.spin === 0 ? 1 : value.spin});
}
export function legacySettings(value = {}) {
 const out=previousSettings(value);
 // Old distance calibrations cannot establish speed: avoid a hair-trigger.
 return settings({...out,powerSpeed:Math.max(DEFAULTS.powerSpeed,out.powerSpeed)});
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
  this.history = [];
  this.peakSpeed = 0;
  this.hasSample = false;
  this.preview = null;
 }

 finish() {
  if (!this.hasSample) return null;
  this.phase = 'spent';
  return {kind: 'throw', ...this.preview};
 }

 tick(now) {
  if (!Number.isFinite(now) || now < this.lastEvent) return this.phase;
  if (this.phase === 'forward' &&
      (now - this.lastMotion >= 160 || now - this.forwardStart >= this.config.timeoutMs)) {
   const shot = this.finish();
   if (shot) return shot;
   this.reset(this.lastMotion);
  }
  if (this.phase === 'backswing' && now - this.lastMotion > this.config.timeoutMs) this.reset(this.lastMotion);
  if (this.phase === 'settling' && now - this.lastMotion >= this.config.quietMs) this.phase = 'ready';
  return this.phase;
 }

 feed(x, y, now) {
  if (![x, y, now].every(Number.isFinite) || now < this.lastEvent ||
      Math.abs(x) > 3000 || Math.abs(y) > 3000) return null;
  const timed = this.tick(now);
  this.lastEvent = now;
  if (timed?.kind === 'throw') {
   if (x !== 0 || y !== 0) this.lastMotion = now;
   return timed;
  }
  if (x === 0 && y === 0) return null;
  this.lastMotion = now;
  if (this.phase === 'settling' || this.phase === 'spent') return null;

  const dx = x * this.config.sensitivity;
  const dy = y * this.config.sensitivity;
  const forward = -dy > 0 && -dy >= Math.abs(dx) * .65;
  if (!forward) {
   if (this.phase === 'forward' && this.hasSample) return this.finish();
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

  // Cumulative samples let us interpolate the exact 80 ms boundary. The
  // opening report (and any reports at that timestamp) has no known duration.
  const elapsed = now - this.forwardStart;
  const last = this.history.at(-1);
  if (last?.time === now) last.counts = this.forwardY;
  else this.history.push({time: now, counts: this.forwardY});
  const boundary = Math.max(this.forwardStart, now - SPEED_WINDOW_MS);
  while (this.history.length > 2 && this.history[1].time <= boundary) this.history.shift();
  const first = this.history[0], next = this.history[1];
  const startCounts = next && boundary > first.time
   ? first.counts + (next.counts - first.counts) * (boundary - first.time) / (next.time - first.time)
   : first.counts;
  const windowMs = now - boundary, windowCounts = this.forwardY - startCounts;
  const speed = windowMs >= MIN_SAMPLE_MS ? windowCounts * 1000 / windowMs : 0;
  if (windowMs >= MIN_SAMPLE_MS && windowCounts >= MIN_FORWARD_COUNTS) {
   this.hasSample = true;
   this.peakSpeed = Math.max(this.peakSpeed, speed);
  }
  const power = clamp(this.peakSpeed / this.config.powerSpeed, 0, 1);
  const angle = Math.atan2(this.forwardX, this.forwardY);
  const aim = clamp(angle * .10, -.10, .10);
  // Back-left gives positive clockwise landing spin; forward X still aims.
  const spin = this.backY && this.backX ? clamp(-this.backX / this.backY, -1, 1) * this.config.spin : 0;
  this.preview = {power, aim, spin, speed: this.peakSpeed, forward: {x: this.forwardX, y: -this.forwardY}};
  // A 40% drop after a full window ends the fast part before a long coast.
  if (this.hasSample && (this.forwardY >= this.config.releaseGate ||
      elapsed >= SPEED_WINDOW_MS && speed <= this.peakSpeed * .6)) return this.finish();
  return {kind: 'preview', ...this.preview};
 }
}

// Pad 0 Twin USB grid: top 0/1/2/6, bottom 3/4/5/7 (4/6/7 inferred).
export const P1_LAYOUT=Object.freeze({0:'shotKey',1:'surfaceKey',2:'resetKey',3:'NudgeLeft',4:'NudgeRight',5:'resetKey',6:'resetKey',7:'resetKey',8:'menuKey',9:'menuKey'});
export const PLACEMENT_STEP=.1;
export function p1Action(index,config,menu=false){
 if(menu)return ({0:config.confirmKey,1:config.menuKey,2:config.resetKey,3:config.confirmKey,4:config.menuKey,5:config.resetKey,8:config.menuKey,9:config.confirmKey,12:'ArrowUp',13:'ArrowDown',14:'ArrowLeft',15:'ArrowRight'})[index];
 const action=P1_LAYOUT[index];return config[action]||action;
}
