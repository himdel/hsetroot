# hsetroot - imlib2-based wallpaper changer

## description
yet another wallpaper application

## dependencies
* libimlib2-dev
* libx11-dev
* pkg-config
* make
* sed

## changelog
### HEAD:
* **xrandr based output support for all modes**
  * requires **xrandr** and **sed** binaries in $PATH
* merged Debian patches
  * fix a segmentation fault when no display is present
  * **add '-extend' option and '-cover' options**
  * re-introduce the screen mask `-screens` option for X screen support
* replaced autotools in favour of a simple Makefile
  * x11 paths added
  * requires **pkg-config**
### 1.0.2:
  * initial commit of Hyriand's codebase:

## usage
```
Syntax: ./hsetroot command1 [arg1..] [command2 [arg1..]]...
Generic Options:
 -help                      This help information
 -screens <int>             Set a screenmask to use
 -outputs                   List screen outputs detected via xrandr
 -output <name>             Apply to xrandr output 'name' only*

*supports: images [all], solid, manipulation [alpha, blur, sharpen]

Gradients:
 -add <color>               Add color to range using distance 1
 -addd <color> <distance>   Add color to range using custom distance
 -gradient <angle>          Render gradient using specified angle
 -clear                     Clear the color range

Solid:
 -solid <color>             Render a solid using the specified color

Image files:
 -center <image>            Render an image centered on screen
 -cover <image>             Render an image centered on screen scaled to fill the screen fully
 -tile <image>              Render an image tiled
 -full <image>              Render an image maximum aspect
 -extend <image>            Render an image max aspect and fill borders
 -fill <image>              Render an image stretched

Manipulations:
 -alpha <amount>            Adjust alpha level for colors and images
 -blur <radius>             Blur the current image
 -sharpen <radius>          Sharpen the current image
 -tint <color>              Tint the current image
 -contrast <amount>         Adjust contrast of current image
 -brightness <amount>       Adjust brightness of current image
 -gamma <amount>            Adjust gamma level of current image
 -flipv                     Flip the current image vertically
 -fliph                     Flip the current image horizontally
 -flipd                     Flip the current image diagonally

Misc:
 -write <filename>          Write current image to file

Colors are in the #rrggbb or #rrggbbaa format.
```

## licence

licenced under GPL2: https://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html

## contributors

* Hyriand `<hyriand@thegraveyard.org>` [original author]
* Martin Hradil `<mhradil@redhat.com>` [maintainer] 
* Michal Suchanek `<hramrach@gmail.com>`
* Alessandro Ghedini `<al3xbio@gmail.com>`
* Francesco Pedrini `<francesco.pedrini@gmail.com>`
* Pete Beardmore `<pete.beardmore@msn.com>`

## todo
* [ ] replace xrandr/sed usage with libxrandr [zero returns in XRRPanning blocker!]
* [ ] re-work command line option parsing, move away from ```getopt_long```
* [ ] support lists of images to be used in sequence across outputs (iterated over in TL->TR->BL->BR direction)

## examples
### listing current xrandr outputs
```
./hsetroot -outputs
output idx: 0, name: DVI-1, size: 1280x720, pos: +3840 +0
output idx: 1, name: DisplayPort-0, size: 1920x1080, pos: +0 +0
output idx: 2, name: DisplayPort-1, size: 1920x1080, pos: +1920 +0
```
### mixing global and output specific options
below, the 'root' (e.g. all outputs) is coloured `-solid` before individual outputs' (addressed by name `-output`) options are overlaid. finally a global only option `-contrast` is applied

note how order matters here - if the final global option was applied before the individual outputs' option sets, it would have only applied to the root's solid colour! also note the deliberate multiple setting of options for the `DisplayPort-1` output, the initial setting of a solid colour giving that output's background a red tint, visible through its ultimate image due to the use of `-alpha`  
```
picpath=/usr/local/share/images; \
pic1=space.opo0511a.mod.jpg; \
pic2=space.potw1411a.mod.jpg; \
./hsetroot -solid "#222222" -alpha 128 \
  -output DisplayPort-0 -solid "#440000" \
  -output DisplayPort-0 -fill $picpath/$pic1 -alpha 192 \
  -output DisplayPort-1 -fill $picpath/$pic2 -alpha 192 -blur 3 \
  -contrast 4
```

