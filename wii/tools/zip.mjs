#!/usr/bin/env node
// Minimal deterministic ZIP (store, no compression) for the HBC bundle: node zip.mjs out.zip <dir-or-file> [...]
import {readdirSync, readFileSync, statSync, writeFileSync} from 'node:fs';
import {join} from 'node:path';
const [out, ...roots] = process.argv.slice(2);
const crcT = Array.from({length: 256}, (_, n) => { let c = n; for (let k = 0; k < 8; k++) c = c & 1 ? 0xEDB88320 ^ (c >>> 1) : c >>> 1; return c >>> 0; });
const crc = (b) => { let c = 0xFFFFFFFF; for (const x of b) c = crcT[(c ^ x) & 255] ^ (c >>> 8); return (c ^ 0xFFFFFFFF) >>> 0; };
const files = [];
function walk(p) {
  if (statSync(p).isDirectory()) {
    for (const n of readdirSync(p).sort()) walk(join(p, n));
  } else files.push(p);
}
for (const root of roots) walk(root);
files.sort();
const parts = [], central = []; let off = 0;
for (const f of files) {
  const data = readFileSync(f), name = Buffer.from(f.replace(/\\/g, '/')), c = crc(data);
  const h = Buffer.alloc(30); h.writeUInt32LE(0x04034b50, 0); h.writeUInt16LE(20, 4); h.writeUInt16LE(0x5A21, 10); h.writeUInt16LE(0x5A6F, 12);
  h.writeUInt32LE(c, 14); h.writeUInt32LE(data.length, 18); h.writeUInt32LE(data.length, 22); h.writeUInt16LE(name.length, 26);
  parts.push(h, name, data);
  const e = Buffer.alloc(46); e.writeUInt32LE(0x02014b50, 0); e.writeUInt16LE(20, 4); e.writeUInt16LE(20, 6); e.writeUInt16LE(0x5A21, 12); e.writeUInt16LE(0x5A6F, 14);
  e.writeUInt32LE(c, 16); e.writeUInt32LE(data.length, 20); e.writeUInt32LE(data.length, 24); e.writeUInt16LE(name.length, 28); e.writeUInt32LE(off, 42);
  central.push(e, name); off += 30 + name.length + data.length;
}
const cd = Buffer.concat(central), end = Buffer.alloc(22);
end.writeUInt32LE(0x06054b50, 0); end.writeUInt16LE(files.length, 8); end.writeUInt16LE(files.length, 10); end.writeUInt32LE(cd.length, 12); end.writeUInt32LE(off, 16);
writeFileSync(out, Buffer.concat([...parts, cd, end]));
console.log(`wrote ${out}: ${files.length} files`);
