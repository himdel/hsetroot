hsetroot - imlib2-based wallpaper changer

original author: `Hyriand <hyriand@thegraveyard.org>` (2003)  
licence: GPL2  
needs libimlib2-dev (>= 1.12.6), libx11-dev, libxinerama-dev, pkg-config, make  
`make check` runs the tests in Xvfb, needs xvfb, x11-xserver-utils (xrandr), x11-utils (xprop), imagemagick

This repo contains hsetroot 1.0.2 with the following changes:

   * autotools was replaced by a simple Makefile which requires pkg-config, x11 paths added
   * includes patches from debian, written by Martin Suchanek, more details in git log
     * Fix a segmentation fault when no display is present.
     * Add the '-extend' option
     * Add the '-cover' option
   * reintroduced the screen mask `-screens` option from earlier versions .. which is rather useless actually, as it deals with X screens, not xrandr outputs
   * **added output support for all modes**, using xinerama
     * use `-root` to treat multiple displays as one big display (original hsetroot behavior)
   * added support for color names and more color formats (using XParseColor except for #rrggbbaa)

NOTE: imlib2 < 1.12.6 renders gradients wrong when the screen is larger than 4096 pixels in either dimension (and on some 4K setups), so the Makefile refuses older versions. Use `make IMLIB2_MIN=0` to build anyway.
