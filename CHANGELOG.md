# Changelog

## Unreleased

### Features

- `--long` options: `--fill image.jpg` and `--fill=image.jpg`, the old `-fill image.jpg` keeps working (#13)
- `-h`/`--help` and `--version`, these no longer need an X display

### Changes

- any error (bad image, bad color, bad number, missing argument, unknown option, failed `-write`) now exits with 1 and leaves the background untouched, previously hsetroot carried on and exited 0 (#6)
- numeric arguments are checked strictly (`-blur 3px` and `-alpha 300` are errors now)
- images are loaded once, not once per X screen
- `make check` runs a regression test suite in Xvfb
- imlib2 >= 1.12.6 is now required, older versions render gradients wrong on large screens (`make IMLIB2_MIN=0` builds anyway)

### Fixes

- `-alpha` with an image broke later `-tint`/`-contrast`/`-brightness`/`-gamma` (and printed an imlib2 warning)
- with multiple monitors, `-solid`, `-gradient` and manipulations after an image only affected the last monitor
- Makefile - libraries go to `LDLIBS`, so the build works with `-Wl,--as-needed` and the `--no-as-needed` workaround is gone (packagers can drop theirs too)

## 1.0.5 - 2019-11-18

See https://github.com/himdel/hsetroot/releases for 1.0.5 and earlier.
