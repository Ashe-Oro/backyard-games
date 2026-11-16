# Compatibility entry point: always build the approved real game.
OUT ?= /workspace/uploads/wii-game
.PHONY: all package smoke
all: package
package:
	$(MAKE) -C .. release OUT="$(OUT)"
smoke:
	$(MAKE) -C .. smoke OUT="$(OUT)"
