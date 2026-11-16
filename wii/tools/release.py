#!/usr/bin/env python3
"""Stage the real game release, with stable ZIP metadata and streamed ISO compression."""
import argparse
import hashlib
from pathlib import Path
import shutil
import zipfile

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--version', required=True)
p.add_argument('--out', type=Path, required=True)
a = p.parse_args()
out = a.out.resolve()
out.mkdir(parents=True, exist_ok=True)
files = {
    'BUILD.md': Path('BUILD.md'),
    'INSTALL-WII.md': Path('INSTALL-WII.md'),
    'boot.dol': Path('build/boot.dol'),
    'BackyardCornhole-BYCEAO.iso': Path('dist/BackyardCornhole-BYCEAO.iso'),
    f'BackyardCornhole-HBC-v{a.version}.zip': Path(f'dist/BackyardCornhole-HBC-v{a.version}.zip'),
    'GameSettings/BYCEAO.ini': Path('dolphin/GameSettings/BYCEAO.ini'),
    'BYCEAO.ini': Path('dolphin/GameSettings/BYCEAO.ini'),
    'INSTALL-KINHANK.md': Path('packaging/KINHANK_INSTALL.md'),
    'KINHANK_INSTALL.md': Path('packaging/KINHANK_INSTALL.md'),
}
for name in ('boot.dol', 'meta.xml', 'icon.png'):
    files[f'apps/backyardcornhole/{name}'] = Path(f'hbc/apps/backyardcornhole/{name}')
with files['BackyardCornhole-BYCEAO.iso'].open('rb') as f:
    if f.read(6) != b'BYCEAO':
        raise SystemExit('ISO disc ID must be BYCEAO')
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
archive = f'BackyardCornhole-BYCEAO-v{a.version}-release.zip'
with zipfile.ZipFile(out / archive, 'w', compression=zipfile.ZIP_DEFLATED, compresslevel=9) as z:
    for name in sorted([*files, 'PAYLOAD-SHA256SUMS']):
        info = zipfile.ZipInfo(name, date_time=(2026, 11, 16, 0, 0, 0))
        info.compress_type = zipfile.ZIP_DEFLATED
        info.external_attr = 0o100644 << 16
        with (out / name).open('rb') as src, z.open(info, 'w', force_zip64=True) as dst:
            shutil.copyfileobj(src, dst)
(out / 'SHA256SUMS').write_text(manifest + f'{checksum(out / archive)}  {archive}\n' +
                               f'{checksum(out / "PAYLOAD-SHA256SUMS")}  PAYLOAD-SHA256SUMS\n')
print(f'Release: {out / archive}')
