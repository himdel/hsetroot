hsetroot - imlib2-based wallpaper changer

Composes a wallpaper for X out of solid colors, gradients, images and X bitmaps, optionally manipulates it, and sets it as the root window background - including the `_XROOTPMAP_ID` / `ESETROOT_PMAP_ID` properties compositors (picom) and pseudo-transparent terminals use.

original author: `Hyriand <hyriand@thegraveyard.org>` (2003)  
licence: GPL2

## Usage

```sh
hsetroot --cover wallpaper.jpg                              # scaled to cover each monitor
hsetroot --root --fill panorama.jpg                         # one image stretched over all monitors
hsetroot --output eDP-1 --cover a.jpg --output HDMI-1 --cover b.jpg  # different image per monitor
hsetroot --solid '#1e1e2e'                                  # solid color
hsetroot --add '#1e1e2e' --add '#89b4fa' --gradient 30      # gradient
hsetroot --fg gray20 --bg black --mod 16 16                 # xsetroot's grid
hsetroot                                                    # just black
```

Options are applied in order, each on top of what's drawn so far - manipulations like `--fliph` or `--blur` change what's already there, settings like `--alpha` or `--fg` affect what comes after. Every option works as `--name value`, `--name=value` or the old `-name value`.

See `hsetroot --help` or `man hsetroot` for all of them:

 * **images** - `--center`, `--cover`, `--tile`, `--full`, `--extend`, `--fill`
 * **colors** - `--solid`, gradients via `--add`/`--addd` + `--gradient`, `--alpha`
 * **bitmaps** like xsetroot - `--bitmap file.xbm`, `--mod x y`, `--gray` with `--fg`, `--bg`, `--rv` (same pixels as xsetroot, but the colors go before the bitmap)
 * **manipulations** - `--tint`, `--blur`, `--sharpen`, `--contrast`, `--brightness`, `--gamma`, `--flipv`, `--fliph`, `--flipd`
 * `--write file.png` saves the result too

Colors are `#rgb`, `#rrggbb`, `#rrggbbaa`, `rgb:r/g/b` or an X color name.

`--mod 3 5` draws foreground vertical lines 3 pixels apart and horizontal lines 5 pixels apart; the gaps use the background color. The pattern repeats every 16 pixels: 0 fills that direction, and values above 16 give one line per tile.

Any error (bad image, bad color, unknown option or output, ...) exits with 1 and leaves the background alone, including on multiple X screens. Numeric arguments reject out-of-range values and non-finite angles or amounts. Failed `--write` reports the imlib2 error.

## Multiple monitors

Images and gradients go on each monitor, as listed by `xrandr --listmonitors` (so monitors made with `xrandr --setmonitor` count too). On servers without RandR 1.5 it falls back to Xinerama, then to the whole screen.

 * `--root` treats all monitors as one big screen (original hsetroot behavior)
 * `--output NAME` scopes drawing and manipulations to that monitor, `--output all` switches back; settings like `--alpha` and colors added with `--add` carry over, so use `--clear` before adding colors for a new gradient
 * `hsr-outputs` lists the monitors hsetroot sees, with their names (`--csv`, `--json`)
 * when no monitor is active (all turned off), drawing images or gradients is an error - `--root` still works

X screens (the `.1` in `DISPLAY=:0.1` - rare, not the same as monitors): hsetroot draws on the screen from `$DISPLAY`. Set `DISPLAY=:0.1` for screen 1, or use `--screens MASK` for several (1 = screen 0, 2 = screen 1, 3 = both; 0 is an error).

## Building

needs libimlib2-dev (>= 1.12.6), libx11-dev, libxinerama-dev, libxrandr-dev, pkg-config, make
additionaly, running tests needs xvfb, x11-xserver-utils, x11-utils, imagemagick

```sh
make
make install                 # PREFIX=/usr/local, DESTDIR= supported
make check                   # tests
```

imlib2 < 1.12.6 renders gradients wrong when the screen is larger than 4096 pixels in either dimension (and on some 4K setups), so the Makefile refuses older versions. Use `make IMLIB2_MIN=0` to build anyway.

## History

This is hsetroot 1.0.2 by Hyriand, with:

 * autotools replaced by a simple Makefile using pkg-config
 * patches from Debian by Martin Suchanek - a fix for a crash without a display, `-extend` and `-cover`
 * everything else from [CHANGELOG.md](CHANGELOG.md) and the [releases](https://github.com/himdel/hsetroot/releases) - multi-monitor support, color names, `-root`, long options, xsetroot bitmaps, ...
