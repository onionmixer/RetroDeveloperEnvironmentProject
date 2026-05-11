# spman-z88dk — z88dk-compatible source distribution of Juan J. Martinez's
# spman sprite manager.
#
# Note (2026-04-29): unlike ubox-msx-lib-z88dk (z80asm-based), spman is a
# single C file that uses ubox_wait_vsync() macro expanding to inline asm.
# Standalone `zcc -c` (compile-to-object) trips on the macro expansion in
# sccz80, so we DO NOT build a .lib here. Instead, examples include
# `src/spman/spman.c` directly in their SRCS via `-I include` + the source
# path, so that zcc compiles the whole app in one pass (which handles the
# inline asm correctly).
#
# This Makefile is kept as a placeholder; future work could try the +msx
# clib path or porting spman.c to z80asm.

all:
	@echo "spman-z88dk uses direct source inclusion — see Examples/.../10_breakout/compile*.sh"
	@echo "Headers: $(CURDIR)/include"
	@echo "Source:  $(CURDIR)/src/spman/spman.c"

clean:
	@true

.PHONY: all clean
