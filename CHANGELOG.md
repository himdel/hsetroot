# Changelog

## 1.1.0

### Features

- `-gradient` takes fractional angles (#45, thanks @lindhobe)
- `make install` supports `DESTDIR` (#38, thanks @sham1)
- warnings for settings that nothing after them uses, e.g. `-fg` after the bitmap, `-add` without `-gradient`, `-alpha` at the end
- man page, `make install` puts it in `$(PREFIX)/share/man/man1`
- xsetroot-style bitmap backgrounds: `-bitmap file.xbm`, `-mod x y`, `-gray`/`-grey`, colored with `-fg`, `-bg`, `-rv`/`-reverse` - pixel-identical to xsetroot, except the colors are positional and go before the bitmap: `xsetroot -mod 16 16 -fg red` is `hsetroot -fg red -mod 16 16` (#25)
- `--output NAME` - everything after it applies to just that monitor (images, `-solid`, `-gradient`, manipulations like `-fliph` or `-tint`), `--output all` switches back; e.g. `hsetroot --output eDP-1 --cover laptop.jpg --output HDMI-1 --cover tv.jpg` (#14)
- `hsr-outputs --help`, unknown options are an error now
- `--long` options: `--fill image.jpg` and `--fill=image.jpg`, the old `-fill image.jpg` keeps working (#13)
- `-h`/`--help` and `--version`, these no longer need an X display

### Changes

- the `output NAME: size(..) pos(..)` lines are only printed with `--verbose`, and list each output drawn on once - by images, gradients, solids, bitmaps or manipulations - instead of once per image
- `-screens 0` now errors instead of silently doing nothing; skip the invocation to keep the old no-op behavior
- **upgrade note:** `-gradient` is drawn on each monitor separately, like images; add `-root` for the old single gradient across all monitors (#44)
- **upgrade note:** with multiple X screens (`:0.0`, `:0.1`, zaphod-style setups - not multiple monitors), only the screen from `$DISPLAY` is drawn now, like other X tools do; before, hsetroot drew on all of them. Use `-screens 3` (bitmask) for the old behaviour, or run it once per screen (#24)
- `hsr-outputs` prints the monitor names instead of Xinerama screen numbers: csv `name,x,y,width,height`, json/text keys `name`, `x`, `y`, `width`, `height`
- monitors come from XRandR (`xrandr --listmonitors`, including monitors defined with `xrandr --setmonitor`), with Xinerama and then the whole screen as fallbacks - new build dependency on libxrandr (Debian `libxrandr-dev`, Arch `libxrandr`)
- when no monitor is active (all turned off or disabled), drawing an image or gradient is an error instead of a silently black background - use `-root` to draw on the whole screen (#41)
- any error (bad image, bad color, bad number, missing argument, unknown option, failed `-write`) now exits with 1 and leaves the background untouched, previously hsetroot carried on and exited 0 (#6)
- numeric arguments are checked strictly (`-blur 3px`, negative or out-of-range radii and screen masks, non-finite values, and `-alpha 300` are errors now)
- images are loaded once, not once per X screen
- `make check` runs a regression test suite in Xvfb
- imlib2 >= 1.12.6 is now required, older versions render gradients wrong on large screens (`make IMLIB2_MIN=0` builds anyway)

### Fixes

- `-center`, `-cover` and `-tile` no longer draw over neighbouring monitors (#18, #30, thanks @djpohly)
- the previous background pixmap is freed again - the atom name used to find it was wrong (#35, #36, thanks @vincentbernat)
- `hsr-outputs` no longer crashes without a display (#32, thanks @jer-gentoo)
- builds on macOS (#31, thanks @sfroment)
- `make install` fixed (#39, thanks @hramrach)
- an unknown `--output` on a later X screen no longer changes the wallpaper on earlier screens before failing
- `-write` reports imlib2 errors such as unsupported file formats instead of `Unknown error -3`
- malformed `#rrggbbaa` colors with only one valid alpha digit are rejected
- `-cover`, `-full` and `-extend` could leave black gaps at an edge for some image sizes
- drawing on more than one X screen crashed with an X error (`BadMatch` in `X_ShmPutImage`, or `BadValue` in `X_KillClient`)
- gradients on screens over 4096px came out as a solid color even with imlib2 1.12.6, hsetroot now scales the color distances to avoid the imlib2 bug; gradients also reach their end colors exactly now (#28, #42, #43)
- `-alpha` with an image broke later `-tint`/`-contrast`/`-brightness`/`-gamma` (and printed an imlib2 warning)
- with multiple monitors, `-solid`, `-gradient` and manipulations after an image only affected the last monitor
- Makefile - libraries go to `LDLIBS`, so the build works with `-Wl,--as-needed` and the `--no-as-needed` workaround is gone (packagers can drop theirs too)

## 1.0.5 - 2019-11-18

See https://github.com/himdel/hsetroot/releases for 1.0.5 and earlier.
