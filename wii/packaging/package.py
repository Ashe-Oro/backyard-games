#!/usr/bin/env python3
"""Compose a Wii homebrew release from an ELF and our source-built apploader."""
import argparse
import hashlib
from pathlib import Path
import shutil
import struct
import subprocess
import zipfile
import zlib


def placeholder_icon(path):
    """Original geometric cornfield/board placeholder; PNG, 128x48, no assets."""
    pixels = bytearray()
    for y in range(48):
        pixels.append(0)  # PNG filter: none
        for x in range(128):
            rgb = (24, 70, 45)
            if 14 <= x < 114 and 9 <= y < 39:
                rgb = (225, 190, 116)
            if (x - 90) ** 2 + (y - 23) ** 2 < 49:
                rgb = (24, 70, 45)
            if 32 <= x < 46 and 24 <= y < 36:
                rgb = (190, 51, 48)
            pixels.extend(rgb)

    def chunk(tag, data):
        return struct.pack('>I', len(data)) + tag + data + struct.pack(
            '>I', zlib.crc32(tag + data) & 0xffffffff)

    path.write_bytes(b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack(
        '>IIBBBBB', 128, 48, 8, 2, 0, 0, 0)) + chunk(
        b'IDAT', zlib.compress(pixels, 9)) + chunk(b'IEND', b''))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--elf', type=Path, required=True)
    parser.add_argument('--out', type=Path, required=True)
    parser.add_argument('--devkitpro', type=Path, default=Path('/opt/devkitpro'))
    args = parser.parse_args()
    src = Path(__file__).resolve().parent
    out = args.out.resolve()
    out.mkdir(parents=True, exist_ok=True)
    elf = args.elf.resolve()
    if elf.read_bytes()[:6] != b'\x7fELF\x01\x02':
        parser.error('input must be a 32-bit big-endian ELF')
    wit = shutil.which('wit')
    if not wit:
        parser.error('wit is required (Debian: apt-get install wit)')
    subprocess.run([str(args.devkitpro / 'tools/bin/elf2dol'), str(elf),
                    str(out / 'boot.dol')], check=True)
    hbc = out / 'apps/backyardcornhole'
    hbc.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(out / 'boot.dol', hbc / 'boot.dol')
    # Keep the project's version source; avoid claiming the stub is playable.
    version = subprocess.check_output(
        ['make', '-s', '-C', str(src.parent), 'version'], text=True).strip()
    (hbc / 'meta.xml').write_text(f'''<?xml version="1.0" encoding="UTF-8"?>
<app version="1">
  <name>Backyard Cornhole</name>
  <coder>Ashe</coder>
  <version>{version}</version>
  <short_description>Backyard Cornhole Wii homebrew</short_description>
  <long_description>Development build. The packaging stub is not a playable game. NTSC / 60 Hz.</long_description>
</app>
''')
    placeholder_icon(hbc / 'icon.png')
    settings = out / 'GameSettings'
    settings.mkdir(exist_ok=True)
    (settings / 'BYCEAO.ini').write_text(
        '[Controls]\nWiimoteSource0 = 2\nWiimoteSource1 = 2\n')
    root = out / 'iso-root'
    system = root / 'sys'
    system.mkdir(parents=True, exist_ok=True)
    (root / 'files').mkdir(exist_ok=True)
    boot = bytearray(0x440)
    boot[:6] = b'BYCEAO'
    struct.pack_into('>I', boot, 0x18, 0x5d1c9ea3)  # Wii magic
    boot[0x20:0x20 + 17] = b'Backyard Cornhole'
    (system / 'boot.bin').write_bytes(boot)
    bi2 = bytearray(0x2000)
    struct.pack_into('>I', bi2, 0x18, 1)  # USA / NTSC
    (system / 'bi2.bin').write_bytes(bi2)
    loader = (out / 'apploader.bin').read_bytes()
    header = b'2026/11/15\x00'.ljust(16, b'\x00') + struct.pack(
        '>IIII', 0x81200000, len(loader), 0, 0)
    (system / 'apploader.img').write_bytes(header + loader)
    shutil.copyfile(out / 'boot.dol', system / 'main.dol')
    # WIT 3.01 also requires bi2.bin; all metadata is generated locally.
    (root / 'setup.txt').write_text('disc-type = Wii\npart-id = BYCEAO\n')
    iso = out / 'BackyardCornhole-BYCEAO.iso'
    subprocess.run([wit, 'copy', str(root), str(iso), '--iso', '--overwrite',
                    '--id=BYCEAO', '--name=Backyard Cornhole', '--region=USA',
                    '--ios=58', '--enc=ENCRYPT'], check=True)
    # Verify the DOL actually present in the ISO, rather than just staging files.
    verify = out / 'verify-iso'
    if verify.exists():
        shutil.rmtree(verify)
    subprocess.run([wit, 'extract', str(iso), str(verify),
                    '--files=+sys/main.dol'], check=True)
    extracted = list(verify.rglob('main.dol'))
    if len(extracted) != 1 or extracted[0].read_bytes() != (out / 'boot.dol').read_bytes():
        raise RuntimeError('ISO main.dol differs from HBC boot.dol')
    shutil.rmtree(verify)
    for name in ('KINHANK_INSTALL.md', 'README.md'):
        shutil.copyfile(src / name, out / name)
    payload = [out / 'boot.dol', iso, *sorted(hbc.iterdir()),
               settings / 'BYCEAO.ini', out / 'KINHANK_INSTALL.md', out / 'README.md']
    # Stream ISO hashing/zipping: the ISO is sparse on disk, large logically.
    sums = []
    for path in payload:
        with path.open('rb') as f:
            digest = hashlib.file_digest(f, 'sha256').hexdigest()
        sums.append(f'{digest}  {path.relative_to(out)}\n')
    checksum = out / 'SHA256SUMS'
    checksum.write_text(''.join(sums))
    archive = out / 'BackyardCornhole-BYCEAO-release.zip'
    with zipfile.ZipFile(archive, 'w', zipfile.ZIP_DEFLATED, compresslevel=9) as z:
        for path in [*payload, checksum]:
            z.write(path, str(path.relative_to(out)))
    for path in [*payload, checksum, archive]:
        print(f'{path.stat().st_size:>12}  {path}')


if __name__ == '__main__':
    main()
