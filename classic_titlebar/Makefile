SDK ?= macosx
ARCHS ?= -arch arm64 -arch arm64e -arch x86_64
OUT ?= ClassicTitlebar.dylib
SRC ?= ClassicTitlebar.m
WHITELIST ?= $(OUT).whitelist
TWEAK_DIR ?= /private/var/ammonia/core/tweaks
SUDO ?= sudo

.PHONY: all install install-whitelist uninstall clean

all: $(OUT)

$(OUT): $(SRC)
	clang -dynamiclib -fobjc-arc \
		-isysroot "$$(xcrun --sdk $(SDK) --show-sdk-path)" \
		-framework Cocoa -framework QuartzCore \
		$(ARCHS) \
		-Wall -Wextra -Wno-unused-parameter \
		-o $(OUT) $(SRC)

# The linker already emits an adhoc,linker-signed signature for arm64/arm64e
# slices. Re-running `codesign --sign -` afterward rewrites that signature
# into a tool-signed form with extra special slots, which the macOS page
# validator rejects on load in some processes (e.g. Slack — SIGKILL "Invalid
# Page" during dlopen). Leave the linker's signature alone.

# The kernel caches the dylib's code signature per-inode. `cp` overwrites
# in place (same inode, new mtime), which leaves the cached cs_mtime out
# of sync with the file's mtime and causes the page validator to reject
# every subsequent dlopen with "Invalid Page" in dmesg. Copy to a sidecar
# path and rename(2) into place so the install path gets a fresh inode —
# the kernel then reads the signature from scratch.
install: $(OUT)
	$(SUDO) mkdir -p "$(TWEAK_DIR)"
	$(SUDO) install -o root -g wheel -m 755 $(OUT) "$(TWEAK_DIR)/$(OUT).new"
	$(SUDO) mv -f "$(TWEAK_DIR)/$(OUT).new" "$(TWEAK_DIR)/$(OUT)"
	@if [ -f "$(TWEAK_DIR)/$(WHITELIST)" ]; then \
		echo "Keeping existing $(TWEAK_DIR)/$(WHITELIST)"; \
	else \
		$(SUDO) install -o root -g wheel -m 644 $(WHITELIST) "$(TWEAK_DIR)/$(WHITELIST)"; \
		echo "Installed default $(TWEAK_DIR)/$(WHITELIST)"; \
	fi
	@echo "installed; relaunch the whitelisted apps to pick it up"

# `install` never overwrites a whitelist that is already in place, so that
# whitelist edits made directly in the tweak directory survive a reinstall.
# This target is the deliberate override: it replaces the installed whitelist
# with this repo's copy.
install-whitelist:
	$(SUDO) install -o root -g wheel -m 644 $(WHITELIST) "$(TWEAK_DIR)/$(WHITELIST).new"
	$(SUDO) mv -f "$(TWEAK_DIR)/$(WHITELIST).new" "$(TWEAK_DIR)/$(WHITELIST)"
	@echo "whitelist replaced; relaunch the affected apps"

uninstall:
	$(SUDO) rm -f "$(TWEAK_DIR)/$(OUT)" "$(TWEAK_DIR)/$(WHITELIST)"
	@echo "removed; relaunch the affected apps to drop it"

clean:
	rm -f $(OUT)
