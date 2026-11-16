# Wii build image

`Dockerfile` pins `devkitpro/devkitppc:20261102` (devkitPPC r51 / GCC 16.2, libogc 3.2.1, libfat-ogc 2.1.0)
and WIT 3.05a r8638 (checksum-verified), plus a host compiler and Node for `make host-test`.

```bash
docker build -t backyard-wii-build wii/docker
docker run --rm -v "$PWD":/src backyard-wii-build sh -c 'make -C wii host-test all iso release'
```

Status: written for CI parity, **not run** on the dev box (it has no Docker). The dev box installed the
same package versions directly; see `../TOOLCHAIN.md`.
