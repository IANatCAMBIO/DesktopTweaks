# Building

## Toolchain

* macOS 26 with Xcode Command Line Tools (tested with the clang shipped in
  Xcode 26 on macOS 26.6.2). `xcode-select --install` is enough; Xcode itself
  is not required.
* No third-party libraries. The tweak resolves frida-gum's interceptor and
  `_os_feature_enabled_impl` with `dlsym` at load time, so it links against
  nothing but libSystem.

## Targets

| target           | what it does                                                        |
|------------------|---------------------------------------------------------------------|
| `make`           | builds `liquid_glass_off.dylib` (arm64 + arm64e, ad-hoc signed) and `solarium_probe` |
| `make run`       | prints the Liquid Glass state of a fresh process with and without the tweak |
| `make test`      | fails unless the tweak flips every probe from ON to OFF             |
| `make install`   | copies the dylib and whitelist into `/private/var/ammonia/core/tweaks` (needs sudo) |
| `make uninstall` | removes them again (needs sudo)                                     |
| `make clean`     | deletes the built dylib and probe                                   |

`SUDO` can be overridden, for example `make install SUDO="sudo -A"` to use an
askpass helper.

## Files

* `liquid_glass_off.c` — the tweak. One exported function, `LoadFunction`,
  which Ammonia calls with its `GumInterceptor`; it replaces
  `_os_feature_enabled_impl` with a wrapper that answers "off" for
  `SwiftUI/Solarium`.
* `liquid_glass_off.dylib.whitelist` — process names the tweak loads into.
  Ammonia reads `<dylib>.whitelist` next to the dylib.
* `solarium_probe.c` — test tool. Loads `fridagum.dylib` and the tweak the
  way Ammonia's opener does, then queries libsystem_featureflags, AppKit's
  `NSSolariumEnabled` and SwiftUI's `Solarium.isEnabled`.

## Platform notes

* `make run` and `make test` need an installed Ammonia, because the probe
  loads `/private/var/ammonia/core/fridagum.dylib`. They do not need root.
* The dylib must be built for `arm64e` as well as `arm64`: Apple's own apps
  run as arm64e, and dyld refuses to load an arm64-only library into them.
* `make install` writes the dylib under a temporary name and renames it into
  place. Overwriting a dylib that a running app has mapped invalidates the
  signature the kernel already validated for that file, and the next launch
  of that app fails until reboot.
* Rosetta apps are not covered; build with
  `make ARCHS="-arch arm64 -arch arm64e -arch x86_64"` if you need them.
