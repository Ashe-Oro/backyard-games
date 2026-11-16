#!/usr/bin/env node
// Generates the 128x48 Homebrew Channel icon (hbc/apps/backyardcornhole/icon.png) from code:
// teal card, a pixel cornhole board with its sunburst + hole, a red bag, and "CORNHOLE" lettering.
import {writeFileSync} from 'node:fs';
import {deflateSync} from 'node:zlib';

const W = 128, H = 48, px = new Uint8Array(W * H * 4);
const set = (x, y, hex, a = 255) => { if (x < 0 || y < 0 || x >= W || y >= H) return; const i = (y * W + x) * 4; px[i] = hex >> 16; px[i + 1] = (hex >> 8) & 255; px[i + 2] = hex & 255; px[i + 3] = a; };
const rect = (x, y, w, h, c) => { for (let j = y; j < y + h; j++) for (let i = x; i < x + w; i++) set(i, j, c); };
rect(0, 0, W, H, 0x173f3d);
rect(0, 0, W, 2, 0x172c32); rect(0, H - 2, W, 2, 0x172c32); rect(0, 0, 2, H, 0x172c32); rect(W - 2, 0, 2, H, 0x172c32);
// board (trapezoid in perspective)
for (let y = 8; y < 42; y++) { const half = 7 + Math.round((y - 8) * 0.25); rect(22 - half, y, half * 2, 1, 0xe8c28c); }
for (let y = 8; y < 42; y++) { const half = 7 + Math.round((y - 8) * 0.25); set(22 - half, y, 0xfff4dc); set(21 + half, y, 0xfff4dc); }
for (let y = 10; y < 22; y++) for (let x = 12; x < 32; x++) if (Math.hypot(x - 21.5, y - 15) < 7.5) set(x, y, y < 15 ? 0xffe45e : 0xff8a3d);
for (let y = 10; y < 22; y++) for (let x = 12; x < 32; x++) { const d = Math.hypot(x - 21.5, y - 15); if (d < 3.2) set(x, y, 0x1c120c); else if (d < 4.2) set(x, y, 0xfff4dc); }
rect(14, 30, 16, 2, 0x1aa39a); rect(14, 33, 16, 2, 0xfff4dc);
// bag
rect(30, 26, 10, 8, 0xe8423c); rect(30, 26, 10, 1, 0x8c1f2a); rect(30, 33, 10, 1, 0x8c1f2a); rect(30, 26, 1, 8, 0x8c1f2a); rect(39, 26, 1, 8, 0x8c1f2a);
// 3x5 pixel font
const F = {B: '110101110101110', A: '010101111101101', C: '011100100100011', K: '101101110101101', Y: '101101010010010', R: '110101110101101', D: '110101101101110', O: '010101101101010', N: '101111111111101', H: '101101111101101', L: '100100100100111', E: '111100110100111'};
const word = (s, x, y, scale, c) => { for (const ch of s) { const g = F[ch]; if (g) for (let r = 0; r < 5; r++) for (let k = 0; k < 3; k++) if (g[r * 3 + k] === '1') rect(x + k * scale, y + r * scale, scale, scale, c); x += 4 * scale; } };
word('BACKYARD', 50, 9, 2, 0xfff1cf);
word('CORNHOLE', 50, 25, 2, 0xffe16b);

// PNG encode (RGBA8)
const crcTable = Array.from({length: 256}, (_, n) => { let c = n; for (let k = 0; k < 8; k++) c = c & 1 ? 0xEDB88320 ^ (c >>> 1) : c >>> 1; return c >>> 0; });
const crc = (b) => { let c = 0xFFFFFFFF; for (const x of b) c = crcTable[(c ^ x) & 255] ^ (c >>> 8); return (c ^ 0xFFFFFFFF) >>> 0; };
const chunk = (type, data) => { const len = Buffer.alloc(4); len.writeUInt32BE(data.length); const td = Buffer.concat([Buffer.from(type, 'ascii'), data]); const c = Buffer.alloc(4); c.writeUInt32BE(crc(td)); return Buffer.concat([len, td, c]); };
const ihdr = Buffer.alloc(13); ihdr.writeUInt32BE(W, 0); ihdr.writeUInt32BE(H, 4); ihdr[8] = 8; ihdr[9] = 6;
const raw = Buffer.alloc((W * 4 + 1) * H);
for (let y = 0; y < H; y++) { raw[y * (W * 4 + 1)] = 0; Buffer.from(px.buffer, y * W * 4, W * 4).copy(raw, y * (W * 4 + 1) + 1); }
const png = Buffer.concat([Buffer.from([137, 80, 78, 71, 13, 10, 26, 10]), chunk('IHDR', ihdr), chunk('IDAT', deflateSync(raw, {level: 9})), chunk('IEND', Buffer.alloc(0))]);
writeFileSync(process.argv[2] || 'icon.png', png);
console.log(`wrote ${process.argv[2] || 'icon.png'} (${W}x${H}, ${png.length} bytes)`);
