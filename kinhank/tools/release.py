#!/usr/bin/env python3
"""Stage the Kinhank (Dolphin) release: ISO + 3x GameSettings, no HBC payload."""
import argparse
import hashlib
from pathlib import Path
import shutil
import zipfile

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--version', required=True)
p.add_argument('--iso', type=Path, required=True)
p.add_argument('--out', type=Path, required=True)
a = p.parse_args()
root = Path(__file__).resolve().parents[1]
iso = a.iso.resolve()
if not iso.is_file():
    raise SystemExit(f'ISO not found: {iso} (build it with make -C wii iso)')
with iso.open('rb') as f:
    if f.read(6) != b'BYCEAO':
        raise SystemExit('ISO disc ID must be BYCEAO')
ini = (root / 'GameSettings' / 'BYCEAO.ini').read_text()
if 'InternalResolution = 3' not in ini:
    raise SystemExit('kinhank/GameSettings/BYCEAO.ini must set InternalResolution = 3')
if 'WiimoteSource0 = 2' not in ini or 'WiimoteSource1 = 2' not in ini:
    raise SystemExit('kinhank/GameSettings/BYCEAO.ini must enable real Wii Remotes')

out = a.out.resolve()
out.mkdir(parents=True, exist_ok=True)
files = {
    'BackyardCornhole-BYCEAO.iso': iso,
    'GameSettings/BYCEAO.ini': root / 'GameSettings' / 'BYCEAO.ini',
    'BYCEAO.ini': root / 'GameSettings' / 'BYCEAO.ini',
    'INSTALL.md': root / 'INSTALL.md',
    'install-adb.sh': root / 'install-adb.sh',
}
for name, src in files.items():
    dst = out / name
    dst.parent.mkdir(parents=True, exist_ok=True)
    if src.resolve() != dst:
        shutil.copyfile(src, dst)

def checksum(path):
    with path.open('rb') as f:
        return hashlib.file_digest(f, 'sha256').hexdigest()

manifest = ''.join(f'{checksum(out / name)}  {name}\n' for name in sorted(files))
(out / 'PAYLOAD-SHA256SUMS').write_text(manifest)
archive = f'BackyardCornhole-Kinhank-v{a.version}.zip'
with zipfile.ZipFile(out / archive, 'w', compression=zipfile.ZIP_DEFLATED, compresslevel=9) as z:
    for name in sorted([*files, 'PAYLOAD-SHA256SUMS']):
        info = zipfile.ZipInfo(name, date_time=(2026, 11, 21, 0, 0, 0))
        info.compress_type = zipfile.ZIP_DEFLATED
        mode = 0o100755 if name.endswith('.sh') else 0o100644
        info.external_attr = mode << 16
        with (out / name).open('rb') as src, z.open(info, 'w', force_zip64=True) as dst:
            shutil.copyfileobj(src, dst)
    names = z.namelist()
    if 'GameSettings/BYCEAO.ini' not in names:
        raise SystemExit('Kinhank zip missing GameSettings/BYCEAO.ini')
    if b'InternalResolution = 3' not in z.read('GameSettings/BYCEAO.ini'):
        raise SystemExit('Kinhank zip ini is missing InternalResolution = 3')
    leaked = [n for n in names if n.startswith('apps/') or n in ('INSTALL-WII.md', 'boot.dol')]
    if leaked:
        raise SystemExit(f'Kinhank zip must not include Wii-only files: {leaked}')

(out / 'SHA256SUMS').write_text(
    manifest
    + f'{checksum(out / archive)}  {archive}\n'
    + f'{checksum(out / "PAYLOAD-SHA256SUMS")}  PAYLOAD-SHA256SUMS\n'
)
print(f'Kinhank release: {out / archive}')
