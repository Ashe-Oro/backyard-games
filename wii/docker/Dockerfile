# Pinned Wii build image (REPO_LAYOUT §5, TOOLCHAIN.md). Not exercised on the dev box (no Docker
# there); the dev box used the same package versions installed from pkg.devkitpro.org.
FROM devkitpro/devkitppc:20261102
# Host compiler + Node for host tests and generators; WIT 3.05a for the Dolphin ISO.
RUN apt-get update && apt-get install -y --no-install-recommends build-essential curl ca-certificates nodejs \
 && rm -rf /var/lib/apt/lists/*
RUN curl -fsSL -o /tmp/wit.tgz https://wit.wiimm.de/download/wit-v3.05a-r8638-x86_64.tar.gz \
 && echo "df95a14f1b4b2bd95d3111b70c6304e91d4ed3e4c7f72b1ad5306c3616d8933f  /tmp/wit.tgz" | sha256sum -c - \
 && mkdir -p /opt/wit && tar -xzf /tmp/wit.tgz -C /opt/wit --strip-components=1 && rm /tmp/wit.tgz
ENV WIT=/opt/wit/bin/wit
WORKDIR /src
# docker run --rm -v "$PWD":/src backyard-wii-build sh -c 'make -C wii host-test all iso release'
