#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <Imlib2.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "outputs.h"


typedef enum { Full, Fill, Center, Tile, Xtend, Cover } ImageMode;

typedef struct {
  const char *name;
  const char *args[2];
  const char *help;
} Option;

// name NULL = section header
static const Option options[] = {
  {NULL, {NULL}, "Generic Options:"},
  {"root", {NULL}, "Treat multiple displays as one big screen (ignore xrandr outputs)"},
  {"screens", {"<int>"}, "Set a screenmask to use (1 = screen 0, 2 = screen 1, 3 = both, ...)"},
  {"output", {"<name>"}, "Draw only on this output from now on (see hsr-outputs), all for every output"},
  {NULL, {NULL}, "Gradients:"},
  {"add", {"<color>"}, "Add color to range using distance 1"},
  {"addd", {"<color>", "<distance>"}, "Add color to range using custom distance"},
  {"gradient", {"<angle>"}, "Render gradient using specified angle"},
  {"clear", {NULL}, "Clear the color range"},
  {NULL, {NULL}, "Solid:"},
  {"solid", {"<color>"}, "Render a solid using the specified color"},
  {NULL, {NULL}, "Image files:"},
  {"center", {"<image>"}, "Render an image centered on screen"},
  {"cover", {"<image>"}, "Render an image centered on screen scaled to fill the screen fully"},
  {"tile", {"<image>"}, "Render an image tiled"},
  {"full", {"<image>"}, "Render an image maximum aspect"},
  {"extend", {"<image>"}, "Render an image max aspect and fill borders"},
  {"fill", {"<image>"}, "Render an image stretched"},
  {NULL, {NULL}, "Bitmaps (as in xsetroot, tiled from the top left corner):"},
  {"fg", {"<color>"}, "Foreground color for the following bitmaps (default black)"},
  {"bg", {"<color>"}, "Background color for the following bitmaps (default white)"},
  {"rv", {NULL}, "Swap the foreground and background colors"},
  {"reverse", {NULL}, "Same as --rv"},
  {"bitmap", {"<xbm>"}, "Render an X bitmap file tiled"},
  {"mod", {"<x>", "<y>"}, "Render a tiled grid with vertical/horizontal line spacing x/y"},
  {"gray", {NULL}, "Render a gray (checkerboard) pattern"},
  {"grey", {NULL}, "Same as --gray"},
  {NULL, {NULL}, "Manipulations:"},
  {"tint", {"<color>"}, "Tint the current image"},
  {"blur", {"<radius>"}, "Blur the current image"},
  {"sharpen", {"<radius>"}, "Sharpen the current image"},
  {"contrast", {"<amount>"}, "Adjust contrast of current image"},
  {"brightness", {"<amount>"}, "Adjust brightness of current image"},
  {"gamma", {"<amount>"}, "Adjust gamma level of current image"},
  {"flipv", {NULL}, "Flip the current image vertically"},
  {"fliph", {NULL}, "Flip the current image horizontally"},
  {"flipd", {NULL}, "Flip the current image diagonally"},
  {NULL, {NULL}, "Misc:"},
  {"alpha", {"<0-255>"}, "Adjust alpha level for colors and images"},
  {"write", {"<filename>"}, "Write current image to file"},
  {"verbose", {NULL}, "Print which outputs images are drawn on"},
  {"help", {NULL}, "Show this help (also -h)"},
  {"version", {NULL}, "Show version"},
};

#define NOPTIONS (sizeof(options) / sizeof(*options))

// settings, and what uses them - to warn when nothing after a setting does
static const char *uses[][2] = {
  {"fg bg rv reverse", "bitmap mod gray grey"},
  {"alpha", "solid add addd center cover tile full extend fill bitmap mod gray grey"},
  {"add addd", "gradient"},
  {"output", "solid gradient center cover tile full extend fill bitmap mod gray grey tint blur sharpen contrast brightness gamma flipv fliph flipd"},
};

// one parsed command line option, in order
typedef struct {
  const Option *opt;
  char *arg[2];
  Imlib_Image image;
  unsigned char *bits;
  unsigned int bitsW, bitsH;
} Op;

void
usage(FILE *f, char *commandline)
{
  fprintf(f,
    "hsetroot - yet another wallpaper application\n"
    "\n"
    "Syntax: %s [--command1 [arg1..]] [--command2 [arg1..]]...\n"
    "(--command=arg and the old -command forms work too)\n"
  , commandline);

  for (size_t i = 0; i < NOPTIONS; i++) {
    const Option *o = &options[i];
    char buf[64];

    if (!o->name) {
      fprintf(f, "\n%s\n", o->help);
      continue;
    }

    snprintf(buf, sizeof(buf), "%s %s %s", o->name, o->args[0] ? o->args[0] : "", o->args[1] ? o->args[1] : "");
    fprintf(f, " --%-24s %s\n", buf, o->help);
  }

  fprintf(f,
    "\n"
    "Colors are in the #rgb, #rrggbb, #rrggbbaa, rgb:1/2/3 formats or a X color name.\n"
    "\n"
    "Create issues at https://github.com/himdel/hsetroot/issues\n\n"
  );
}

// Globals:
Display *display;
int screen;
int opt_root = false;
int opt_verbose = false;
unsigned long screen_mask = 0;

int
valid_arg(const char *kind, const char *arg)
{
  char *end;
  long l;

  errno = 0;
  if (!strcmp(kind, "<int>")) {
    if (*arg == '-')
      return 0;
    strtoul(arg, &end, 0);
  } else if (!strcmp(kind, "<radius>") || !strcmp(kind, "<distance>") || !strcmp(kind, "<0-255>") || !strcmp(kind, "<x>") || !strcmp(kind, "<y>")) {
    l = strtol(arg, &end, 0);
    if (!strcmp(kind, "<0-255>") && (l < 0 || l > 255))
      return 0;
    if (!strcmp(kind, "<radius>") && (l < 0 || l > INT_MAX))
      return 0;
    if (!strcmp(kind, "<distance>") && (l < INT_MIN || l > INT_MAX))
      return 0;
  } else if (!strcmp(kind, "<amount>") || !strcmp(kind, "<angle>")) {
    if (!isfinite(strtod(arg, &end)))
      return 0;
  } else {
    return 1;
  }

  return *arg && !*end && errno != ERANGE;
}

bool
in_list(const char *list, const char *name)
{
  size_t n = strlen(name);

  for (const char *p = list; (p = strstr(p, name)); p += n)
    if ((p == list || p[-1] == ' ') && (p[n] == ' ' || !p[n]))
      return true;
  return false;
}

// argv -> ops, handles global options, exits on errors
int
parse_args(int argc, char **argv, Op *ops)
{
  int nops = 0;

  for (int i = 1; i < argc; i++) {
    char *name = argv[i], *val = NULL, *eq;
    const Option *o = NULL;

    if (!strcmp(name, "-h"))
      name = "--help";

    if (name[0] == '-') {
      name += (name[1] == '-') ? 2 : 1;
      if ((eq = strchr(name, '='))) {
        *eq = 0;
        val = eq + 1;
      }

      for (size_t j = 0; j < NOPTIONS && !o; j++)
        if (options[j].name && !strcmp(options[j].name, name))
          o = &options[j];
    }

    if (!o) {
      fprintf(stderr, "Unknown option (%s)\n\n", argv[i]);
      usage(stderr, argv[0]);
      exit(1);
    }

    if (val && !o->args[0]) {
      fprintf(stderr, "--%s takes no value\n", name);
      exit(1);
    }

    Op op = { .opt = o };
    for (int j = 0; j < 2 && o->args[j]; j++) {
      op.arg[j] = (j == 0 && val) ? val : (++i < argc) ? argv[i] : NULL;

      if (!op.arg[j]) {
        fprintf(stderr, "Missing %s for --%s\n", o->args[j], name);
        exit(1);
      }
      if (!valid_arg(o->args[j], op.arg[j])) {
        fprintf(stderr, "Bad %s for --%s (%s)\n", o->args[j], name, op.arg[j]);
        exit(1);
      }
    }

    if (!strcmp(name, "help")) {
      usage(stdout, argv[0]);
      exit(0);
    } else if (!strcmp(name, "version")) {
      printf("hsetroot %s\n", VERSION);
      exit(0);
    } else if (!strcmp(name, "verbose")) {
      opt_verbose = true;
    } else if (!strcmp(name, "root")) {
      opt_root = true;
    } else if (!strcmp(name, "screens")) {
      screen_mask = strtoul(op.arg[0], NULL, 0);
      if (!screen_mask) {
        fprintf(stderr, "Bad screen mask (%s): no screens selected\n", op.arg[0]);
        exit(1);
      }
    } else {
      ops[nops++] = op;
    }
  }

  for (int i = 0; i < nops; i++) {
    for (size_t k = 0; k < sizeof(uses) / sizeof(*uses); k++) {
      bool used = false;

      if (!in_list(uses[k][0], ops[i].opt->name))
        continue;
      for (int j = i + 1; j < nops && !used; j++)
        used = in_list(uses[k][1], ops[j].opt->name);
      if (!used)
        fprintf(stderr, "Warning: --%s has no effect, nothing after it uses it\n", ops[i].opt->name);
    }
  }

  return nops;
}

int
ignore_errors(Display *display, XErrorEvent *event)
{
  return 0;
}

// Adapted from fluxbox' bsetroot
int
setRootAtoms(Pixmap pixmap)
{
  Atom atom_root, atom_eroot, type;
  unsigned char *data_root, *data_eroot;
  int format;
  unsigned long length, after;

  atom_root = XInternAtom(display, "_XROOTPMAP_ID", True);
  atom_eroot = XInternAtom(display, "ESETROOT_PMAP_ID", True);

  // doing this to clean up after old background
  if (atom_root != None && atom_eroot != None) {
    XGetWindowProperty(display, RootWindow(display, screen), atom_root, 0L, 1L, False, AnyPropertyType, &type, &format, &length, &after, &data_root);

    if (type == XA_PIXMAP) {
      XGetWindowProperty(display, RootWindow(display, screen), atom_eroot, 0L, 1L, False, AnyPropertyType, &type, &format, &length, &after, &data_eroot);

      // the old owner may be gone already, when it also set another screen we just did
      if (data_root && data_eroot && type == XA_PIXMAP && *((Pixmap *) data_root) == *((Pixmap *) data_eroot)) {
        XSync(display, False);
        XErrorHandler handler = XSetErrorHandler(ignore_errors);
        XKillClient(display, *((Pixmap *) data_root));
        XSync(display, False);
        XSetErrorHandler(handler);
      }
    }
  }

  atom_root = XInternAtom(display, "_XROOTPMAP_ID", False);
  atom_eroot = XInternAtom(display, "ESETROOT_PMAP_ID", False);

  if (atom_root == None || atom_eroot == None)
    return 0;

  // setting new background atoms
  XChangeProperty(display, RootWindow(display, screen), atom_root, XA_PIXMAP, 32, PropModeReplace, (unsigned char *) &pixmap, 1);
  XChangeProperty(display, RootWindow(display, screen), atom_eroot, XA_PIXMAP, 32, PropModeReplace, (unsigned char *) &pixmap, 1);

  return 1;
}

typedef struct {
  int r, g, b, a;
} Color;

int
parse_color(const char *arg, Color *c, int default_alpha)
{
  Colormap colormap = DefaultColormap(display, screen);
  XColor color;
  char buf[8];

  c->a = default_alpha;

  // we support #rrggbbaa..
  if ((arg[0] == '#') && (strlen(arg) == 9)) {
    if (!isxdigit((unsigned char) arg[7]) || !isxdigit((unsigned char) arg[8]))
      return 0;
    if (sscanf(arg + 7, "%2x", &(c->a)) != 1)
      return 0;
    // ..but XParseColor wouldn't
    snprintf(buf, sizeof(buf), "%.7s", arg);
    arg = buf;
  }

  Status ret = XParseColor(display, colormap, arg, &color);
  if (ret == 0)
    return 0;

  c->r = color.red >> 8;
  c->g = color.green >> 8;
  c->b = color.blue >> 8;

  return 1;
}

void
draw_image(ImageMode mode, Imlib_Image image, int alpha, Imlib_Image rootimg, Output *outputs, int noutputs)
{
  int imgW, imgH, o;
  Imlib_Image buffer = image;

  imlib_context_set_image(image);
  imgW = imlib_image_get_width();
  imgH = imlib_image_get_height();

  if (alpha < 255) {
    // Create alpha-override mask, on a copy as image gets reused
    buffer = imlib_clone_image();
    imlib_context_set_image(buffer);
    imlib_image_set_has_alpha(1);
    Imlib_Color_Modifier previous = imlib_context_get_color_modifier();
    Imlib_Color_Modifier modifier = imlib_create_color_modifier();
    imlib_context_set_color_modifier(modifier);

    DATA8 red[256], green[256], blue[256], alph[256];
    imlib_get_color_modifier_tables(red, green, blue, alph);
    for (o = 0; o < 256; o++)
      alph[o] = (DATA8) alpha;
    imlib_set_color_modifier_tables(red, green, blue, alph);

    imlib_apply_color_modifier();
    imlib_free_color_modifier();
    imlib_context_set_color_modifier(previous);
  }

  imlib_context_set_image(rootimg);

  for (int i = 0; i < noutputs; i++) {
    Output o = outputs[i];
    if (opt_verbose)
      printf("output %s: size(%d, %d) pos(%d, %d)\n", o.name, o.width, o.height, o.x, o.y);
    imlib_context_set_cliprect(o.x, o.y, o.width, o.height);

    if (mode == Fill) {
      imlib_blend_image_onto_image(buffer, 0, 0, 0, imgW, imgH, o.x, o.y, o.width, o.height);
    } else if ((mode == Full) || (mode == Xtend) || (mode == Cover)) {
      double aspect = ((double) o.width) / imgW;
      if ((imgH * aspect > o.height) != /*xor*/ (mode == Cover))
        aspect = (double) o.height / (double) imgH;

      // rounded, truncating could leave a 1px gap
      int scaledW = imgW * aspect + 0.5;
      int scaledH = imgH * aspect + 0.5;
      int top = (o.height - scaledH) / 2;
      int left = (o.width - scaledW) / 2;

      imlib_blend_image_onto_image(buffer, 0, 0, 0, imgW, imgH, o.x + left, o.y + top, scaledW, scaledH);

      if (mode == Xtend) {
        int w;

        if (left > 0) {
          int right = left - 1 + scaledW;
          /* check only the right border - left is int divided so the right border is larger */
          for (w = 1; right + w < o.width; w <<= 1) {
            imlib_image_copy_rect(o.x + left + 1 - w, o.y, w, o.height, o.x + left + 1 - w - w, o.y);
            imlib_image_copy_rect(o.x + right, o.y, w, o.height, o.x + right + w, o.y);
          }
        }

        if (top > 0) {
          int bottom = top - 1 + scaledH;
          for (w = 1; (bottom + w < o.height); w <<= 1) {
            imlib_image_copy_rect(o.x, o.y + top + 1 - w, o.width, w, o.x, o.y + top + 1 - w - w);
            imlib_image_copy_rect(o.x, o.y + bottom, o.width, w, o.x, o.y + bottom + w);
          }
        }
      }
    } else {  // Center || Tile
      int left = (o.width - imgW) / 2;
      int top = (o.height - imgH) / 2;

      if (mode == Tile) {
        int x, y;
        for (; left > 0; left -= imgW);
        for (; top > 0; top -= imgH);

        for (x = left; x < o.width; x += imgW)
          for (y = top; y < o.height; y += imgH)
            imlib_blend_image_onto_image(buffer, 0, 0, 0, imgW, imgH, o.x + x, o.y + y, imgW, imgH);
      } else {
        imlib_blend_image_onto_image(buffer, 0, 0, 0, imgW, imgH, o.x + left, o.y + top, imgW, imgH);
      }
    }
  }

  imlib_context_set_cliprect(0, 0, 0, 0);

  if (buffer != image) {
    imlib_context_set_image(buffer);
    imlib_free_image();
  }

  imlib_context_set_image(rootimg);
}

// tile a 1 bit per pixel X bitmap over the area, aligned to the root origin like xsetroot
void
draw_bitmap(const unsigned char *bits, int w, int h, Color fg, Color bg, Imlib_Image rootimg, Output *area)
{
  Imlib_Image tile = imlib_create_image(w, h);

  imlib_context_set_image(tile);
  imlib_image_set_has_alpha(fg.a < 255 || bg.a < 255);
  DATA32 *data = imlib_image_get_data();
  for (int y = 0; y < h; y++) {
    for (int x = 0; x < w; x++) {
      Color c = ((bits[y * ((w + 7) / 8) + x / 8] >> (x % 8)) & 1) ? fg : bg;
      data[y * w + x] = (c.a << 24) | (c.r << 16) | (c.g << 8) | c.b;
    }
  }
  imlib_image_put_back_data(data);

  imlib_context_set_image(rootimg);
  imlib_context_set_cliprect(area->x, area->y, area->width, area->height);
  for (int y = area->y - area->y % h; y < area->y + area->height; y += h)
    for (int x = area->x - area->x % w; x < area->x + area->width; x += w)
      imlib_blend_image_onto_image(tile, 0, 0, 0, w, h, x, y, w, h);
  imlib_context_set_cliprect(0, 0, 0, 0);

  imlib_context_set_image(tile);
  imlib_free_image();
  imlib_context_set_image(rootimg);
}

Output *
find_output(Output *outputs, int noutputs, const char *name)
{
  for (int i = 0; i < noutputs; i++)
    if (!strcmp(outputs[i].name, name))
      return &outputs[i];
  return NULL;
}

void
apply_color_modifier(Output *target)
{
  if (target)
    imlib_apply_color_modifier_to_rectangle(target->x, target->y, target->width, target->height);
  else
    imlib_apply_color_modifier();
  imlib_free_color_modifier();
}

int
main(int argc, char **argv)
{
  Visual *vis;
  Colormap cm;
  Imlib_Image image;
  int width, height, depth, alpha;
  Pixmap pixmap;
  Imlib_Color_Modifier modifier = NULL;
  Op *ops = calloc(argc, sizeof(Op));
  int nops = parse_args(argc, argv, ops);

  /* global */ display = XOpenDisplay(NULL);

  if (!display) {
    fprintf(stderr, "Cannot open X display!\n");
    exit(123);
  }

  if (!screen_mask)
    screen_mask = 1UL << DefaultScreen(display);
  if (!(screen_mask & ((1UL << ScreenCount(display)) - 1))) {
    fprintf(stderr, "No such screen, there are %d\n", ScreenCount(display));
    exit(1);
  }

  // check colors and load images before touching anything
  for (int i = 0; i < nops; i++) {
    Op *op = &ops[i];

    for (int j = 0; j < 2 && op->opt->args[j]; j++) {
      Color c;

      if (!strcmp(op->opt->args[j], "<color>") && !parse_color(op->arg[j], &c, 255)) {
        fprintf(stderr, "Bad color (%s)\n", op->arg[j]);
        exit(1);
      }
      if (!strcmp(op->opt->args[j], "<image>") && !(op->image = imlib_load_image_immediately_without_cache(op->arg[j]))) {
        fprintf(stderr, "Bad image (%s)\n", op->arg[j]);
        exit(1);
      }
      int hx, hy;
      if (!strcmp(op->opt->args[j], "<xbm>") && XReadBitmapFileData(op->arg[j], &op->bitsW, &op->bitsH, &op->bits, &hx, &hy) != BitmapSuccess) {
        fprintf(stderr, "Bad bitmap (%s)\n", op->arg[j]);
        exit(1);
      }
    }
  }

  // imlib2 (1.12.6 too) truncates its color range step above 4096px when distances are small,
  // scaling all of them keeps the same gradient
  long distances = 0;
  for (int i = 0; i < nops; i++)
    if (!strcmp(ops[i].opt->name, "add") || !strcmp(ops[i].opt->name, "addd"))
      distances += ops[i].arg[1] ? labs(strtol(ops[i].arg[1], NULL, 0)) : 1;
  // ..as long as the sum stays below imlib's int overflow at 32768
  int scale = distances ? 32767 / distances : 1;
  if (scale > 256)
    scale = 256;
  if (scale < 1)
    scale = 1;

  bool need_outputs = false;
  for (int i = 0; i < nops; i++) {
    if (ops[i].image || !strcmp(ops[i].opt->name, "gradient"))
      need_outputs = true;

    if (opt_root && !strcmp(ops[i].opt->name, "output") && strcmp(ops[i].arg[0], "all")) {
      fprintf(stderr, "Can't use --output %s with -root\n", ops[i].arg[0]);
      exit(1);
    }
  }

  // Check every selected screen before drawing on any of them.
  if (!opt_root && (screen_mask & (screen_mask - 1))) {
    for (int s = 0; s < ScreenCount(display); s++) {
      if (!(screen_mask & (1UL << s)))
        continue;
      int count = 0;
      Output *available = query_outputs(display, s, &count);
      if (!count && need_outputs) {
        fprintf(stderr, "No outputs found (all turned off?), use -root to draw on the whole screen\n");
        exit(1);
      }
      for (int i = 0; i < nops; i++) {
        if (!strcmp(ops[i].opt->name, "output") && strcmp(ops[i].arg[0], "all") && !find_output(available, count, ops[i].arg[0])) {
          fprintf(stderr, "Unknown output (%s), see hsr-outputs\n", ops[i].arg[0]);
          exit(1);
        }
      }
      free(available);
    }
  }

  for (/* global */ screen = 0; screen < ScreenCount(display); screen++) {
    if ((screen_mask & (1UL << screen)) == 0)
      continue;

    Imlib_Context *context = imlib_context_new();
    imlib_context_push(context);

    imlib_context_set_display(display);
    vis = DefaultVisual(display, screen);
    cm = DefaultColormap(display, screen);
    width = DisplayWidth(display, screen);
    height = DisplayHeight(display, screen);
    depth = DefaultDepth(display, screen);

    int noutputs = 1;
    Output root = { "root", 0, 0, width, height };
    Output *outputs = opt_root ? &root : query_outputs(display, screen, &noutputs);

    if (!noutputs && need_outputs) {
      fprintf(stderr, "No outputs found (all turned off?), use -root to draw on the whole screen\n");
      exit(1);
    }

    for (int i = 0; i < nops; i++) {
      if (!strcmp(ops[i].opt->name, "output") && strcmp(ops[i].arg[0], "all") && !find_output(outputs, noutputs, ops[i].arg[0])) {
        fprintf(stderr, "Unknown output (%s), see hsr-outputs\n", ops[i].arg[0]);
        exit(1);
      }
    }

    pixmap = XCreatePixmap(display, RootWindow(display, screen), width, height, depth);

    imlib_context_set_visual(vis);
    imlib_context_set_colormap(cm);
    imlib_context_set_drawable(pixmap);
    imlib_context_set_color_range(imlib_create_color_range());

    image = imlib_create_image(width, height);
    imlib_context_set_image(image);

    imlib_context_set_color(0, 0, 0, 255);
    imlib_image_fill_rectangle(0, 0, width, height);

    imlib_context_set_dither(1);
    imlib_context_set_blend(1);

    alpha = 255;
    Output *target = NULL;
    const char *fg = "black", *bg = "white";

    for (int i = 0; i < nops; i++) {
      Op *op = &ops[i];
      const char *name = op->opt->name;
      Output *area = target ? target : &root;
      Color c;

      if (modifier != NULL)
        apply_color_modifier(target);
      modifier = imlib_create_color_modifier();
      imlib_context_set_color_modifier(modifier);

      // these work on the whole image, give them just the target output's part
      bool crop = target && (!strcmp(name, "blur") || !strcmp(name, "sharpen") || !strncmp(name, "flip", 4));
      if (crop)
        imlib_context_set_image(imlib_create_cropped_image(target->x, target->y, target->width, target->height));

      if (!strcmp(name, "output")) {
        target = find_output(outputs, noutputs, op->arg[0]);
      } else if (!strcmp(name, "fg")) {
        fg = op->arg[0];
      } else if (!strcmp(name, "bg")) {
        bg = op->arg[0];
      } else if (!strcmp(name, "rv") || !strcmp(name, "reverse")) {
        const char *swap = fg;
        fg = bg;
        bg = swap;
      } else if (!strcmp(name, "bitmap") || !strcmp(name, "mod") || !strcmp(name, "gray") || !strcmp(name, "grey")) {
        // the built-in patterns are xsetroot's
        unsigned char gray[] = { 0x01, 0x02 }, mod[32], *bits = gray;
        int w = 2, h = 2;
        Color f, b;

        if (op->bits) {
          bits = op->bits;
          w = op->bitsW;
          h = op->bitsH;
        } else if (!strcmp(name, "mod")) {
          int mx = strtol(op->arg[0], NULL, 0), my = strtol(op->arg[1], NULL, 0), line = 0;
          bits = mod;
          w = h = 16;
          for (int j = 16; j--; )
            line = (line << 1) | (mx <= 0 || j % mx == 0);
          for (int j = 0; j < 16; j++) {
            mod[j * 2] = (my <= 0 || j % my == 0) ? 0xff : line & 0xff;
            mod[j * 2 + 1] = (my <= 0 || j % my == 0) ? 0xff : line >> 8;
          }
        }

        parse_color(fg, &f, alpha);
        parse_color(bg, &b, alpha);
        draw_bitmap(bits, w, h, f, b, image, area);
      } else if (!strcmp(name, "alpha")) {
        alpha = strtol(op->arg[0], NULL, 0);
      } else if (!strcmp(name, "solid")) {
        parse_color(op->arg[0], &c, alpha);
        imlib_context_set_color(c.r, c.g, c.b, c.a);
        imlib_image_fill_rectangle(area->x, area->y, area->width, area->height);
      } else if (!strcmp(name, "clear")) {
        imlib_free_color_range();
        imlib_context_set_color_range(imlib_create_color_range());
      } else if (!strcmp(name, "add") || !strcmp(name, "addd")) {
        parse_color(op->arg[0], &c, alpha);
        imlib_context_set_color(c.r, c.g, c.b, c.a);
        imlib_add_color_to_color_range(scale * (op->arg[1] ? strtol(op->arg[1], NULL, 0) : 1));
      } else if (!strcmp(name, "gradient")) {
        for (int j = 0; j < (target ? 1 : noutputs); j++) {
          Output *o = target ? target : &outputs[j];
          imlib_image_fill_color_range_rectangle(o->x, o->y, o->width, o->height, strtod(op->arg[0], NULL));
        }
      } else if (op->image) {
        ImageMode mode = !strcmp(name, "fill") ? Fill
                       : !strcmp(name, "full") ? Full
                       : !strcmp(name, "extend") ? Xtend
                       : !strcmp(name, "tile") ? Tile
                       : !strcmp(name, "center") ? Center
                       : Cover;
        draw_image(mode, op->image, alpha, image, target ? target : outputs, target ? 1 : noutputs);
      } else if (!strcmp(name, "tint")) {
        DATA8 r[256], g[256], b[256], a[256];

        parse_color(op->arg[0], &c, 255);
        imlib_get_color_modifier_tables(r, g, b, a);

        for (int j = 0; j < 256; j++) {
          r[j] = (DATA8) (((double) r[j] / 255.0) * (double) c.r);
          g[j] = (DATA8) (((double) g[j] / 255.0) * (double) c.g);
          b[j] = (DATA8) (((double) b[j] / 255.0) * (double) c.b);
        }

        imlib_set_color_modifier_tables(r, g, b, a);
      } else if (!strcmp(name, "blur")) {
        imlib_image_blur(strtol(op->arg[0], NULL, 0));
      } else if (!strcmp(name, "sharpen")) {
        imlib_image_sharpen(strtol(op->arg[0], NULL, 0));
      } else if (!strcmp(name, "contrast")) {
        imlib_modify_color_modifier_contrast(strtod(op->arg[0], NULL));
      } else if (!strcmp(name, "brightness")) {
        imlib_modify_color_modifier_brightness(strtod(op->arg[0], NULL));
      } else if (!strcmp(name, "gamma")) {
        imlib_modify_color_modifier_gamma(strtod(op->arg[0], NULL));
      } else if (!strcmp(name, "flipv")) {
        imlib_image_flip_vertical();
      } else if (!strcmp(name, "fliph")) {
        imlib_image_flip_horizontal();
      } else if (!strcmp(name, "flipd")) {
        imlib_image_flip_diagonal();
      } else if (!strcmp(name, "write")) {
        int err = 0;
        imlib_save_image_with_errno_return(op->arg[0], &err);
        if (err) {
          fprintf(stderr, "Can't write (%s): %s\n", op->arg[0], imlib_strerror(err));
          exit(1);
        }
      }

      if (crop) {
        Imlib_Image part = imlib_context_get_image();
        int w = imlib_image_get_width(), h = imlib_image_get_height();

        imlib_context_set_image(image);
        imlib_context_set_blend(0);
        imlib_blend_image_onto_image(part, 1, 0, 0, w, h, target->x, target->y, target->width, target->height);
        imlib_context_set_blend(1);
        imlib_context_set_image(part);
        imlib_free_image();
        imlib_context_set_image(image);
      }
    }

    if (modifier != NULL) {
      imlib_context_set_color_modifier(modifier);
      apply_color_modifier(target);
      modifier = NULL;
    }

    imlib_render_image_on_drawable(0, 0);
    imlib_free_image();
    imlib_free_color_range();

    if (setRootAtoms(pixmap) == 0)
      fprintf(stderr, "Couldn't create atoms...\n");

    XKillClient(display, AllTemporary);
    XSetCloseDownMode(display, RetainTemporary);

    XSetWindowBackgroundPixmap(display, RootWindow(display, screen), pixmap);
    XClearWindow(display, RootWindow(display, screen));

    XFlush(display);
    XSync(display, False);

    // drop imlib's cached XImages and GCs, they don't work on another screen's root
    imlib_context_disconnect_display();
    imlib_context_pop();
    imlib_context_free(context);

    if (!opt_root)
      free(outputs);
  }

  return 0;
}
