# Changelog

## Unreleased

### Changes

- `make check` runs a regression test suite in Xvfb
- imlib2 >= 1.12.6 is now required, older versions render gradients wrong on large screens (`make IMLIB2_MIN=0` builds anyway)

### Fixes

- Makefile - libraries go to `LDLIBS`, so the build works with `-Wl,--as-needed` and the `--no-as-needed` workaround is gone (packagers can drop theirs too)

## 1.0.5 - 2019-11-18

See https://github.com/himdel/hsetroot/releases for 1.0.5 and earlier.
