# firefox_titlebar — Ammonia tweak that centers the titlebar text in Firefox.

TWEAK      = firefox_titlebar
DYLIB      = $(TWEAK).dylib
WHITELIST  = $(DYLIB).whitelist
TWEAK_DIR  = /private/var/ammonia/core/tweaks
SUDO      ?= sudo
CC         = clang
CFLAGS     = -Wall -Wextra -O2
ARCHS      = -arch arm64 -arch arm64e

.PHONY: all run install uninstall clean

all: $(DYLIB)

# Linking AppKit makes dyld load it before this library's constructor runs,
# which matters because Firefox's main executable loads AppKit late, via XUL.
# The linker's own adhoc,linker-signed signature is kept as-is. Re-signing
# with codesign produces a tool-signed form that macOS rejects at dlopen time
# in some hardened third-party processes.
$(DYLIB): $(TWEAK).c Makefile
	$(CC) $(CFLAGS) $(ARCHS) -dynamiclib -lobjc -framework AppKit -o $@ $<

# Relaunch Firefox so it picks up (or drops) the installed tweak.
run:
	-osascript -e 'tell application "Firefox" to quit'
	while pgrep -xq firefox; do sleep 0.2; done
	open -a Firefox

# Copy the tweak and its process whitelist into Ammonia's tweak directory.
# The dylib is written under a new name and renamed in, so a Firefox that
# already has the old file mapped keeps a valid signature until it relaunches.
install: $(DYLIB)
	$(SUDO) install -o root -g wheel -m 755 $(DYLIB) $(TWEAK_DIR)/$(DYLIB).new
	$(SUDO) mv -f $(TWEAK_DIR)/$(DYLIB).new $(TWEAK_DIR)/$(DYLIB)
	$(SUDO) install -o root -g wheel -m 644 $(WHITELIST) $(TWEAK_DIR)/$(WHITELIST)
	@echo "installed; run 'make run' to relaunch Firefox"

uninstall:
	$(SUDO) rm -f $(TWEAK_DIR)/$(DYLIB) $(TWEAK_DIR)/$(WHITELIST)
	@echo "removed; run 'make run' to relaunch Firefox"

clean:
	rm -f $(DYLIB)
