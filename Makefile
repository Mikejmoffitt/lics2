# Project meta configuration
PROJECT_NAME := lyle
MDKROOT := mdk/mdk

# Project directories.
SRCDIR := src
RESDIR := res
OBJDIR := obj

# Sources.
SOURCES_C := $(shell find $(SRCDIR)/ -type f -name '*.c')
SOURCES_ASM := $(shell find $(SRCDIR)/ -type f -name '*.s')
RESOURCES_LIST := $(shell find $(RESDIR)/ -type f -name '*')

EXTERNAL_DEPS = obj_dispatch.inc
EXTERNAL_ARTIFACTS = obj_dispatch.inc

include $(MDKROOT)/md-rules.mk

.PHONY: music obj_dispatch.inc

music:
	cd music && ./convert-music.sh

obj_dispatch.inc:
	@bash -c 'printf " \e[34m[ GEN ]\e[0m $@\n"'
	@-python3 generate_obj_dispatch.py setup_funcs $(SRCDIR)/obj $@

dispatch: obj_dispatch.inc
