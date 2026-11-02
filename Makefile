# DesktopTweaks — build, install and remove the Ammonia tweaks in this repo.
#
# Every subdirectory is self-contained and carries its own Makefile, README
# and build notes. This file only delegates to them, so the whole set can be
# built or installed from one place without duplicating any build logic.

TWEAKS    = classic_titlebar firefox_titlebar liquid_glass_off

# Not every subdirectory provides every target; these lists keep the
# delegation honest rather than inventing targets that do not exist.
TESTABLE  = liquid_glass_off
UNINSTALL = firefox_titlebar liquid_glass_off

.PHONY: all install uninstall test clean $(TWEAKS)

# Build every tweak. Run this as yourself before `sudo make install` so the
# build products are not left owned by root.
all: $(TWEAKS)

$(TWEAKS):
	$(MAKE) -C $@

# Each subdirectory's install target invokes sudo itself, so this runs as
# yourself and prompts once sudo's timestamp expires. Override with
# `make install SUDO=` when already root.
install:
	@for d in $(TWEAKS); do $(MAKE) -C $$d install || exit 1; done

uninstall:
	@for d in $(UNINSTALL); do $(MAKE) -C $$d uninstall || exit 1; done
	@echo "classic_titlebar has no uninstall target; remove its files by hand:"
	@echo "  sudo rm -f /private/var/ammonia/core/tweaks/ClassicTitlebar.dylib*"

test:
	@for d in $(TESTABLE); do $(MAKE) -C $$d test || exit 1; done

clean:
	@for d in $(TWEAKS); do $(MAKE) -C $$d clean || exit 1; done
