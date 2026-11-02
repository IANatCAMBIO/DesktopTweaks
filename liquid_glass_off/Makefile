# GoAwayLiquidGlass — Ammonia tweak that turns Liquid Glass off per process.

TWEAK      = liquid_glass_off
DYLIB      = $(TWEAK).dylib
WHITELIST  = $(DYLIB).whitelist
PROBE      = solarium_probe
TWEAK_DIR ?= /private/var/ammonia/core/tweaks
SUDO      ?= sudo
CC         = clang
CFLAGS     = -Wall -Wextra -O2
ARCHS      = -arch arm64 -arch arm64e

.PHONY: all run install install-whitelist uninstall test clean

all: $(DYLIB) $(PROBE)

$(DYLIB): $(TWEAK).c
	$(CC) $(CFLAGS) $(ARCHS) -dynamiclib -install_name @rpath/$(DYLIB) -o $@ $<
	codesign -s - -f $@

$(PROBE): $(PROBE).c
	$(CC) $(CFLAGS) -o $@ $<

# Show the Liquid Glass state of a fresh process with and without the tweak.
run: all
	@echo "== without tweak"; ./$(PROBE)
	@echo "== with tweak";    ./$(PROBE) ./$(DYLIB)

# Fail unless the tweak flips every probe from ON to OFF.
test: all
	./$(PROBE) | grep -q ON
	./$(PROBE) ./$(DYLIB)
	! ./$(PROBE) ./$(DYLIB) | grep -q ON

# Copy the tweak and its process whitelist into Ammonia's tweak directory.
# Files are written under a new name and renamed in, so an app that already
# has the old dylib mapped keeps a valid code signature until it relaunches.
install: $(DYLIB)
	$(SUDO) install -o root -g wheel -m 755 $(DYLIB) $(TWEAK_DIR)/$(DYLIB).new
	$(SUDO) mv -f $(TWEAK_DIR)/$(DYLIB).new $(TWEAK_DIR)/$(DYLIB)
	@if [ -f "$(TWEAK_DIR)/$(WHITELIST)" ]; then \
		echo "Keeping existing $(TWEAK_DIR)/$(WHITELIST)"; \
	else \
		$(SUDO) install -o root -g wheel -m 644 $(WHITELIST) $(TWEAK_DIR)/$(WHITELIST); \
		echo "Installed default $(TWEAK_DIR)/$(WHITELIST)"; \
	fi
	@echo "installed; relaunch the whitelisted apps to pick it up"

# `install` never overwrites a whitelist that is already in place, so that
# whitelist edits made directly in the tweak directory survive a reinstall.
# This target is the deliberate override: it replaces the installed whitelist
# with this repo's copy.
install-whitelist:
	$(SUDO) install -o root -g wheel -m 644 $(WHITELIST) $(TWEAK_DIR)/$(WHITELIST).new
	$(SUDO) mv -f $(TWEAK_DIR)/$(WHITELIST).new $(TWEAK_DIR)/$(WHITELIST)
	@echo "whitelist replaced; relaunch the affected apps"

uninstall:
	$(SUDO) rm -f $(TWEAK_DIR)/$(DYLIB) $(TWEAK_DIR)/$(WHITELIST)
	@echo "removed; relaunch the whitelisted apps to drop it"

clean:
	rm -f $(DYLIB) $(PROBE)
