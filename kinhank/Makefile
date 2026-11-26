# Backyard Cornhole — Kinhank (Dolphin on the Kinhank)
# Packages the Wii ISO with this target's GameSettings. Does not rebuild the game.
#
#   make -C kinhank ISO=../wii/dist/BackyardCornhole-BYCEAO.iso
#
# The ISO is produced by `make -C wii iso`. GameSettings/BYCEAO.ini here is
# the source of truth and must set InternalResolution = 3.

VERSION := 0.1.9
ISO ?= ../wii/dist/BackyardCornhole-BYCEAO.iso
OUT ?= dist
PYTHON ?= python3

.PHONY: all kinhank clean version

all: kinhank

version:
	@echo $(VERSION)

kinhank:
	@grep -Fq "## [$(VERSION)]" CHANGELOG.md
	@grep -qx "WiimoteSource0 = 2" GameSettings/BYCEAO.ini
	@grep -qx "WiimoteSource1 = 2" GameSettings/BYCEAO.ini
	@grep -qx "InternalResolution = 3" GameSettings/BYCEAO.ini
	$(PYTHON) tools/release.py --version $(VERSION) --iso "$(ISO)" --out "$(OUT)"
	@cat "$(OUT)/SHA256SUMS"

clean:
	@rm -rf dist
