# DesktopTweaks — build, install and remove the Ammonia tweaks in this repo.
#
# Every subdirectory is self-contained and carries its own Makefile, README
# and build notes. This file only delegates to them, so the whole set can be
# built or installed from one place without duplicating any build logic.

TWEAKS    = classic_titlebar firefox_titlebar liquid_glass_off

# liquid_glass_off is the only subdirectory with a test target; the rest of
# the interface is the same in all three.
TESTABLE  = liquid_glass_off

.PHONY: all install install-whitelist uninstall test clean $(TWEAKS)

# Build every tweak. Both this and `install` run as yourself; install invokes
# sudo only for the steps that write into Ammonia's root-owned tweak
# directory, so the build products are never left owned by root.
all: $(TWEAKS)

$(TWEAKS):
	$(MAKE) -C $@

# Each subdirectory's install target invokes sudo itself, so this runs as
# yourself and prompts once sudo's timestamp expires. Override with
# `make install SUDO=` when already root.
install:
	@for d in $(TWEAKS); do $(MAKE) -C $$d install || exit 1; done

# Replace the installed whitelists with this repo's copies. `install` leaves an
# existing whitelist alone, so this is the way to push whitelist changes out.
install-whitelist:
	@for d in $(TWEAKS); do $(MAKE) -C $$d install-whitelist || exit 1; done

uninstall:
	@for d in $(TWEAKS); do $(MAKE) -C $$d uninstall || exit 1; done

test:
	@for d in $(TESTABLE); do $(MAKE) -C $$d test || exit 1; done

clean:
	@for d in $(TWEAKS); do $(MAKE) -C $$d clean || exit 1; done
