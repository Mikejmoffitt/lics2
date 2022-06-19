# md-framework common build rules.

# Environment
MD_ENV := /opt/toolchains/gen/m68k-elf
GBIN := $(MD_ENV)/bin

CC_HOST := cc
CC := $(GBIN)/m68k-elf-gcc
AS := $(GBIN)/m68k-elf-gcc
LD := $(GBIN)/m68k-elf-ld
NM := $(GBIN)/m68k-elf-nm
OBJCOPY := $(GBIN)/m68k-elf-objcopy
BIN2S := $(UTILDIR)/bin2s
BIN2H := $(UTILDIR)/bin2h
MEGALOADER := $(UTILDIR)/megaloader
PNGTO := $(UTILDIR)/pngto
BLASTEM := $(UTILDIR)/blastem64-*/blastem

# If the user isn't overriding the emulator
ifeq ($(MDEMU),)
MDEMU := $(BLASTEM)
endif

# Compiler, assembler, and linker flag setup
CFLAGS += -Wno-strict-aliasing -ffreestanding
CFLAGS += -fomit-frame-pointer -fno-defer-pop -frename-registers -fshort-enums
CFLAGS += -mcpu=68000
CFLAGS += -I.
CFLAGS += -O3
CFLAGS += # -fno-store-merging # Needed to avoid breakage with GCC8.
CFLAGS += -ffunction-sections -fdata-sections -fconserve-stack
ASFLAGS := $(CFLAGS)
ASFLAGS := -Wa,-I$(SRCDIR) -Wa,-I$(OBJDIR) -Wa,-I$(COMMONSRCDIR)
# LDFLAGS := -L/usr/lib/gcc-cross/m68k-linux-gnu/8
LDFLAGS := -L$(MD_ENV)/m68k-elf/lib -L$(MD_ENV)/lib/gcc/m68k-elf/6.3.0
LDFLAGS += --gc-sections -nostdlib
LDFLAGS += -T$(LDSCRIPT)
LIBS += -lgcc

# Naming intermediates
OUTPUT_ELF := $(OBJDIR)/$(OUTPUT_FILE).elf
OUTPUT_UNPAD := $(OBJDIR)/$(OUTPUT_FILE).gen
OUTPUT_GEN := $(OUTPUT_FILE).gen
OBJECTS_C := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SOURCES_C)) \
  $(patsubst $(COMMONSRCDIR)/%.c,$(OBJDIR)/%.o,$(COMMONSOURCES_C))
OBJECTS_ASM := $(patsubst $(SRCDIR)/%.s,$(OBJDIR)/%.o,$(SOURCES_ASM)) \
  $(patsubst $(COMMONSRCDIR)/%.s,$(OBJDIR)/%.o,$(COMMONSOURCES_ASM))
OBJECTS_RES := $(OBJDIR)/res.o

RES_HEADER := res.h

.PHONY: all vars $(RES_HEADER)

# Generic var for additional files, etc. that are a build prereq.
EXTERNAL_DEPS ?=
EXTERNAL_ARTIFACTS ?=

all: $(BLASTEM) $(BINCLUDE) $(MEGALOADER) $(OUTPUT_GEN)

# Generic target that is intended to be overridden.
ext_deps: $(EXTERNAL_DEPS)

vars:
	@echo "CFLAGS is" "$(CFLAGS)"
	@echo "COMMONSOURCES_C is" "$(COMMONSOURCES_C)"
	@echo "COMMONSOURCES_ASM is" "$(COMMONSOURCES_ASM)"
	@echo "SOURCES_C is" "$(SOURCES_C)"
	@echo "SOURCES_ASM is" "$(SOURCES_ASM)"
	@echo "OBJECTS_C is" "$(OBJECTS_C)"
	@echo "OBJECTS_ASM is" "$(OBJECTS_ASM)"

# An archive for Blastem is included; this just unpacks it.
$(BLASTEM):
	cd $(UTILDIR) && tar -xf blastem64-*.tar.gz

$(MEGALOADER): $(UTILDIR)/megaloader.c
	@$(CC_HOST) -D_DEFAULT_SOURCE $< -o $@ $(HOSTCFLAGS)

$(BIN2S): $(UTILDIR)/bin2s.c
	@$(CC_HOST) $^ -o $@ -Os  $(HOSTCFLAGS)

$(BIN2H): $(UTILDIR)/bin2h.c
	@$(CC_HOST) $^ -o $@ -Os  $(HOSTCFLAGS)

$(PNGTO): $(UTILDIR)/pngto.c $(UTILDIR)/musl_getopt.c $(UTILDIR)/lodepng.c $(UTILDIR)/indexedimage.c
	@$(CC_HOST) $^ -o $@ -Os -DLODEPNG_NO_COMPILE_ENCODER $(HOSTCFLAGS)

$(OUTPUT_GEN): $(OUTPUT_ELF)
	@bash -c 'printf " \e[36m[ PAD ]\e[0m ... --> $@\n"'
	@$(OBJCOPY) -O binary $< $(OUTPUT_UNPAD)
	@dd if=$(OUTPUT_UNPAD) of=$@ bs=512 conv=sync status=none
	@rm $(OUTPUT_UNPAD)
	@bash -c 'printf "\e[92m [ OK! ]\e[0m --> $(OUTPUT_GEN)\n"'

$(OBJDIR)/$(OUTPUT_FILE).elf: $(OBJECTS_RES) $(OBJECTS_C) $(OBJECTS_ASM)
	@mkdir -p $(dir $@)
	@bash -c 'printf " \e[36m[ LNK ]\e[0m ... --> $@\n"'
	@$(LD) -o $@ $(LDFLAGS) $(OBJECTS_RES) $(OBJECTS_C) $(OBJECTS_ASM) $(LIBS)

$(OBJDIR)/%.o: $(SRCDIR)/%.c $(OBJECTS_RES) $(RES_HEADER) ext_deps
	@mkdir -p $(dir $@)
	@bash -c 'printf " \e[96m[  C  ]\e[0m $< --> $@\n"'
	@$(CC) $(CFLAGS) -c $< -o $@
	@$(CC) $(CFLAGS) -S $< -o $@.asm

$(OBJDIR)/%.o: $(SRCDIR)/%.s $(OBJECTS_RES) ext_deps
	@mkdir -p $(dir $@)
	@bash -c 'printf " \e[33m[ ASM ]\e[0m $< --> $@\n"'
	@$(AS) $(ASFLAGS) -c $< -o $@

$(OBJDIR)/%.o: $(COMMONSRCDIR)/%.c $(OBJECTS_RES) ext_deps
	@mkdir -p $(dir $@)
	@bash -c 'printf " \e[96m[ C:C ]\e[0m $< --> $@\n"'
	@$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/%.o: $(COMMONSRCDIR)/%.s $(OBJECTS_RES) ext_deps
	@mkdir -p $(dir $@)
	@bash -c 'printf " \e[33m[C:ASM]\e[0m $< --> $@\n"'
	@$(AS) $(ASFLAGS) -c $< -o $@

# Converts res.s and other intermediate files generated by util
$(OBJDIR)/%.o: $(OBJDIR)/%.s
	@bash -c 'printf " \e[33m[B:ASM]\e[0m $< --> $@\n"'
	@$(AS) $(ASFLAGS) -c $< -o $@

# Converts a file to object files
$(OBJDIR)/res.s: $(BIN2S) $(RESOURCES_LIST)
	@mkdir -p $(dir $@)
	@bash -c 'printf " \e[95m[ BIN ]\e[0m $^ --> $@\n"'
	@$^ > $@

# Generates header entries for resource data
$(RES_HEADER): $(BIN2H) $(RESOURCES_LIST)
	@bash -c 'printf " \e[95m[RES.H]\e[0m $^ --> $@\n"'
	@printf '#ifndef _RES_AUTOGEN_H\n#define _RES_AUTOGEN_H\n' > $@
	@$^ >> $@
	@printf '#endif  // _RES_AUTOGEN_H\n' >> $@

res_post:
	@printf

flash: all
	@exec $(MEGALOADER) md $(OUTPUT_GEN) /dev/ttyUSB0 2> /dev/null

debug: all
	@exec $(MDEMU) -m gen -d $(OUTPUT_GEN)

test: all
	@exec $(MDEMU) -m gen $(OUTPUT_GEN)

mame: all
	@exec mame megadrij -cart $(OUTPUT_GEN) -debug -r 640x480

clean:
	@-rm -f $(OBJECTS_C) $(OBJECTS_ASM) $(OUTPUT_GEN)
	@-rm -f $(OUTPUT_ELF) $(OUTPUT_UNPAD)
	@-rm -f $(OBJECTS_RES) $(OBJDIR)/res.s $(RES_HEADER)
	echo $(EXTERNAL_ARTIFACTS) | xargs --no-run-if-empty rm -f $(EXTERNAL_ARTIFACTS)
