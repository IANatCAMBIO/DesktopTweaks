# DesktopTweaks

[Ammonia](https://github.com/CoreBedtime/ammonia) tweaks that restore classic
macOS window chrome on macOS 26 Tahoe. Each tweak is a small dylib that
Ammonia injects into a whitelisted set of processes before `main()` runs;
nothing on the system volume is modified, and any process not on a tweak's
whitelist is left completely untouched.

| Directory | Tweak dylib | What it does |
|---|---|---|
| [`classic_titlebar/`](classic_titlebar/) | `ClassicTitlebar.dylib` | Forces classic AppKit titlebars — opaque titlebar, visible traffic lights, centered title — for native AppKit, Electron and Catalyst apps. |
| [`firefox_titlebar/`](firefox_titlebar/) | `firefox_titlebar.dylib` | Centers the window title in Firefox browser windows, and changes nothing else about Firefox's chrome. |
| [`liquid_glass_off/`](liquid_glass_off/) | `liquid_glass_off.dylib` | Turns Liquid Glass ("Solarium") off per process, so whitelisted apps lay out and draw the way they did on macOS 15. |

Every directory is self-contained, with its own README, build notes, Makefile,
source and default whitelist. Read those for the details of how each tweak
works; this file only covers the set as a whole.

Firefox gets its own tweak rather than a branch inside `classic_titlebar`
because it needs the title centering and nothing else — running it through the
generic tweak clears the window's full-size-content flag, and Gecko keeps
reserving its own titlebar strip, which leaves a blank band above the tab
strip.

## Prerequisites

* macOS 26 (Tahoe) on Apple silicon — the dylibs are built for `arm64` and
  `arm64e`.
* Ammonia installed and working, which in turn requires SIP disabled and the
  `-arm64e_preview_abi` boot argument set.
* Xcode Command Line Tools (`clang`, `codesign`).

## Build and install

```sh
make         # build all three tweaks
make install # install all three into /private/var/ammonia/core/tweaks
```

Run both as yourself. Ammonia's tweak directory is root-owned, so each tweak's
`install` target invokes `sudo` itself and you are prompted once, rather than
running the build as root and leaving root-owned build products behind. If you
are already root, use `make install SUDO=` to skip the inner `sudo`.

Then relaunch the affected apps — injection happens at process launch, so
anything already running keeps whatever it was launched with. No reboot is
needed.

Other top-level targets: `make test` (runs `liquid_glass_off`'s probe-based
test, the only subdirectory with one), `make clean`, and `make uninstall`
(removes `firefox_titlebar` and `liquid_glass_off`; `classic_titlebar` has no
uninstall target yet, and the target says what to remove by hand).

Each tweak can also be built and installed on its own:

```sh
cd liquid_glass_off && make && make test && make install
```

## Choosing which apps a tweak applies to

Ammonia gates injection in two independent stages, and a tweak runs in a
process only if it passes both:

1. **Ammonia's own allowlist.** If `/private/var/ammonia/core/ammonia.allowlist`
   exists, Ammonia injects only into the executables it names. If that file is
   absent, Ammonia runs in role-based mode and injects into all GUI-role apps.
2. **The tweak's whitelist.** Each tweak ships a
   `<tweak>.dylib.whitelist` sidecar, installed next to the dylib, listing one
   executable name per line. A tweak loads *only* into the processes listed
   there.

Matching is on the **executable** name — the binary inside the `.app`, not the
bundle identifier — and is exact and case-sensitive, matched against either the
full executable path or its last path component. So `Mail` matches
`/Applications/Mail.app/Contents/MacOS/Mail` but not `MailChimp` and not
`mail`. A bare name matches whether the app lives in `/Applications` or
`/System/Applications`. Use a full path only to disambiguate two apps that
ship an executable with the same name.

Keep system UI processes — `NotificationCenter`, `ControlCenter`, `Dock`,
`IOUIAgent`, `loginwindow` — off the `liquid_glass_off` whitelist. They have
no non-glass fallback and will render without backgrounds.

To change the targets, edit the **installed** whitelist in
`/private/var/ammonia/core/tweaks/` (or edit the copy here and reinstall) and
relaunch the app. Note that `classic_titlebar`'s `make install` deliberately
will not overwrite an existing installed whitelist.

Keep whitelist files to one bare executable name per line: Ammonia's per-tweak
whitelist parser does not strip leading or trailing spaces and does not treat
`#` as a comment, so `  Finder` and `# Apple apps` both become entries that
simply never match.

## Verifying what is actually injected

Read the real mapped images rather than inferring from the whitelist:

```sh
lsof /private/var/ammonia/core/tweaks/ClassicTitlebar.dylib
vmmap <pid> | grep ammonia/core/tweaks
```

The strong check is differential: pick a process that one whitelist includes
and another excludes, and confirm only the expected subset is mapped.

## Temporarily disabling a tweak

Ammonia skips any tweak that has neither a `.whitelist` nor a `.blacklist`
sidecar, so renaming the sidecar out of the way disables the tweak without
touching the dylib:

```sh
cd /private/var/ammonia/core/tweaks
sudo mv ClassicTitlebar.dylib.whitelist ClassicTitlebar.dylib.whitelist.off
```

Relaunch an app and it comes up clean; rename the sidecar back and relaunch to
restore. Sidecars are read at each process launch, so no daemon restart is
needed either way.

## Credits

* [Ammonia](https://github.com/CoreBedtime/ammonia) by CoreBedtime provides
  the injection framework and the frida-gum interceptor every tweak here uses.
* `liquid_glass_off` began as
  [GoAwayLiquidGlass](https://github.com/YotsubaTamiko/GoAwayLiquidGlass) by
  Tamiko Yotsuba; see that directory's README for what changed.
