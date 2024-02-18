# Project meta configuration
PROJECT_NAME := lyle
MDKROOT := mdk/mdk

# Project directories.
SRCDIR := src
RESDIR := res
OBJDIR := obj
CSPDIR := $(RESDIR)/csp

KOSMAPS := kosmaps
KOSCOMP := util/accurate-kosinski/kosinski-compress

# The physics need 8 bits of subpixel precision.
FLAGS = -D MD_FIXED_BITS=8

# Sources.
SOURCES_C := $(shell find $(SRCDIR)/ -type f -name '*.c')
SOURCES_ASM := $(shell find $(SRCDIR)/ -type f -name '*.s')

# MDK_WANT_ASM_OUT := 1
# TARGET_SYSTEM := MDK_TARGET_C2

EXTERNAL_DEPS = dispatch compress_mapdata \
	$(CSPDIR)/end_lyle.csp \
	$(CSPDIR)/end_cube_keddums.csp \
	$(RESDIR)/end_bg.bin
EXTERNAL_ARTIFACTS = obj_dispatch.inc $(KOSMAPS) $(RESDIR)/map $(CSPDIR)

include $(MDKROOT)/md-rules.mk

.PHONY: music obj_dispatch.inc compress_mapdata $(CSPDIR)/end_lyle.csp

music:
	cd music && ./convert-music.sh

obj_dispatch.inc:
	@bash -c 'printf " \e[34m[ GEN ]\e[0m $@\n"'
	@-python3 generate_obj_dispatch.py setup_funcs $(SRCDIR)/obj $@

dispatch: obj_dispatch.inc

compress_mapdata: $(KOSCOMP)
	./compress_mapdata.sh

$(KOSCOMP):
	git clone git@github.com:Clownacy/accurate-kosinski --recurse-submodules
	mv accurate-kosinski util/
	cd util/accurate-kosinski && cmake .
	cd util/accurate-kosinski && make

$(CSPDIR):
	mkdir -p $@

$(CSPDIR)/end_lyle.csp: $(PNG2CSP) $(CSPDIR)
	$(PNG2CSP) png/end_lyle.png 144 200 $@ "Ending Sprites 1" lt

$(CSPDIR)/end_cube_keddums.csp: $(PNG2CSP) $(CSPDIR)
	$(PNG2CSP) png/end_cube_keddums.png 96 112 $@ "Ending Sprites 2" lt

$(RESDIR)/end_bg.bin: $(MDTILER)
	$(MDTILER) -b png/end_bg.til
