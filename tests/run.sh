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

# screen WxH [WxH+X+Y..] - fresh Xvfb, optionally split into monitors
screen() {
  stop
  Xvfb -displayfd 3 -noreset -screen 0 "$1"x24 3>"$T/fd" >/dev/null 2>&1 &
  XPID=$!
  while [ ! -s "$T/fd" ]; do sleep 0.1; done
  DISPLAY=:$(cat "$T/fd")
  export DISPLAY
  : >"$T/fd"
  shift
  out=screen
  for m in "$@"; do
    xrandr --setmonitor "m$m" "$(echo "$m" | sed 's/x\([0-9]*\)/\/1x\1\/1/')" $out >/dev/null
    out=none
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
for args in "-fill /nonexistent" "-solid nosuchcolor" "-solid" "-addd red" "-blur 3px" "-blur ''" "-alpha 300" "-gradient x" "-bogus" "foo" "-flipv=1" "-solid #ff0000zz"; do
  run -solid red
  eval run -solid blue $args
  rc_is 1
  is 10 10 255,0,0
done

t "-write failure exits 1"
run -solid red -write /nonexistent/x.png
rc_is 1

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
