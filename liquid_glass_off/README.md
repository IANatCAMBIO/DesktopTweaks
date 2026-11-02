# liquid_glass_off

An [Ammonia](https://github.com/CoreBedtime/ammonia) tweak that turns off
Liquid Glass ("Solarium") inside chosen apps on macOS 26 Tahoe. Nothing on
the system volume is modified and no feature-flag file is written. Processes
that are not in the whitelist, including Notification Center, Control Center,
the Dock, system alerts and the USB "Allow accessory" prompt, keep their
normal rendering.

## How it works

AppKit and SwiftUI both decide whether to draw Liquid Glass by asking
libsystem_featureflags for one flag, `SwiftUI/Solarium`, once per process at
start-up. Ammonia injects `liquid_glass_off.dylib` before `main()` runs, and
the tweak replaces that single query (via the frida-gum interceptor that
Ammonia hands every tweak) so it answers "off" for `SwiftUI/Solarium` and
passes every other flag through untouched. The process then lays out and
draws the way it did on macOS 15.

Why not the old `FeatureFlags/Domain/SwiftUI.plist` override that this repo
used to ship? That override is read by every process on the machine, and on
macOS 26.1 and later the system's own chrome (Notification Center, Control
Center, the Dock, menus and alerts) has no non-glass fallback, so those
surfaces lose their backgrounds. Notifications became transparent, and the
USB "Allow accessory to connect?" alert, drawn by IOUIAgent, became
invisible, which is why external drives appeared not to mount. Scoping the
change per process avoids both problems.

## Prerequisites

* macOS 26 on Apple silicon (built for `arm64` and `arm64e`).
* Ammonia installed and working, with SIP disabled and the
  `-arm64e_preview_abi` boot argument set, as Ammonia requires.
* Xcode Command Line Tools (`clang`, `codesign`).

## Install

```sh
make            # builds liquid_glass_off.dylib and the solarium_probe test tool
make test       # proves the hook flips Liquid Glass off in a fresh process
make install    # invokes sudo itself
```

`make install` copies the dylib into
`/private/var/ammonia/core/tweaks`. It installs the default whitelist only if
no whitelist is already there, so edits made directly in the tweak directory
survive a reinstall; use `make install-whitelist` to deliberately replace the
installed whitelist with this repo's copy. Relaunch the whitelisted apps
(for Finder: `killall Finder`) and they come up without Liquid Glass.
No reboot is needed.

If Ammonia runs with an `ammonia.allowlist`, each app must also be listed
there, or Ammonia never injects anything into it.

## Choosing the processes

`liquid_glass_off.dylib.whitelist` lists one executable name per line, matched
on the whole name (the executable inside the `.app`, not the bundle id):

```
Finder
Calendar
Mail
Messages
Notes
Terminal
```

Edit it, run `make install-whitelist` and relaunch the app. Keep system UI
processes such as `NotificationCenter`, `ControlCenter`, `Dock`,
`IOUIAgent` and `loginwindow` out of the list; they have no non-glass fallback
and will render without backgrounds.

## Verify

```sh
make run
```

prints the Liquid Glass state of a fresh process with and without the tweak
loaded, using the same loading path Ammonia uses:

```
== without tweak
feature flag SwiftUI/Solarium    ON
AppKit NSSolariumEnabled         ON
SwiftUI Solarium.isEnabled       ON
== with tweak
feature flag SwiftUI/Solarium    OFF
AppKit NSSolariumEnabled         OFF
SwiftUI Solarium.isEnabled       OFF
```

If a whitelisted app still shows Liquid Glass after a relaunch, check that
its executable name is in both the tweak whitelist and Ammonia's allowlist,
then look for the tweak's own error lines:

```sh
log show --last 5m --predicate 'eventMessage CONTAINS "liquid_glass_off"'
```

## Uninstall

```sh
make uninstall
```

then relaunch the affected apps.

## Limitations

* An app that opts into Liquid Glass explicitly (`SupportsSolarium` in its
  Info.plist, used by a few Apple apps) is still governed by the flag, so the
  tweak works there too, but a view that is only implemented with glass
  (`NSGlassEffectView` without a fallback) will draw nothing in that app.
  None of the default whitelist entries do this in their main windows.
* Apps you build yourself do not need the tweak: set
  `UIDesignRequiresCompatibility` to `YES` in their Info.plist, Apple's
  supported per-app opt-out, which still works on macOS 26.6.

## Credits

This tweak began as [GoAwayLiquidGlass](https://github.com/YotsubaTamiko/GoAwayLiquidGlass)
by Tamiko Yotsuba, which shipped a global
`FeatureFlags/Domain/SwiftUI.plist` override. The per-process Ammonia hook
described above, the `solarium_probe` test tool and this README are a later
rewrite; the original repo publishes no licence.

It depends on [Ammonia](https://github.com/CoreBedtime/ammonia) by CoreBedtime
for injection.
