#
# pi-fheroes2 — fheroes2 as a bootable bare-metal Raspberry Pi image.
#
#   make check-toolchain     report the cross compiler this build will use
#   make deps                the three circle-stdlib worlds and the shim
#                            archives built against them (long: the worlds
#                            build newlib and libc++ from source)
#   make deps-rpi4           the same for one board only, for a machine that
#                            cannot hold three worlds at once
#   make rpi5 | rpi4 | rpi3  one board's kernel image
#   make kernels             all three, built in parallel
#   make verify              truth-gate: every image exists and is non-empty
#   make media               download the freely redistributable demo data
#                            into media/
#   make netboot             stage the Pi 5 image and its boot configuration
#                            into build/netboot-rpi5/
#   make card                stage the whole card into build/sd-card/, copying
#                            in whatever media/ holds and naming what it does
#                            not. It never downloads anything
#   make clean-boards        drop every board's build tree
#
# The three boards never share mutable state: each has its own circle-stdlib
# world, its own shim archive and its own object directory, so building them
# at the same time is safe and building one never disturbs another.
#
# The libc++ sources every world is built from are one immutable git tag, and
# CIRCLE_LLVM says where that checkout lives. The default puts it beside this
# repository, which is right for a plain clone and for a CI runner. Point
# several projects at one directory to fetch it once for all of them:
#
#   make deps CIRCLE_LLVM=/path/to/circle-llvm
#

include mk/toolchain.mk

# Stated explicitly because the first rule this file sees comes from an
# included makefile, and that would otherwise decide the default goal.
.DEFAULT_GOAL := kernels

BOARDS ?= rpi3 rpi4 rpi5

IMAGE_rpi3 = kernel8.img
IMAGE_rpi4 = kernel8-rpi4.img
IMAGE_rpi5 = kernel_2712.img

.PHONY: deps kernels verify netboot card clean-boards $(BOARDS)
.PHONY: $(addprefix deps-,$(BOARDS))

deps:
	$(MAKE) -C circle-libsdl2 deps

# One board's dependencies: its own circle-stdlib world and the shim archive
# built against it. A machine with a small disk — a CI runner, most obviously
# — builds one board at a time and keeps only that board's world.
# Written as a static pattern rule over the board list rather than a plain
# pattern rule: these targets are phony, and make does not apply pattern rules
# to phony targets — it would quietly answer "nothing to be done" and leave
# the world unbuilt.
$(addprefix deps-,$(BOARDS)): deps-%:
	$(MAKE) -C circle-libsdl2 world BOARD=$*
	$(MAKE) -C circle-libsdl2 libSDL2-$*.a BOARD=$*

$(BOARDS): check-toolchain
	$(MAKE) -C host RAPI_BOARD=$@

# All three at once. Each sub-make owns a different world and a different
# output directory, so there is nothing for them to collide on.
#
# Each board is waited for BY PID, and its status kept. A bare `wait` reports
# only that the shell has no children left — it is success whatever the jobs
# did — so a board that failed to build would leave this target reporting
# success, and the truth-gate would then pass the board's PREVIOUS image,
# still on disk.
kernels: check-toolchain
	@pids=; fail=0; \
	for b in $(BOARDS); do $(MAKE) -C host RAPI_BOARD=$$b & pids="$$pids $$!"; done; \
	for p in $$pids; do wait $$p || fail=1; done; \
	exit $$fail

# Truth-gate: ask the filesystem, not the exit codes. An image that is
# missing or empty fails here even if the build claimed success.
verify:
	@fail=0; \
	for b in $(BOARDS); do \
		case $$b in \
			rpi3) img=host/build/rpi3/$(IMAGE_rpi3) ;; \
			rpi4) img=host/build/rpi4/$(IMAGE_rpi4) ;; \
			rpi5) img=host/build/rpi5/$(IMAGE_rpi5) ;; \
		esac; \
		if [ -s "$$img" ]; then \
			echo "  OK    $$img ($$(wc -c < $$img | tr -d ' ') bytes)"; \
		else \
			echo "  FAIL  $$img missing or empty"; fail=1; \
		fi; \
	done; \
	exit $$fail

# ---------------------------------------------------------------------------
# The game data
# ---------------------------------------------------------------------------
#
# Heroes of Might and Magic II's data is not this project's to ship, and the
# retail files are sold rather than given away. What IS freely available is
# the 1996 New World Computing/3DO demo, and fheroes2 itself ships a script to
# fetch it — script/demo/download_demo_version.sh in the game's own submodule.
# The URL and the SHA256 below are that script's own two constants, copied
# rather than sourced independently: the integrity check is upstream's, made
# for this exact archive.
#
# What arrives is the demo: one campaign map and the demo HEROES2.AGG. A user
# who owns the retail game replaces the same files with theirs — see README.md.
MEDIA_DIR = media

H2DEMO_ZIP    = $(MEDIA_DIR)/h2demo.zip
H2DEMO_URL    = https://archive.org/download/HeroesofMightandMagicIITheSuccessionWars_1020/h2demo.zip
H2DEMO_SHA256 = 12048c8b03875c81e69534a3813aaf6340975e77b762dc1b79a4ff5514240e3c

# The file whose presence means the demo is already unpacked, so a re-run
# verifies instead of downloading again.
H2DEMO_AGG = $(MEDIA_DIR)/data/HEROES2.AGG

# sha256sum on Linux, shasum on macOS. Whichever exists; if neither does the
# target stops rather than accepting a download it cannot check.
SHA256SUM := $(firstword $(shell command -v sha256sum 2>/dev/null) \
                         $(shell command -v shasum 2>/dev/null))

.PHONY: media
media:
	@if [ -z "$(SHA256SUM)" ]; then \
		echo "  MEDIA no checksum tool on this machine (sha256sum or shasum)"; \
		echo "        — refusing to download something that cannot be verified."; \
		exit 1; \
	fi
	@command -v unzip >/dev/null 2>&1 || { \
		echo "  MEDIA unzip is not on this machine, and the demo is a zip."; \
		exit 1; }
	@mkdir -p $(MEDIA_DIR)
	@if [ -f "$(H2DEMO_AGG)" ]; then \
		echo "  MEDIA $(MEDIA_DIR)/ already unpacked — verifying"; \
	else \
		if [ -f "$(H2DEMO_ZIP)" ]; then \
			echo "  MEDIA $(H2DEMO_ZIP) already here — verifying"; \
		else \
			echo "  MEDIA fetching $(H2DEMO_URL)"; \
			curl -fL --retry 3 -o "$(H2DEMO_ZIP).part" "$(H2DEMO_URL)" || { \
				rm -f "$(H2DEMO_ZIP).part"; \
				echo "  MEDIA download failed"; exit 1; }; \
			mv "$(H2DEMO_ZIP).part" "$(H2DEMO_ZIP)"; \
		fi; \
		got=`$(SHA256SUM) -a 256 "$(H2DEMO_ZIP)" 2>/dev/null || $(SHA256SUM) "$(H2DEMO_ZIP)"`; \
		got=`echo "$$got" | awk '{print $$1}'`; \
		if [ "$$got" != "$(H2DEMO_SHA256)" ]; then \
			echo "  MEDIA SHA256 MISMATCH for $(H2DEMO_ZIP)"; \
			echo "        expected $(H2DEMO_SHA256)"; \
			echo "        got      $$got"; \
			echo "        the file has been left in place for inspection, and"; \
			echo "        is NOT safe to put on a card."; \
			exit 1; \
		fi; \
		echo "  MEDIA $(H2DEMO_ZIP) verified against upstream's own SHA256"; \
		rm -rf $(MEDIA_DIR)/unpack; \
		mkdir -p $(MEDIA_DIR)/unpack $(MEDIA_DIR)/data $(MEDIA_DIR)/maps; \
		unzip -q -o "$(H2DEMO_ZIP)" -d $(MEDIA_DIR)/unpack; \
		cp $(MEDIA_DIR)/unpack/DATA/* $(MEDIA_DIR)/data/; \
		cp $(MEDIA_DIR)/unpack/MAPS/* $(MEDIA_DIR)/maps/; \
		rm -rf $(MEDIA_DIR)/unpack "$(H2DEMO_ZIP)"; \
		echo "  MEDIA unpacked into $(MEDIA_DIR)/data/ and $(MEDIA_DIR)/maps/"; \
	fi
	@if [ ! -f "$(H2DEMO_AGG)" ]; then \
		echo "  MEDIA $(H2DEMO_AGG) is missing after unpacking — the archive"; \
		echo "        did not contain what this target expects."; \
		exit 1; \
	fi
	@head -c 4 "$(H2DEMO_AGG)" | od -An -tu2 -N2 | awk '{ \
		if ($$1 > 0 && $$1 < 10000) exit 0; else exit 1 }' || { \
		echo "  MEDIA $(H2DEMO_AGG) does not begin with a plausible AGG record"; \
		echo "        count — the file is not what it claims to be."; exit 1; }
	@echo "  MEDIA $(H2DEMO_AGG) present ($$(wc -c < $(H2DEMO_AGG) | tr -d ' ') bytes)"
	@printf '%s\n' \
		"Heroes of Might and Magic II — the 1996 demo data" \
		"" \
		"Source:   $(H2DEMO_URL)" \
		"Item:     https://archive.org/details/HeroesofMightandMagicIITheSuccessionWars_1020" \
		"Fetched:  `date -u '+%Y-%m-%d %H:%M:%S UTC'`" \
		"SHA256:   $(H2DEMO_SHA256)  (h2demo.zip, the source archive)" \
		"" \
		"What it is: the official New World Computing/3DO demo release of" \
		"Heroes of Might and Magic II: The Succession Wars, from 1996." \
		"DATA/ and MAPS/ were copied out of the archive into data/ and maps/;" \
		"the Windows installer executable and its DLLs were discarded, as" \
		"fheroes2 reimplements the engine and reads only the data." \
		"" \
		"Licence: the demo was distributed freely to promote the retail game" \
		"and is still hosted on those terms. It is NOT the retail data." \
		"" \
		"Verification: the SHA256 above is the H2DEMO_SHA256 constant in" \
		"fheroes2's own script/demo/download_demo_version.sh, and the URL is" \
		"that script's H2DEMO_URL. Both are upstream's, checked against the" \
		"copy this target downloaded — not checksums invented here." \
		"" \
		"Heroes of Might and Magic is a trademark of Ubisoft Entertainment." \
		"This file is not redistributed by this repository." \
		> $(MEDIA_DIR)/provenance.txt
	@echo "  MEDIA provenance written to $(MEDIA_DIR)/provenance.txt"

# The Pi 5 netboot bundle: the image the Pi 5 firmware looks for, plus the
# boot configuration it must be served alongside. Copy the contents into the
# TFTP root the board boots from (the Raspberry Pi firmware files themselves
# come from that root's existing installation, not from here).
NETBOOT_DIR = build/netboot-rpi5
netboot: rpi5
	@mkdir -p $(NETBOOT_DIR)
	@cp host/build/rpi5/$(IMAGE_rpi5) $(NETBOOT_DIR)/
	@cp host/config.txt host/cmdline.txt $(NETBOOT_DIR)/
	@echo "  STAGED $(NETBOOT_DIR)/"
	@ls -l $(NETBOOT_DIR)/

# The card, staged into a directory to copy onto media formatted elsewhere:
# the three kernels, boot configuration, the parts of the game's data that
# fheroes2 SHIPS ITSELF and is free to redistribute — its own .h2d asset files
# and the scenarios written for it — and whatever game data media/ happens to
# hold.
#
# Everything belonging to this game lives in one directory on the card, named
# by RAPI_GAME_DIR in host/Makefile. A card carries several games, and two of
# them writing an `fheroes2.cfg` into the FAT root would each silently
# overwrite the other's. The two paths have to agree: the kernel enters this
# directory before the game starts, so data staged anywhere else is data the
# game never sees.
#
# This target downloads nothing. It copies what `make media` left and names
# what is absent.
CARD_DIR  = build/sd-card
CARD_GAME = $(CARD_DIR)/games/fheroes2

card: kernels
	@rm -rf $(CARD_DIR)
	@mkdir -p $(CARD_GAME)/data $(CARD_GAME)/maps $(CARD_GAME)/files/data
	@cp host/build/rpi3/$(IMAGE_rpi3) $(CARD_DIR)/
	@cp host/build/rpi4/$(IMAGE_rpi4) $(CARD_DIR)/
	@cp host/build/rpi5/$(IMAGE_rpi5) $(CARD_DIR)/
	@cp host/config.txt host/cmdline.txt $(CARD_DIR)/
	@cp host/fheroes2.cfg $(CARD_GAME)/
	@cp fheroes2/files/data/*.h2d $(CARD_GAME)/files/data/
	@cp fheroes2/maps/*.fh2m $(CARD_GAME)/maps/
	@echo "  STAGED $(CARD_DIR)/"
	@if [ -d "$(MEDIA_DIR)/data" ]; then \
		cp $(MEDIA_DIR)/data/* $(CARD_GAME)/data/ 2>/dev/null || true; \
	fi
	@if [ -d "$(MEDIA_DIR)/maps" ]; then \
		cp $(MEDIA_DIR)/maps/* $(CARD_GAME)/maps/ 2>/dev/null || true; \
	fi
	@for f in $(CARD_GAME)/data/*; do \
		if [ -f "$$f" ]; then echo "  DATA   `basename $$f`"; fi; \
	done; \
	for f in $(CARD_GAME)/maps/*; do \
		if [ -f "$$f" ]; then echo "  MAP    `basename $$f`"; fi; \
	done; \
	exit 0
	@echo
	@if [ -f "$(CARD_GAME)/data/HEROES2.AGG" ] \
	   || [ -f "$(CARD_GAME)/data/heroes2.agg" ]; then :; else \
		echo "  ABSENT no HEROES2.AGG. The game cannot start without it."; \
		echo "         Either the file from a copy of Heroes II you own, or"; \
		echo "         the free 1996 demo — 'make media' fetches that one."; \
	fi
	@echo "  NOTE   The Raspberry Pi firmware files are not staged here either."
	@echo "         See README.md."

# Board build trees and staged output only. media/ is not touched: it holds
# downloaded data, which no build target deletes.
clean-boards:
	@for b in $(BOARDS); do $(MAKE) -C host RAPI_BOARD=$$b clean-board; done
	rm -rf $(NETBOOT_DIR) $(CARD_DIR)
