#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xinerama.h>
#include <Imlib2.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


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
  {"screens", {"<int>"}, "Set a screenmask to use"},
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
  {"help", {NULL}, "Show this help (also -h)"},
  {"version", {NULL}, "Show version"},
};

#define NOPTIONS (sizeof(options) / sizeof(*options))

// one parsed command line option, in order
typedef struct {
  const Option *opt;
  char *arg[2];
  Imlib_Image image;
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
unsigned long screen_mask = ~0;

int
valid_arg(const char *kind, const char *arg)
{
  char *end;
  long l;

  if (!strcmp(kind, "<int>") || !strcmp(kind, "<radius>") || !strcmp(kind, "<distance>") || !strcmp(kind, "<0-255>")) {
    l = strtol(arg, &end, 0);
    if (!strcmp(kind, "<0-255>") && (l < 0 || l > 255))
      return 0;
  } else if (!strcmp(kind, "<amount>") || !strcmp(kind, "<angle>")) {
    strtod(arg, &end);
  } else {
    return 1;
  }

  return *arg && !*end;
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
    } else if (!strcmp(name, "root")) {
      opt_root = true;
    } else if (!strcmp(name, "screens")) {
      screen_mask = strtoul(op.arg[0], NULL, 0);
    } else {
      ops[nops++] = op;
    }
  }

  return nops;
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

      if (data_root && data_eroot && type == XA_PIXMAP && *((Pixmap *) data_root) == *((Pixmap *) data_eroot))
        XKillClient(display, *((Pixmap *) data_root));
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
draw_image(ImageMode mode, Imlib_Image image, int alpha, Imlib_Image rootimg, XineramaScreenInfo *outputs, int noutputs)
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
    Imlib_Color_Modifier modifier = imlib_create_color_modifier();
    imlib_context_set_color_modifier(modifier);

    DATA8 red[256], green[256], blue[256], alph[256];
    imlib_get_color_modifier_tables(red, green, blue, alph);
    for (o = 0; o < 256; o++)
      alph[o] = (DATA8) alpha;
    imlib_set_color_modifier_tables(red, green, blue, alph);

    imlib_apply_color_modifier();
    imlib_free_color_modifier();
  }

  imlib_context_set_image(rootimg);

  for (int i = 0; i < noutputs; i++) {
    XineramaScreenInfo o = outputs[i];
    printf("output %d: size(%d, %d) pos(%d, %d)\n", i, o.width, o.height, o.x_org, o.y_org);
    imlib_context_set_cliprect(o.x_org, o.y_org, o.width, o.height);

    if (mode == Fill) {
      imlib_blend_image_onto_image(buffer, 0, 0, 0, imgW, imgH, o.x_org, o.y_org, o.width, o.height);
    } else if ((mode == Full) || (mode == Xtend) || (mode == Cover)) {
      double aspect = ((double) o.width) / imgW;
      if (((int) (imgH * aspect) > o.height) != /*xor*/ (mode == Cover))
        aspect = (double) o.height / (double) imgH;

      int top = (o.height - (int) (imgH * aspect)) / 2;
      int left = (o.width - (int) (imgW * aspect)) / 2;

      imlib_blend_image_onto_image(buffer, 0, 0, 0, imgW, imgH, o.x_org + left, o.y_org + top, (int) (imgW * aspect), (int) (imgH * aspect));

      if (mode == Xtend) {
        int w;

        if (left > 0) {
          int right = left - 1 + (int) (imgW * aspect);
          /* check only the right border - left is int divided so the right border is larger */
          for (w = 1; right + w < o.width; w <<= 1) {
            imlib_image_copy_rect(o.x_org + left + 1 - w, o.y_org, w, o.height, o.x_org + left + 1 - w - w, o.y_org);
            imlib_image_copy_rect(o.x_org + right, o.y_org, w, o.height, o.x_org + right + w, o.y_org);
          }
        }

        if (top > 0) {
          int bottom = top - 1 + (int) (imgH * aspect);
          for (w = 1; (bottom + w < o.height); w <<= 1) {
            imlib_image_copy_rect(o.x_org, o.y_org + top + 1 - w, o.width, w, o.x_org, o.y_org + top + 1 - w - w);
            imlib_image_copy_rect(o.x_org, o.y_org + bottom, o.width, w, o.x_org, o.y_org + bottom + w);
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
            imlib_blend_image_onto_image(buffer, 0, 0, 0, imgW, imgH, o.x_org + x, o.y_org + y, imgW, imgH);
      } else {
        imlib_blend_image_onto_image(buffer, 0, 0, 0, imgW, imgH, o.x_org + left, o.y_org + top, imgW, imgH);
      }
    }
  }

  if (buffer != image) {
    imlib_context_set_image(buffer);
    imlib_free_image();
  }

  imlib_context_set_image(rootimg);
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
    }
  }

  int noutputs = 0;
  XineramaScreenInfo *outputs = NULL;

  XineramaScreenInfo fake = {
    .x_org = 0,
    .y_org = 0,
    .width = 0,
    .height = 0,
  };

  if (opt_root) {
    noutputs = 1;
    outputs = &fake;
  } else {
    outputs = XineramaQueryScreens(display, &noutputs);
  }

  for (/* global */ screen = 0; screen < ScreenCount(display); screen++) {
    if ((screen_mask & (1 << screen)) == 0)
      continue;

    Imlib_Context *context = imlib_context_new();
    imlib_context_push(context);

    imlib_context_set_display(display);
    vis = DefaultVisual(display, screen);
    cm = DefaultColormap(display, screen);
    width = DisplayWidth(display, screen);
    height = DisplayHeight(display, screen);
    depth = DefaultDepth(display, screen);

    if (opt_root) {
      outputs[0].width = width;
      outputs[0].height = height;
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

    for (int i = 0; i < nops; i++) {
      Op *op = &ops[i];
      const char *name = op->opt->name;
      Color c;

      if (modifier != NULL) {
        imlib_apply_color_modifier();
        imlib_free_color_modifier();
      }
      modifier = imlib_create_color_modifier();
      imlib_context_set_color_modifier(modifier);

      if (!strcmp(name, "alpha")) {
        alpha = strtol(op->arg[0], NULL, 0);
      } else if (!strcmp(name, "solid")) {
        parse_color(op->arg[0], &c, alpha);
        imlib_context_set_color(c.r, c.g, c.b, c.a);
        imlib_image_fill_rectangle(0, 0, width, height);
      } else if (!strcmp(name, "clear")) {
        imlib_free_color_range();
        imlib_context_set_color_range(imlib_create_color_range());
      } else if (!strcmp(name, "add") || !strcmp(name, "addd")) {
        parse_color(op->arg[0], &c, alpha);
        imlib_context_set_color(c.r, c.g, c.b, c.a);
        imlib_add_color_to_color_range(op->arg[1] ? strtol(op->arg[1], NULL, 0) : 1);
      } else if (!strcmp(name, "gradient")) {
        imlib_image_fill_color_range_rectangle(0, 0, width, height, strtod(op->arg[0], NULL));
      } else if (op->image) {
        ImageMode mode = !strcmp(name, "fill") ? Fill
                       : !strcmp(name, "full") ? Full
                       : !strcmp(name, "extend") ? Xtend
                       : !strcmp(name, "tile") ? Tile
                       : !strcmp(name, "center") ? Center
                       : Cover;
        draw_image(mode, op->image, alpha, image, outputs, noutputs);
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
          fprintf(stderr, "Can't write (%s): %s\n", op->arg[0], strerror(err));
          exit(1);
        }
      }
    }

    if (modifier != NULL) {
      imlib_context_set_color_modifier(modifier);
      imlib_apply_color_modifier();
      imlib_free_color_modifier();
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

    imlib_context_pop();
    imlib_context_free(context);
  }

  if (outputs != NULL) {
    if (!opt_root) {
      XFree(outputs);
    }

    outputs = NULL;
    noutputs = 0;
  }

  return 0;
}
