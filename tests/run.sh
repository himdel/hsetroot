#!/bin/sh
# regression tests, run in Xvfb - needs xvfb, xrandr, xprop, imagemagick
cd "$(dirname "$0")/.." || exit 1
T=$(mktemp -d)
XPID=
trap 'stop; rm -rf "$T"' EXIT
pass=0
fail=0

stop() {
  [ -n "$XPID" ] && kill $XPID && wait $XPID 2>/dev/null
  XPID=
}

# screen WxH [WxH+X+Y..] - fresh Xvfb, optionally split into monitors, extra Xvfb args in $XARGS
screen() {
  stop
  Xvfb -displayfd 3 -noreset -screen 0 "$1"x24 $XARGS 3>"$T/fd" >/dev/null 2>&1 &
  XPID=$!
  while [ ! -s "$T/fd" ]; do sleep 0.1; done
  DISPLAY=:$(cat "$T/fd")
  export DISPLAY
  : >"$T/fd"
  shift
  out=screen
  n=0
  for m in "$@"; do
    xrandr --setmonitor "m$n" "$(echo "$m" | sed 's/x\([0-9]*\)/\/1x\1\/1/')" $out >/dev/null
    out=none
    n=$((n + 1))
  done
}

# run hsetroot args.. - keeps exit code in $rc, root window in $T/root.png
run() {
  ./hsetroot "$@" >"$T/out" 2>&1
  rc=$?
  import -window root "$T/root.png"
}

t() {
  name=$1
}

ok() {
  pass=$((pass + 1))
}

ko() {
  fail=$((fail + 1))
  echo "FAIL: $name: $*"
}

# is X Y R,G,B [tolerance] - root pixel color
is() {
  got=$(convert "$T/root.png" -format "%[fx:int(255*p{$1,$2}.r+.5)],%[fx:int(255*p{$1,$2}.g+.5)],%[fx:int(255*p{$1,$2}.b+.5)]" info:)
  if echo "$got $3 ${4:-2}" | awk '{ split($1, a, ","); split($2, b, ","); for (i = 1; i <= 3; i++) if (a[i] - b[i] > $3 || b[i] - a[i] > $3) exit 1 }'; then
    ok
  else
    ko "pixel $1,$2 is $got, expected $3"
  fi
}

rc_is() {
  [ "$rc" = "$1" ] && ok || ko "exit code $rc, expected $1"
}

# images: 200x100 red|blue halves, 1500x500 red|blue halves
convert -size 100x100 xc:red -size 100x100 xc:blue +append "$T/rb.png"
convert -size 750x500 xc:red -size 750x500 xc:blue +append "$T/rb-big.png"


t "no args gives black root"
screen 400x300
run
rc_is 0
is 200 150 0,0,0

t "solid color formats"
for c in red '#ff0000' 'rgb:ff/00/00' '#ff0000ff'; do
  run -solid "$c"
  is 10 10 255,0,0
done
# XParseColor treats #rgb as the high bits
run -solid '#f00'
is 10 10 240,0,0

t "#rrggbbaa alpha over black"
run -solid '#ff000080'
is 10 10 128,0,0

t "-alpha applies to later colors"
run -solid blue -alpha 128 -solid red
is 10 10 128,0,127

t "background atoms are set (#35)"
run -solid red
props=$(xprop -root _XROOTPMAP_ID ESETROOT_PMAP_ID | sed 's/.*# //' | sort -u)
[ "$(echo "$props" | wc -l)" = 1 ] && echo "$props" | grep -q '^0x' && ok || ko "atoms: $props"

t "-fill per monitor"
screen 1000x500 500x500+0+0 500x500+500+0
run -fill "$T/rb.png"
is 10 10 255,0,0
is 490 10 0,0,255
is 510 10 255,0,0
is 990 10 0,0,255

t "-root -fill spans monitors (#19, #26)"
run -root -fill "$T/rb.png"
is 10 10 255,0,0
is 490 10 255,0,0
is 510 10 0,0,255

t "-full keeps aspect, centered"
screen 1000x1000
run -full "$T/rb.png"
is 10 10 0,0,0
is 10 500 255,0,0
is 990 500 0,0,255
is 500 990 0,0,0

t "-cover keeps aspect, covers"
run -cover "$T/rb.png"
is 10 10 255,0,0
is 990 990 0,0,255

t "-center doesn't bleed into other monitors (#18, #30)"
screen 2000x500 1000x500+0+0 1000x500+1000+0
run -center "$T/rb-big.png"
is 900 10 0,0,255
is 1100 10 255,0,0

t "-tile, centered"
screen 400x100
run -tile "$T/rb.png"
is 10 10 0,0,255
is 110 10 255,0,0
is 210 10 0,0,255

t "manipulations are positional (#10)"
run -fill "$T/rb.png" -fliph
is 10 10 0,0,255
run -fliph -fill "$T/rb.png"
is 10 10 255,0,0

t "-tint"
run -solid white -tint red
is 10 10 255,0,0

t "gradient"
screen 1000x500
run -add red -add blue -gradient 90
is 5 250 0,0,255 10
is 995 250 255,0,0 10

t "gradients over 4096px (#28, #42, #43)"
for g in 5120x500 8193x200 500x5000; do
  screen $g
  w=${g%x*}
  h=${g#*x}
  run -add red -add blue -gradient 90
  is 0 $((h / 2)) 0,0,255 6
  is $((w / 2)) $((h / 2)) 128,0,128 6
  is $((w - 1)) $((h / 2)) 255,0,0 6
  run -add red -add blue -gradient 0
  is $((w / 2)) 0 255,0,0 6
  is $((w / 2)) $((h - 1)) 0,0,255 6
done
run -addd red 20000 -addd blue 20000 -gradient 0
is 2 2 255,0,0 4

t "gradient per output, -root for one over all (#44)"
screen 1000x500 500x500+0+0 500x500+500+0
run -add red -add blue -gradient 90
is 0 250 0,0,255 6
is 499 250 255,0,0 6
is 500 250 0,0,255 6
is 999 250 255,0,0 6
run -root -add red -add blue -gradient 90
is 0 250 0,0,255 6
is 500 250 128,0,128 6
is 999 250 255,0,0 6

t "fractional gradient angle (#45)"
screen 1000x500
run -add red -add blue -gradient 45.5
rc_is 0

t "hsr-outputs lists RandR monitors by name"
screen 1000x500 500x500+0+0 500x500+500+0
[ "$(./hsr-outputs --csv | tr '\n' ' ')" = "name,x,y,width,height m0,0,0,500,500 m1,500,0,500,500 " ] && ok || ko "$(./hsr-outputs --csv)"

t "--output draws on one output (#14)"
screen 1000x500 500x500+0+0 500x500+500+0
run --output m1 -fill "$T/rb.png"
is 10 10 0,0,0
is 510 10 255,0,0
is 990 10 0,0,255
run --output m0 -solid red --output m1 -solid blue
is 10 10 255,0,0
is 990 10 0,0,255
run --output=m1 -solid red --output all -center "$T/rb.png"
is 10 10 0,0,0
is 510 10 255,0,0
is 230 250 255,0,0
is 730 250 255,0,0

t "manipulations stay on the --output"
run -fill "$T/rb.png" --output m1 -fliph
is 10 10 255,0,0
is 510 10 0,0,255
run -solid white --output m1 -tint red
is 10 10 255,255,255
is 510 10 255,0,0
run -solid white --output m0 -brightness -0.5 --output m1 -solid blue
is 10 10 128,128,128 3
is 510 10 0,0,255
run --output m1 -add red -add blue -gradient 90
is 10 10 0,0,0
is 501 250 0,0,255 6
is 998 250 255,0,0 6

t "-clear starts a new gradient on the next output"
run --output m0 -add red -add blue -gradient 90 -clear --output m1 -add green -add yellow -gradient 90
is 1 250 0,0,255 6
is 498 250 255,0,0 6
is 501 250 255,255,0 6
is 998 250 0,255,0 6

t "--output errors"
run -solid red
run -solid blue --output nosuch -solid green
rc_is 1
is 10 10 255,0,0
run -root --output m0 -solid green
rc_is 1
run -root --output all -solid green
rc_is 0

t "drawing on two X screens"
XARGS="-screen 1 200x100x24" screen 400x100
XARGS=
D=$DISPLAY
DISPLAY=$D ./hsetroot -solid red
DISPLAY=$D ./hsetroot -screens 3 -solid white
rc=$?
rc_is 0
for n in 0 1; do
  import -display "$D.$n" -window root "$T/root.png"
  is 10 10 255,255,255
done
t "hsr-outputs --help"
DISPLAY= ./hsr-outputs --help | grep -q Syntax && ok || ko "no help"
./hsr-outputs --bogus 2>/dev/null
[ $? = 1 ] && ok || ko "bad option doesn't exit 1"
[ "$(./hsr-outputs --text)" = "$(./hsr-outputs)" ] && ok || ko "--text differs"

t "X screens - default from \$DISPLAY, -screens (#24)"
XARGS="-screen 1 200x100x24" screen 400x100
XARGS=
D=$DISPLAY
shot() {
  import -display "$D.$1" -window root "$T/root.png"
}
DISPLAY=$D.0 ./hsetroot -solid red
DISPLAY=$D.1 ./hsetroot -solid blue
shot 0
is 10 10 255,0,0
shot 1
is 10 10 0,0,255
DISPLAY=$D.0 ./hsetroot -screens 2 -solid green
shot 0
is 10 10 255,0,0
shot 1
is 10 10 0,255,0
DISPLAY=$D ./hsetroot -screens 3 -solid white
rc=$?
rc_is 0
shot 0
is 10 10 255,255,255
shot 1
is 10 10 255,255,255
for args in "--screen 1" "-screens 4"; do
  run $args -solid red
  rc_is 1
done

t "zero screen mask is an error"
run -screens 0 -solid red
rc_is 1
grep -q 'no screens selected' "$T/out" && ok || ko "zero screen mask: $(cat "$T/out")"
shot 0
is 10 10 255,255,255

t "unknown output on another X screen leaves both roots untouched"
DISPLAY=$D.0 xrandr --setmonitor m0 400/1x100/1+0+0 screen >/dev/null
DISPLAY=$D.1 xrandr --setmonitor m1 200/1x100/1+0+0 screen >/dev/null
DISPLAY=$D.0 ./hsr-outputs --csv | grep -q '^m0,0,0,400,100$' && ok || ko "m0 not found on screen 0"
DISPLAY=$D.1 ./hsr-outputs --csv | grep -q '^m1,0,0,200,100$' && ok || ko "m1 not found on screen 1"
DISPLAY=$D ./hsetroot -screens 3 -solid red
DISPLAY=$D ./hsetroot -screens 3 --output m0 -solid blue >"$T/out" 2>&1
rc=$?
rc_is 1
shot 0
is 10 10 255,0,0
shot 1
is 10 10 255,0,0
export DISPLAY=$D

t "Xinerama fallback without RandR"
XARGS="-extension RANDR +xinerama -screen 1 200x100x24" screen 400x100
run -fill "$T/rb.png"
# both heads are at 0,0 in Xvfb, the smaller one wins on its first 200px
is 50 10 255,0,0
is 150 10 0,0,255
is 250 10 0,0,255
XARGS=

t "whole screen without RandR and Xinerama"
XARGS="-extension RANDR" screen 400x100
run -fill "$T/rb.png"
is 150 10 255,0,0
is 250 10 0,0,255
XARGS=

t "no outputs (all turned off, #41) is an error for images, -root still works"
screen 400x100
xrandr --output screen --off 2>/dev/null
run -solid red
rc_is 0
run -solid blue -fill "$T/rb.png"
rc_is 1
grep -q -- -root "$T/out" && ok || ko "no -root hint: $(cat "$T/out")"
is 10 10 255,0,0
run -root -fill "$T/rb.png"
rc_is 0
is 10 10 255,0,0
is 250 10 0,0,255

t "-write"
run -solid red -write "$T/w.png"
[ -s "$T/w.png" ] && ok || ko "no file written"

t "--help and --version work without a display"
DISPLAY= ./hsetroot --help >/dev/null 2>&1 && DISPLAY= ./hsetroot -h >/dev/null 2>&1 && DISPLAY= ./hsetroot --version >/dev/null 2>&1 && ok || ko "failed"

t "long options (#13)"
screen 400x100
for f in "--fill=$T/rb.png" "--fill $T/rb.png" "-fill $T/rb.png" "-fill=$T/rb.png"; do
  run -solid green $f
  is 10 10 255,0,0
  is 300 10 0,0,255
done

t "errors exit 1, root untouched (#6)"
for args in "-fill /nonexistent" "-solid nosuchcolor" "-solid" "-addd red" "-blur 3px" "-blur ''" "-blur -1" "-blur 2147483648" "-addd red 2147483648" "-screens -1" "-screens 18446744073709551616" "-alpha 300" "-gradient x" "-gradient nan" "-contrast inf" "-bogus" "foo" "-flipv=1" "-solid #ff0000zz" "-solid #ff0000fZ"; do
  run -solid red
  eval run -solid blue $args
  rc_is 1
  is 10 10 255,0,0
done

t "-write failure exits 1"
run -solid red -write /nonexistent/x.png
rc_is 1

t "-write reports unsupported formats"
run -solid red -write "$T/w.unknownformat"
rc_is 1
grep -q 'No saver' "$T/out" && ok || ko "write error: $(cat "$T/out")"

t "manipulations after an image apply to the whole root (cliprect reset)"
screen 1000x500 500x500+0+0 500x500+500+0
run -center "$T/rb.png" -solid red
is 10 10 255,0,0
is 990 10 255,0,0

t "-alpha with images keeps later color modifiers working"
run -solid white -alpha 128 -contrast 2 -fill "$T/rb.png" -brightness -0.5 -solid green
is 10 10 64,128,0
grep -q Warning "$T/out" && ko "imlib warning: $(cat "$T/out")" || ok


echo "$pass passed, $fail failed"
[ $fail = 0 ]
