# Building

## Toolchain

* macOS 26 with Xcode Command Line Tools (`xcode-select --install`); tested
  with the clang shipped with Xcode 26 on macOS 26.6.
* No third-party libraries. The tweak is plain C over the Objective-C runtime
  (`libobjc`) and libdispatch, and resolves every AppKit class and selector by
  name. It links AppKit anyway, without using a symbol from it, so dyld loads
  AppKit before the constructor runs: Firefox's main executable is a stub that
  loads AppKit late through XUL, and an inserted library that does not link
  AppKit itself would initialize before `NSThemeFrame` exists.

## Targets

| target           | what it does                                                             |
|------------------|--------------------------------------------------------------------------|
| `make`           | builds `firefox_titlebar.dylib` (arm64 + arm64e)                         |
| `make run`       | quits and relaunches Firefox so it picks up or drops the installed tweak |
| `make install`   | copies the dylib and whitelist into `/private/var/ammonia/core/tweaks` (needs sudo) |
| `make uninstall` | removes them again (needs sudo)                                          |
| `make clean`     | deletes the built dylib                                                  |

`SUDO` can be overridden, for example `make install SUDO="sudo -A"`.

## Files

* `firefox_titlebar.c` — the tweak. A library constructor swaps the
  implementation of `-[NSThemeFrame _defaultTitlebarTitleRect]` for one that
  returns the centered rect for Firefox browser windows, then re-runs the
  title layout of any browser window that already exists.
* `firefox_titlebar.dylib.whitelist` — process names the tweak loads into.
  Ammonia reads `<dylib>.whitelist` next to the dylib.

## Platform notes

* Firefox runs as `arm64`; the `arm64e` slice is there so the same binary can
  be loaded into Apple's own processes for diagnosis.
* The dylib keeps the linker's `adhoc,linker-signed` signature. Do not run
  `codesign` on it: the tool-signed form is rejected at `dlopen` time by some
  hardened third-party processes.
* `make install` writes the dylib under a temporary name and renames it into
  place. Overwriting a dylib that a running app has mapped invalidates the
  signature the kernel already validated for that inode.
* The hook relies on two private AppKit methods, `_defaultTitlebarTitleRect`
  and `_defaultCenteredTitlebarTitleRect`. If either is missing after a macOS
  update the constructor logs one line to syslog and does nothing.
