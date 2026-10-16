hsetroot - imlib2-based wallpaper changer

original author: `Hyriand <hyriand@thegraveyard.org>` (2003)  
licence: GPL2  
needs libimlib2-dev (>= 1.12.6), libx11-dev, libxinerama-dev, libxrandr-dev, pkg-config, make  
`make check` runs the tests in Xvfb, needs xvfb, x11-xserver-utils (xrandr), x11-utils (xprop), imagemagick

This repo contains hsetroot 1.0.2 with the following changes:

   * autotools was replaced by a simple Makefile which requires pkg-config, x11 paths added
   * includes patches from debian, written by Martin Suchanek, more details in git log
     * Fix a segmentation fault when no display is present.
     * Add the '-extend' option
     * Add the '-cover' option
   * X screens (the `:0.1` part of `$DISPLAY`, not monitors - for those see `--output`): hsetroot draws on the screen from `$DISPLAY`, `--screen N` picks another one, `-screens <mask>` several (1 = screen 0, 2 = screen 1, 3 = both; 0 is an error)
   * **added output support for all modes** - images are drawn on each monitor, as listed by `xrandr --listmonitors` (so `xrandr --setmonitor` works too), falls back to Xinerama and then to the whole screen on servers without RandR 1.5
     * use `-root` to treat multiple displays as one big display (original hsetroot behavior)
     * when no monitor is active (all turned off), drawing images is an error, `-root` still works
   * `--output NAME` scopes drawing and manipulations to that monitor, `--output all` goes back to all of them - e.g. `hsetroot --output eDP-1 --cover laptop.jpg --output HDMI-1 --cover tv.jpg`
     * settings like `-alpha` and colors added with `-add` carry over to the next `--output`; use `-clear` before adding colors for a new gradient
   * `hsr-outputs` lists the monitors hsetroot sees, with their names (`--csv`, `--json`)
   * added support for color names and more color formats (using XParseColor except for #rrggbbaa)
   * modern `--long` options (`--fill image.jpg`, `--fill=image.jpg`), the old `-fill image.jpg` form still works
   * `-h`/`--help` and `--version`
   * any error (bad image, bad color, unknown option, or an unknown output on another X screen) exits with 1 before touching the background; failed `-write` reports the imlib2 error
   * numeric arguments reject out-of-range values, negative blur/sharpen radii and screen masks, and non-finite angles/amounts

NOTE: imlib2 < 1.12.6 renders gradients wrong when the screen is larger than 4096 pixels in either dimension (and on some 4K setups), so the Makefile refuses older versions. Use `make IMLIB2_MIN=0` to build anyway.
