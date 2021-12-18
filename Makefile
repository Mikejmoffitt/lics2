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

include common/md-rules.mk

LDFLAGS += -Map lyle.map

.PHONY: music

music:
	cd music && ./convert-music.sh
