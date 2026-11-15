// Three.js +Y is counterclockwise from above. Negative spin is clockwise,
// which curls right (+X); positive spin curls left. No spin means no curl.
export const AIR_SPIN_RATE = 14; // radians/sec at full spin, throughout flight
export const CURL = Object.freeze({slick: {acceleration: 3.2, decay: 1.4}, sticky: {acceleration: 1.5, decay: 4.5}});
export function landingCurl(spin, side) {
 return spin ? -Math.max(-1, Math.min(1, spin)) * CURL[side].acceleration : 0;
}
// Mutate the existing velocity so blocker impulses continue through this step.
export function slideVelocity(velocity, curl, side, surface, gravity, normalGravity, dt) {
 velocity.x += (gravity.x + curl) * dt;
 velocity.z += gravity.z * dt;
 const speed = Math.hypot(velocity.x, velocity.z), deceleration = surface.mu * normalGravity * dt;
 if (speed <= deceleration) { velocity.x = 0; velocity.z = 0; }
 else { velocity.x -= velocity.x / speed * deceleration; velocity.z -= velocity.z / speed * deceleration; }
 return curl * Math.exp(-CURL[side].decay * dt);
}
