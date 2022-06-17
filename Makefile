# Configuration
OUTPUT_FILE := lyle
OUTPUT_EXT := gen
OUTPUT_VERSION := wip

CPUTYPE := 68000
SRCDIR := src
COMMONSRCDIR := common/src
RESDIR := res
OBJDIR := obj
UTILDIR := util
CFLAGS := -I$(SRCDIR) -I$(OBJDIR) -I$(COMMONSRCDIR)
CFLAGS += -Wall -Wextra -O3 -std=c11 -Wno-unused-function
CFLAGS += -fshort-enums

HOSTCFLAGS := -Os -std=c11
LDSCRIPT := common/md.ld
SOURCES_C := $(shell find $(SRCDIR)/ -type f -name '*.c')
SOURCES_ASM := $(shell find $(SRCDIR)/ -type f -name '*.s')
COMMONSOURCES_C := $(shell find $(COMMONSRCDIR)/ -type f -name '*.c')
COMMONSOURCES_ASM := $(shell find $(COMMONSRCDIR)/ -type f -name '*.s')
RESOURCES_LIST := $(shell find $(RESDIR)/ -type f -name '*')

EXTERNAL_DEPS = obj_dispatch.inc
EXTERNAL_ARTIFACTS = obj_dispatch.inc

include common/md-rules.mk

LDFLAGS += -Map lyle.map

.PHONY: music obj_dispatch.inc

music:
	cd music && ./convert-music.sh

obj_dispatch.inc:
	@bash -c 'printf " \e[34m[ GEN ]\e[0m $@\n"'
	@-python3 generate_obj_dispatch.py setup_funcs $(SRCDIR)/obj $@

dispatch: obj_dispatch.inc
