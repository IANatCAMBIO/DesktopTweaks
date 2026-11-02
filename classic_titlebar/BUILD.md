# Building

## Toolchain

* macOS 26 with Xcode Command Line Tools (`xcode-select --install`); Xcode
  itself is not required. The SDK is located at build time with
  `xcrun --sdk $(SDK) --show-sdk-path`, so `SDK` can point at another SDK.
* No third-party libraries. The tweak is Objective-C built with ARC
  (`-fobjc-arc`) against Cocoa and QuartzCore, and does all of its work
  through the Objective-C runtime (`objc/runtime.h`, `objc/message.h`).
  QuartzCore is needed for the sharp-corner override, which manipulates the
  titlebar decoration's `CUIWindowFrameLayer` directly.
* Unlike the other two tweaks here, nothing is resolved with `dlsym` and
  frida-gum is not used. Ammonia hands every tweak a `GumInterceptor` through
  the exported `LoadFunction`, but this tweak's `LoadFunction` is a no-op that
  exists only so Ammonia can confirm a clean load; the real work is installed
  from a library constructor, synchronously, so the swizzles are in place
  before the app creates its first `NSWindow`.

## Targets

| target                  | what it does                                                                        |
|-------------------------|-------------------------------------------------------------------------------------|
| `make`                  | builds `ClassicTitlebar.dylib` (arm64 + arm64e + x86_64)                            |
| `make install`          | copies the dylib into `/private/var/ammonia/core/tweaks`, and the whitelist only if none is installed (needs sudo) |
| `make install-whitelist`| replaces the installed whitelist with this repo's copy (needs sudo)                 |
| `make uninstall`        | removes the dylib and whitelist again (needs sudo)                                  |
| `make clean`            | deletes the built dylib                                                             |

There is no `run` or `test` target: the tweak only takes effect when Ammonia
injects it at process launch, so it cannot be exercised in-process the way
`liquid_glass_off`'s probe can.

`SUDO` can be overridden, for example `make install SUDO="sudo -A"` to use an
askpass helper, or `make install SUDO=` when already root. `ARCHS`, `SDK`,
`OUT`, `SRC` and `TWEAK_DIR` are overridable the same way.

## Files

* `ClassicTitlebar.m` — the tweak. A library constructor swizzles a set of
  `NSWindow` setters (`setStyleMask:`, `setToolbar:`, `setToolbarStyle:`,
  `setSubtitle:`, the titlebar-accessory inserters and `orderFront:`) plus
  `-[NSThemeFrame _defaultTitlebarTitleRect]`, and classifies each window at
  runtime to pick the base or the Electron treatment. `ElectronNSWindow` and
  `NSTitlebarContainerView` swizzles are installed lazily, on first sight of
  such a window, because the class only exists once the app has loaded it.
* `ClassicTitlebar.dylib.whitelist` — process names the tweak loads into.
  Ammonia reads `<dylib>.whitelist` next to the installed dylib.

## Platform notes

* The dylib keeps the linker's `adhoc,linker-signed` signature. **Do not run
  `codesign` on it.** Re-signing produces a tool-signed form with extra
  special slots that the macOS page validator rejects at `dlopen` time in some
  processes — Slack `SIGKILL`s with `Invalid Page` in `dmesg`.
* `make install` writes the dylib under a sidecar name and `rename(2)`s it into
  place. The kernel caches a dylib's code signature per inode; overwriting in
  place with `cp` keeps the inode and changes the mtime, which leaves the
  cached `cs_mtime` out of sync and makes every later `dlopen` fail with
  `Invalid Page`. Renaming gives the install path a fresh inode, so the
  signature is read from scratch.
* An `x86_64` slice is built as well as `arm64` and `arm64e`, so the dylib can
  also be injected into Rosetta-translated processes. The other two tweaks
  here are `arm64`/`arm64e` only.
* The tweak depends on private AppKit internals, and these are the first things
  to check after a macOS update: `_defaultTitlebarTitleRect` and
  `_defaultCenteredTitlebarTitleRect` on `NSThemeFrame`, the
  `NSTitlebarContainerView` and `_NSTitlebarDecorationView` view classes, and
  `ElectronNSWindow`'s `originalContentRectForFrameRect:`. Window
  classification happens at runtime, so adding an app to the whitelist needs
  no code change, but a renamed selector will silently stop a treatment.
* The sharp-corner override is driven by the global `NSConvolutionOverride1`
  default (`defaults write -g NSConvolutionOverride1 -float 5`) and only runs
  while that is set.
