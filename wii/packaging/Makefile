DEVKITPRO ?= /opt/devkitpro
DEVKITPPC ?= $(DEVKITPRO)/devkitPPC
OUT ?= /workspace/uploads/wii-stub
ELF ?= $(OUT)/stub.elf
PPC := $(DEVKITPPC)/bin/powerpc-eabi
WII_FLAGS := -DGEKKO -mrvl -mcpu=750 -meabi -mhard-float

.PHONY: all stub package smoke clean
all: package
stub: $(OUT)/stub.elf

$(OUT):
	mkdir -p "$@"

$(OUT)/stub.elf: stub/main.c | $(OUT)
	$(PPC)-gcc $(WII_FLAGS) -O2 -Wall -Wextra -Werror -I$(DEVKITPRO)/libogc/include $< -L$(DEVKITPRO)/libogc/lib/wii -lwiiuse -lbte -logc -lm -o "$@"

$(OUT)/apploader.elf: apploader.c apploader.ld | $(OUT)
	$(PPC)-gcc -mcpu=750 -meabi -m32 -mbig-endian -mno-sdata -ffreestanding -fno-builtin -fno-stack-protector -Os -Wall -Wextra -Werror -nostdlib -Wl,-T,apploader.ld apploader.c -o "$@"

$(OUT)/apploader.bin: $(OUT)/apploader.elf
	$(PPC)-objcopy -O binary "$<" "$@"

# With ELF=/path/to/game.elf this never builds or modifies the game sources.
package: $(ELF) $(OUT)/apploader.bin
	python3 package.py --elf "$(ELF)" --out "$(OUT)" --devkitpro "$(DEVKITPRO)"

smoke: package
	./smoke.sh "$(OUT)/BackyardCornhole-BYCEAO.iso"

# Delete only known intermediate files, keeping release artifacts.
clean:
	rm -f "$(OUT)/stub.elf" "$(OUT)/apploader.elf" "$(OUT)/apploader.bin"
