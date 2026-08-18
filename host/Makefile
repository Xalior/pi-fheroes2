#
# fheroes2 as a bootable Raspberry Pi kernel image.
#
# One board per invocation (RAPI_BOARD, default rpi5). The board selects
# three things at once: the circle-stdlib world the kernel is compiled and
# linked against, the shim archive built against that world, and — through
# the world's Config.mk — the CPU tuning and RASPPI value. Each board's
# newlib and libc++ are built for its own architecture, so an image is
# never portable between boards.
#
#   make                     the default board's image
#   make RAPI_BOARD=rpi3     that board's image
#   make clean-board         drop just this board's build/<board>/ tree
#
# Everything this invocation generates lands under build/<board>/: the
# kernel's own objects, every fheroes2 object, every zlib object, and the
# final .elf/.map/.lst/.img. The object names are shared across boards but
# their contents are not (different -mcpu, different -DRASPPI, different
# world), so a flat build directory would silently relink one board's
# objects into another board's kernel — a corrupt image with no build error.
# A directory per board keeps all three isolated: no `make clean` between
# boards, and three boards can build at the same time without touching each
# other.
#
# fheroes2's own source tree (../fheroes2) and zlib's (../zlib) are never
# written into: they are read-only input, and every object they produce is
# redirected here.
#
# Prerequisite: the board's world and shim archive exist —
# `make -C ../circle-libsdl2 deps`, or `make deps` at the repository root.
#

include ../mk/toolchain.mk

RAPI_BOARD ?= rpi5

# WHERE THE BUILT WORLD COMES FROM.
#
# The default is this game's own nested shim, so a clone of this repository
# alone builds and uses its own worlds and nothing needs setting. A parent
# that keeps ONE set of built worlds for several games — they are gigabytes
# each, and identical when the pin and the configuration match — points
# CIRCLE_WORLDS at them.
#
# Named before the include below rather than beside SHIM further down,
# because make reads an include when it reaches it.
CIRCLE_WORLDS ?= ../circle-libsdl2
WORLD = $(CIRCLE_WORLDS)/circle-stdlib-$(RAPI_BOARD)

include $(WORLD)/Config.mk

ROOT = ..
# WHERE THE SHIM COMES FROM.
#
# Unset, this repository's own pinned copy — what a clone of this repository
# gets, and what CI builds. A parent proving one shim across several games
# points SHIM at the single checkout it is proving, so the library is built
# once rather than once per game.
SHIM ?= $(ROOT)/circle-libsdl2
FH2  = $(ROOT)/fheroes2
ZLIB = $(ROOT)/zlib

OBJDIR = build/$(RAPI_BOARD)

# Circle's own image names, which the Raspberry Pi firmware selects between
# from the model sections of config.txt: the Pi 3 world boots kernel8.img,
# the Pi 4 world kernel8-rpi4.img, the Pi 5 world kernel_2712.img.
IMAGE_rpi3 = kernel8
IMAGE_rpi4 = kernel8-rpi4
IMAGE_rpi5 = kernel_2712
IMAGE = $(IMAGE_$(RAPI_BOARD))
ifeq ($(IMAGE),)
$(error unknown RAPI_BOARD '$(RAPI_BOARD)' — expected rpi3, rpi4 or rpi5)
endif

TARGET = $(OBJDIR)/$(IMAGE)

# Stated explicitly because the first rule this file sees comes from an
# included makefile, and that would otherwise decide the default goal.
.DEFAULT_GOAL := image

# ---------------------------------------------------------------------------
# Sources
# ---------------------------------------------------------------------------

# The kernel wrapper: C++, and this project's own code. There is no SDL layer
# here — circle-libsdl2 implements SDL2, SDL_image and SDL_mixer, and anything
# this port added would be a second copy of them — and the file syscalls are
# its circle-syscallwrap component, included below rather than written here.
HOST_SRCS = main.cpp kernel.cpp defaults.cpp
HOST_OBJS = $(addprefix $(OBJDIR)/,$(HOST_SRCS:.cpp=.o))

# fheroes2's own source list, taken the way upstream's CMakeLists takes it:
# every .cpp under src/engine and every .cpp under src/fheroes2. Upstream
# globs, so this globs, and a file added upstream is picked up by a pin bump
# with nothing here to edit. The two directories are laid out flat and one
# level deep respectively, which is why two patterns cover them.
FH2_ENGINE_SRCS = $(wildcard $(FH2)/src/engine/*.cpp)
FH2_GAME_SRCS   = $(wildcard $(FH2)/src/fheroes2/*/*.cpp)

# The Smacker video decoder that plays the game's introduction, vendored by
# upstream and built by upstream's own build as a library of one file.
SMACKER_SRCS = $(FH2)/src/thirdparty/libsmacker/smacker.c

FH2_SRCS = $(FH2_ENGINE_SRCS) $(FH2_GAME_SRCS)
FH2_OBJS = $(patsubst $(FH2)/%.cpp,$(OBJDIR)/fh2/%.o,$(FH2_SRCS))
SMACKER_OBJS = $(patsubst $(FH2)/%.c,$(OBJDIR)/fh2/%.o,$(SMACKER_SRCS))

# zlib, which fheroes2 requires: its saved games, its high scores, its map
# format and its own .h2d asset files are all deflate streams, and the AGG
# reader checksums with crc32. Upstream's build finds a system zlib; there is
# no system here, so zlib is a submodule of this repository and its sources
# are compiled into the image alongside everything else.
#
# The gz*.c files are left out. They are zlib's stdio wrapper — gzopen,
# gzprintf and the rest — reading and writing .gz files through FILE
# streams. Nothing in fheroes2 calls them, and leaving them out is what
# keeps a bare-metal build from carrying a file layer it never uses.
ZLIB_SRCS = adler32.c compress.c crc32.c deflate.c infback.c inffast.c \
	inflate.c inftrees.c trees.c uncompr.c zutil.c
ZLIB_OBJS = $(addprefix $(OBJDIR)/zlib/,$(ZLIB_SRCS:.c=.o))

OBJS = $(HOST_OBJS) $(FH2_OBJS) $(SMACKER_OBJS) $(ZLIB_OBJS)

# fheroes2 is written to the 2017 standard and says so in its CMakeLists.
STANDARD = -std=gnu++17

# Stated, not inherited: this is Circle's own default, written down so the
# build's optimization level is a fact in the file rather than a forensic
# reconstruction.
OPTIMIZE = -O2

# Circle's Rules.mk defines a link rule for $(TARGET).img with its own linker
# script; sdl-app.mk below defines the one this port actually uses. Two
# recipes for one target makes make warn on every board of every build — a
# warning that reads like a fault and is not one. Pointing TARGET at a name
# nothing builds while Rules.mk is read attaches its rule there instead, so
# there is only ever one recipe for the image.
SDL_APP_IMAGE := $(TARGET)
TARGET := $(OBJDIR)/.circle-unused

# The SD card's read cache, which lives in circle-libsdl2 rather than here.
#
# It is not part of SDL and does not use it: it is Circle device-layer code
# that wraps the card and is picked up by name, so FatFs and every file the
# game opens go through it without knowing.
#
# On the line before Rules.mk because that is where its own makefile fragment
# says to put it: Rules.mk works out its dependency list from OBJS as it reads
# it, so a source added afterwards is one make never checks. The fragment
# finds its own directory and adds everything else itself.
include $(SHIM)/circle-diskcache/diskcache.mk

# The file syscalls, redirected at link time.
#
# The game reads its AGG archives, its maps, its configuration and its save
# games through the C library and through std::filesystem — open/read,
# opendir/readdir, stat, mkdir — and it runs on the application core.
# newlib's Circle glue drives FatFs and the SD card straight from wherever it
# is called, which off core 0 is illegal, and fheroes2 has no file layer of
# its own to redirect. This component puts the shim's any-core I/O service
# underneath the C library instead.
#
# It brings its own --wrap flags, emitted from the same symbol list its
# source implements, so there is no list to keep in step here. The mechanism,
# and the three things about it that are easy to get wrong, are in
# circle-libsdl2/docs/CORE-SPLIT.md.
#
# NOTHING OF SDL IS WRAPPED, here or there, and nothing of SDL ever should
# be. A --wrap renames a reference rather than defining a second symbol, so
# the whole-archive link below — which turns a leftover local copy of a
# library function into a duplicate-symbol error — cannot see one. A wrapped
# SDL function is therefore the one kind of private SDL that could sit here
# for years without anything saying so. Whatever the game needs from SDL, the
# library provides; where it does not, the library is what gets fixed.
#
# On the line before Rules.mk for the same reason the disk cache is.
include $(SHIM)/circle-syscallwrap/syscallwrap.mk

include $(CIRCLEHOME)/Rules.mk
TARGET := $(SDL_APP_IMAGE)

# The world must be the multicore one circle-libsdl2 configures: the shim's
# core-split machinery cannot exist without it, and an object compiled
# against a single-core world links without complaint and then misbehaves.
# Every object here inherits the flag through Rules.mk's CPPFLAGS, so this
# check covers the whole build.
ifeq ($(findstring ARM_ALLOW_MULTI_CORE,$(DEFINE)),)
$(error the payload requires the multicore circle-stdlib world — build it with 'make -C ../circle-libsdl2 deps')
endif

# WHERE THIS GAME LIVES ON THE CARD. One card carries several games, and two
# of them writing an `fheroes2.cfg` into the FAT root would each silently
# overwrite the other's. Every game therefore keeps to a directory of its
# own, and nothing of this port's ever touches the root.
#
# It is a build-time constant, not a boot switch: a card's layout is decided
# when the card is staged, and a game that could be relocated at boot would
# only mean two places to look for the same file.
RAPI_GAME_DIR ?= /games/fheroes2
DEFINE += -DRAPI_GAME_DIR='"$(RAPI_GAME_DIR)"'

# The same path, told to upstream through upstream's OWN knob. fheroes2
# already has a compile-time setting for where its data lives — CMake calls
# it FHEROES2_DATA and passes it as a macro — and the game puts it FIRST in
# the list of directories it searches. So there is no patch here and nothing
# to imitate: the game is simply pointed.
#
# Unquoted, because the game stringifies it itself.
DEFINE += -DFHEROES2_DATA=$(RAPI_GAME_DIR)

# fheroes2 writes `#include <SDL_touch.h>` — the spelling a desktop
# sdl2-config include path provides — so the shim's SDL2/ directory goes on
# the path in its own right alongside the SDL2/-prefixed one this project's
# own code uses.
#
# The game's own directories are on the path the way its build system puts
# them there: src/engine for the engine headers, every immediate
# subdirectory of src/fheroes2 for the game's, and libsmacker for the video
# decoder's.
# Derived from the files rather than from the directories: this make's
# $(wildcard .../*/) matches plain files as well as directories, and a
# CMakeLists.txt handed to -I is a warning on every compile in the build.
FH2_INCLUDE_DIRS = $(FH2)/src/engine \
	$(sort $(dir $(wildcard $(FH2)/src/fheroes2/*/*))) \
	$(FH2)/src/thirdparty/libsmacker

INCLUDE := -I $(SHIM)/include -I $(SHIM)/include/SDL2 \
	-I $(CURDIR)/sdl2ext \
	$(addprefix -I ,$(FH2_INCLUDE_DIRS)) \
	-I $(ZLIB) \
	$(CIRCLE_STDLIB_INCLUDES) $(INCLUDE)

# zlib is C89 and warns about everything a modern compiler notices in it. It
# is upstream's code, unmodified, so the warnings are not this project's to
# answer — and left on they bury the ones that are.
ZLIB_CFLAGS = -std=gnu99 -w $(CFLAGS)

# libsmacker is C99 and is upstream's too, on the same terms.
SMACKER_CFLAGS = -std=gnu99 -w $(CFLAGS)

# Two warnings turned off for the whole build, both raised inside libc++'s
# own headers rather than in any source here.
#
#   -Wmaybe-uninitialized       reported against libc++'s hash table after
#                               the optimiser has inlined six levels of
#                               template into one function
#   -Walloc-size-larger-than    reported against libc++'s vector growth,
#                               where the compiler reasons about a branch
#                               that cannot be taken
#
# Each arrives as thirty lines of instantiated template names, several times
# per file, and between them they bury every diagnostic that is about this
# project's own code — which is the only reason a build log is kept.
CPPFLAGS += -Wno-maybe-uninitialized -Wno-alloc-size-larger-than

# The flags fheroes2's OWN sources are compiled with: everything above, and
# one directory placed AHEAD of all the others. fh2compat/ holds headers that
# resolve a name collision between Circle and upstream, and each works by
# interposing on a header of Circle's — which only happens if this directory
# is searched first, so it is prepended rather than appended. It is kept off
# this port's own compiles because the resolution is only right for one side
# of the collision — see the comment in fh2compat/assert.h.
FH2_CPPFLAGS = -I $(CURDIR)/fh2compat $(CPPFLAGS)

# The shim comes in whole, every member, rather than only the members the
# link happens to need. An object file linked straight into the kernel beats
# an archive member of the same name and does it silently, so a stub left
# behind here after the library implemented the real thing would quietly win
# — the game would keep calling the stub and nothing would say so. Pulling
# the archive in whole turns that into a duplicate-symbol error at link
# time, which is the only moment it can still be cheap to fix.
#
# That is what it did: the port's own SDL layer and its stand-in for
# SDL_mixer were both named this way when the library grew the real
# implementations, and both are now gone.
LIBS = --whole-archive $(SHIM)/libSDL2-$(RAPI_BOARD).a --no-whole-archive \
	$(CIRCLE_STDLIB_LIBS)

# ---------------------------------------------------------------------------
# Compilation
# ---------------------------------------------------------------------------

# This project's own objects, board-scoped. These more-specific rules win
# over Rules.mk's in-place %.o/%.d, so nothing is written beside the source.
$(OBJDIR)/%.o: %.cpp | $(OBJDIR)
	@echo "  CPP   $@"
	@$(CPP) $(CPPFLAGS) -c -o $@ $<

$(OBJDIR)/%.d: %.cpp | $(OBJDIR)
	@$(CPP) $(CPPFLAGS) -M -MG -MT $(@:.d=.o) -MT $@ -MF $@ $<

# fheroes2's objects, mirroring its source tree under $(OBJDIR)/fh2/. The
# source tree itself is the upstream submodule, and is never written to.
$(OBJDIR)/fh2/%.o: $(FH2)/%.cpp
	@mkdir -p $(dir $@)
	@echo "  CPP   $@"
	@$(CPP) $(FH2_CPPFLAGS) -c -o $@ $<

$(OBJDIR)/fh2/%.d: $(FH2)/%.cpp
	@mkdir -p $(dir $@)
	@$(CPP) $(FH2_CPPFLAGS) -M -MG -MT $(@:.d=.o) -MT $@ -MF $@ $<

# fheroes2's main() is the program's entry point on a desktop, but here
# main() belongs to the Circle kernel (main.cpp in this directory), which
# calls the game once the board is up. Renaming the entry point through the
# preprocessor for this one translation unit keeps upstream's source
# untouched, and the kernel calls the new name.
$(OBJDIR)/fh2/src/fheroes2/game/fheroes2.o: $(FH2)/src/fheroes2/game/fheroes2.cpp
	@mkdir -p $(dir $@)
	@echo "  CPP   $@ (entry point renamed)"
	@$(CPP) $(FH2_CPPFLAGS) -Dmain=fheroes2_main -c -o $@ $<

$(OBJDIR)/fh2/src/fheroes2/game/fheroes2.d: $(FH2)/src/fheroes2/game/fheroes2.cpp
	@mkdir -p $(dir $@)
	@$(CPP) $(FH2_CPPFLAGS) -Dmain=fheroes2_main -M -MG -MT $(@:.d=.o) -MT $@ -MF $@ $<

# libsmacker, upstream's vendored copy, built with the C flags its own
# library target gets.
$(OBJDIR)/fh2/%.o: $(FH2)/%.c
	@mkdir -p $(dir $@)
	@echo "  CC    $@"
	@$(CC) $(SMACKER_CFLAGS) -c -o $@ $<

$(OBJDIR)/fh2/%.d: $(FH2)/%.c
	@mkdir -p $(dir $@)
	@$(CC) $(SMACKER_CFLAGS) -M -MG -MT $(@:.d=.o) -MT $@ -MF $@ $<

# zlib, from its own submodule.
$(OBJDIR)/zlib/%.o: $(ZLIB)/%.c
	@mkdir -p $(dir $@)
	@echo "  CC    $@"
	@$(CC) $(ZLIB_CFLAGS) -c -o $@ $<

$(OBJDIR)/zlib/%.d: $(ZLIB)/%.c
	@mkdir -p $(dir $@)
	@$(CC) $(ZLIB_CFLAGS) -M -MG -MT $(@:.d=.o) -MT $@ -MF $@ $<

$(OBJDIR):
	@mkdir -p $(OBJDIR)

-include $(DEPS)

# The image link rule. sdl-app.mk lets the caller choose the linker script,
# so this build supplies its own: the shim's script with room made at image
# offset 0x800 for the patchable-defaults block. Everything else about the
# script — including the TLS-section adjacency binutils 2.44 and later
# require — is inherited from the shim's.
SDL_APP_LDSCRIPT = $(CURDIR)/fheroes2-defaults.ld
include $(SHIM)/sdl-app.mk

# The other half of the guarantee the linker script's ASSERTs begin: check
# the BYTES of the finished image, not just the link. A pre-boot writer
# refuses to stamp an image whose magic is not at 0x800, so an image that
# lost it would boot and then quietly ignore everything ever sent to it —
# including --rapi-debug-uart, which is the channel used to drive the board
# when nobody is at the bench. Better to have no image than that one.
.PHONY: image
image: $(TARGET).img
	@if [ "`dd if=$(TARGET).img bs=4 skip=512 count=1 2>/dev/null`" = "PM8D" ]; \
	then \
		echo "  GATE  $(TARGET).img defaults block at 0x800: present"; \
	else \
		echo "  GATE  $(TARGET).img: defaults block ABSENT at 0x800 — image deleted"; \
		rm -f $(TARGET).img; \
		exit 1; \
	fi

# Board-scoped clean: drop just this board's tree, leaving the other two
# boards' images and objects untouched.
.PHONY: clean-board
clean-board:
	rm -rf $(OBJDIR)
