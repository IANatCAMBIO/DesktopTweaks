# firefox_titlebar

An [Ammonia](https://github.com/CoreBedtime/ammonia) tweak that centers the
window title in Firefox browser windows on macOS 26 Tahoe. It loads into the
`firefox` process only and changes one thing: where AppKit places the title
text. Firefox's tab strip, toolbars, window style and content layout are left
exactly as Firefox set them.

## Why a separate tweak

Firefox already shows a real titlebar (with "Title Bar" enabled in its
toolbar customization), but macOS 26 leading-aligns the text in it. Running
Firefox through a generic classic-titlebar tweak fixes the alignment but also
clears the window's full-size-content flag, and Gecko keeps reserving its own
titlebar strip at the top of the content, so a blank band appears between the
titlebar and the tab strip. Firefox needs the centering and nothing else, so
it gets its own tweak instead of Gecko-specific branches in a tweak that is
loaded into other apps.

## How it works

AppKit decides between a centered and a leading-aligned title rect in
`-[NSThemeFrame _defaultTitlebarTitleRect]`. Under Liquid Glass it picks
leading unless the window has a Preference-style toolbar or a leading titlebar
accessory, neither of which Firefox has. The tweak replaces that method; for
windows of Gecko's `ToolbarWindow` class (Firefox's browser windows) it
returns AppKit's own `_defaultCenteredTitlebarTitleRect`, and for every other
window it returns the original answer. The field keeps tracking resizes and
title changes because AppKit recomputes the rect through the same method.

Titles wider than the titlebar still fill the available width and truncate,
exactly as they do in a centered native app.

## Prerequisites

* macOS 26 on Apple silicon (built for `arm64` and `arm64e`).
* Ammonia installed and working.
* Xcode Command Line Tools (`clang`).

## Install

```sh
make
sudo make install
make run          # relaunches Firefox
```

`make install` copies the dylib and its whitelist into
`/private/var/ammonia/core/tweaks`. If Ammonia runs with an
`ammonia.allowlist`, `firefox` must be listed there as well.

## Choosing the processes

`firefox_titlebar.dylib.whitelist` contains one line, `firefox`, which is the
lowercase name of Firefox's main executable. Ammonia matches the whole
executable name, so helper processes such as `plugin-container` are never
touched. Firefox Developer Edition and Nightly use the same executable name
and are covered by the same line.

## Verify

After `make run`, a browser window with a short title (a new tab shows
"Mozilla Firefox") has that title centered between the traffic lights and the
right edge, with the tab strip directly below the titlebar and no blank band.
If it is not, confirm the tweak is in the process and look for its own error
line:

```sh
lsof -p "$(pgrep -x firefox)" | grep firefox_titlebar
log show --last 5m --predicate 'eventMessage CONTAINS "firefox_titlebar"'
```

## Uninstall

```sh
sudo make uninstall
make run
```
