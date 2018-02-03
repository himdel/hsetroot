#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <Imlib2.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "outputs.h"
#include "options.h"


void
usage(char *commandline)
{
  printf(
    "hsetroot - yet another wallpaper application\n"
    "\n"
    "Syntax: %s command1 [arg1..] [command2 [arg1..]]..."
    "\n"
    "Generic Options:\n"
    " -help                      This help information\n"
    " -screens <int>             Set a screenmask to use\n"
    " -outputs                   List screen outputs detected via xrandr\n"
    " -output <name>             Apply to xrandr output 'name' only*\n"
    "\n"
    "*supports: images [all], solid, manipulation [alpha]\n"
    "\n"
    "Gradients:\n"
    " -add <color>               Add color to range using distance 1\n"
    " -addd <color> <distance>   Add color to range using custom distance\n"
    " -gradient <angle>          Render gradient using specified angle\n"
    " -clear                     Clear the color range\n"
    "\n"
    "Solid:\n"
    " -solid <color>             Render a solid using the specified color\n"
    "\n"
    "Image files:\n"
    " -center <image>            Render an image centered on screen\n"
    " -cover <image>             Render an image centered on screen scaled to fill the screen fully\n"
    " -tile <image>              Render an image tiled\n"
    " -full <image>              Render an image maximum aspect\n"
    " -extend <image>            Render an image max aspect and fill borders\n"
    " -fill <image>              Render an image stretched\n"
    "\n"
    "Manipulations:\n"
    " -alpha <amount>            Adjust alpha level for colors and images\n"
    " -blur <radius>             Blur the current image\n"
    " -sharpen <radius>          Sharpen the current image\n"
    " -tint <color>              Tint the current image\n"
    " -contrast <amount>         Adjust contrast of current image\n"
    " -brightness <amount>       Adjust brightness of current image\n"
    " -gamma <amount>            Adjust gamma level of current image\n"
    " -flipv                     Flip the current image vertically\n"
    " -fliph                     Flip the current image horizontally\n"
    " -flipd                     Flip the current image diagonally\n"
    "\n"
    "Misc:\n"
    " -write <filename>          Write current image to file\n"
    "\n"
    "Colors are in the #rrggbb or #rrggbbaa format.\n"
    "\n"
    "Create issues at https://github.com/himdel/hsetroot/issues\n\n"
  , commandline);
}

// Globals:
Display *display;
int screen;

// Adapted from fluxbox' bsetroot
int
setRootAtoms(Pixmap pixmap)
{
  Atom atom_root, atom_eroot, type;
  unsigned char *data_root, *data_eroot;
  int format;
  unsigned long length, after;

  atom_root = XInternAtom(display, "_XROOTMAP_ID", True);
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
    return false;

  // setting new background atoms
  XChangeProperty(display, RootWindow(display, screen), atom_root, XA_PIXMAP, 32, PropModeReplace, (unsigned char *) &pixmap, 1);
  XChangeProperty(display, RootWindow(display, screen), atom_eroot, XA_PIXMAP, 32, PropModeReplace, (unsigned char *) &pixmap, 1);

  return true;
}

typedef struct {
  int r, g, b, a;
} Color, *PColor;

int
getHex(char c)
{
  switch (c) {
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      return c - '0';

    case 'A':
    case 'B':
    case 'C':
    case 'D':
    case 'E':
    case 'F':
      return c - 'A' + 10;

    case 'a':
    case 'b':
    case 'c':
    case 'd':
    case 'e':
    case 'f':
      return c - 'a' + 10;

    default:
      return 0;
  }
}

int
parse_color(char *arg, PColor c, int a)
{
  if (arg[0] != '#')
    return false;

  if ((strlen(arg) != 7) && (strlen(arg) != 9))
    return false;

  c->r = getHex(arg[1]) * 16 + getHex(arg[2]);
  c->g = getHex(arg[3]) * 16 + getHex(arg[4]);
  c->b = getHex(arg[5]) * 16 + getHex(arg[6]);
  c->a = a;

  if (strlen(arg) == 9)
    c->a = getHex(arg[7]) * 16 + getHex(arg[8]);

  return true;
}

int
overlay_effects(Imlib_Image root_image, Options *options)
{
  int res = true;
  int width, height;

  OutputInfo *outputs = options->outputs;
  int noutputs = options->output_count;

  imlib_context_set_image(root_image);
  width = imlib_image_get_width();
  height = imlib_image_get_height();

  Imlib_Image image = root_image;

  for (int i = 0; i < noutputs; i++) {
    OutputInfo o = outputs[i];

    if (options->effects_mode == Output) {
      // use a buffer
      width = o.w;
      height = o.h;
      image = imlib_create_image(width, height);
      imlib_context_set_image(image);
    }

    if (options->set_solid) {
      Color c;
      parse_color(options->solid_colour, &c, 255);
      imlib_context_set_color(c.r, c.g, c.b, c.a);
      imlib_image_fill_rectangle(0, 0, width, height);
    }

    if (options->set_alpha) {
      // create alpha-override mask
      imlib_image_set_has_alpha(1);
      Imlib_Color_Modifier modifier = imlib_create_color_modifier();
      imlib_context_set_color_modifier(modifier);

      DATA8 red[256], green[256], blue[256], alpha[256];
      imlib_get_color_modifier_tables(red, green, blue, alpha);
      for (int i = 0; i < 256; i++)
        alpha[i] = (DATA8) options->alpha_level;
      imlib_set_color_modifier_tables(red, green, blue, alpha);

      imlib_apply_color_modifier();
      imlib_free_color_modifier();
    }

    if (options->effects_mode == Output) {
      // overlay modified buffer to root
      imlib_context_set_image(root_image);
      imlib_blend_image_onto_image(image, 0, 0, 0, o.w, o.h, o.x, o.y, o.w, o.h);
      imlib_context_set_image(image);
      imlib_free_image();
    }
  }

  imlib_context_set_image(root_image);

  return res;
}

int
overlay_image(Imlib_Image root_image, Options *options)
{
  int res = true;
  int width, height;
  Imlib_Image image = imlib_load_image(options->image_mode_arg);
  if (!image)
    return false;

  OutputInfo *outputs = options->outputs;
  int noutputs = options->output_count;
  ImageMode image_mode = options->image_mode;

  options->effects_mode = Root;
  if (!overlay_effects(image, options)) {
    printf("error overlaying image effects\n");
    return false;
  }

  imlib_context_set_image(image);
  width = imlib_image_get_width();
  height = imlib_image_get_height();

  imlib_context_set_image(root_image);
  for (int i = 0; i < noutputs; i++) {
    OutputInfo o = outputs[i];

    if (image_mode == Fill) {
      imlib_blend_image_onto_image(image, 0, 0, 0, width, height, o.x, o.y, o.w, o.h);
    } else if ((image_mode == Full) || (image_mode == Xtend) || (image_mode == Cover)) {
      double aspect = ((double) o.w) / width;
      if (((int) (height * aspect) > o.h) != /*xor*/ (image_mode == Cover))
        aspect = (double) o.h / (double) height;

      int top = (o.h - (int) (height * aspect)) / 2;
      int left = (o.w - (int) (width * aspect)) / 2;

      imlib_blend_image_onto_image(image, 0, 0, 0, width, height, o.x + left, o.y + top, (int) (width * aspect), (int) (height * aspect));

      if (image_mode == Xtend) {
        int w;

        if (left > 0) {
          int right = left - 1 + (int) (width * aspect);
          /* check only the right border - left is int divided so the right border is larger */
          for (w = 1; right + w < o.w; w <<= 1) {
            imlib_image_copy_rect(o.x + left + 1 - w, o.y, w, o.h, o.x + left + 1 - w - w, o.y);
            imlib_image_copy_rect(o.x + right, o.y, w, o.h, o.x + right + w, o.y);
          }
        }

        if (top > 0) {
          int bottom = top - 1 + (int) (height * aspect);
          for (w = 1; (bottom + w < o.h); w <<= 1) {
            imlib_image_copy_rect(o.x, o.y + top + 1 - w, o.w, w, o.x, o.y + top + 1 - w - w);
            imlib_image_copy_rect(o.x, o.y + bottom, o.w, w, o.x, o.y + bottom + w);
          }
        }
      }
    } else {  // Center || Tile
      int left = (o.w - width) / 2;
      int top = (o.h - height) / 2;

      if (image_mode == Tile) {
        int x, y;
        for (; left > 0; left -= width);
        for (; top > 0; top -= height);

        for (x = left; x < o.w; x += width)
          for (y = top; y < o.h; y += height)
            imlib_blend_image_onto_image(image, 0, 0, 0, width, height, o.x + x, o.y + y, width, height);
      } else {
        imlib_blend_image_onto_image(image, 0, 0, 0, width, height, o.x + left, o.y + top, width, height);
      }
    }
  }

  imlib_context_set_image(image);
  imlib_free_image();

  imlib_context_set_image(root_image);

  return res;
}

int
push_to_root(Imlib_Image root_image, Options *options)
{
  if (options->image_mode != Unset) {
    if (!overlay_image(root_image, options)) {
      printf("error overlaying image\n");
      return false;
    }
  } else {
    if (!overlay_effects(root_image, options)) {
      printf("error overlaying effects\n");
      return false;
    }
  }
  return true;
}

int
main(int argc, char **argv)
{
  Visual *vis;
  Colormap cm;
  Display *_display;
  Imlib_Context *context;
  Imlib_Image root_image;
  int width, height, depth, i;
  Pixmap pixmap;
  Imlib_Color_Modifier modifier = NULL;
  unsigned long screen_mask = ~0;

  /* global */ _display = XOpenDisplay(NULL);

  if (!_display) {
    fprintf(stderr, "Cannot open X display!\n");
    exit(123);
  }

  if (argc < 2) {
    usage(argv[0]);
    exit(0);
  }

  // global options
  for (i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-help") == 0) {
      usage(argv[0]);
      exit(0);
    }
  }
  for (i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-screens"))
      continue;

    int intval;
    if ((++i) >= argc) {
      fprintf(stderr, "Missing value\n");
      continue;
    }
    if ((!sscanf(argv[i], "%i", &intval)) || (intval < 0)) {
      fprintf(stderr, "Bad value (%s)\n", argv[i]);
      continue;
    }
    screen_mask = intval;
  }

  for (/* global */ screen = 0; screen < ScreenCount(_display); screen++) {
    if ((screen_mask & (1 << screen)) == 0)
      continue;

    display = XOpenDisplay(NULL);

    // global options
    int processed = 0;
    for (i = 1; i < argc; i++) {
      if (strcmp(argv[i], "-outputs") == 0) {
        processed = 1;

        Outputs outputs;
        outputs_set(&outputs);
        for (int i = 0; i < outputs.noutputs; i++) {
          outputs_print(outputs.infos[i]);
        }
        outputs_free(&outputs);

        break;
      }
    }

    if (processed == 1)
      continue;

    Options options;
    options_default(&options);

    context = imlib_context_new();
    imlib_context_push(context);

    imlib_context_set_display(display);
    vis = DefaultVisual(display, screen);
    cm = DefaultColormap(display, screen);
    width = DisplayWidth(display, screen);
    height = DisplayHeight(display, screen);
    depth = DefaultDepth(display, screen);

    pixmap = XCreatePixmap(display, RootWindow(display, screen), width, height, depth);

    imlib_context_set_visual(vis);
    imlib_context_set_colormap(cm);
    imlib_context_set_drawable(pixmap);
    imlib_context_set_color_range(imlib_create_color_range());

    root_image = imlib_create_image(width, height);
    imlib_context_set_image(root_image);

    imlib_context_set_color(0, 0, 0, 255);
    imlib_image_fill_rectangle(0, 0, width, height);

    imlib_context_set_dither(1);
    imlib_context_set_blend(1);

    Outputs outputs;
    outputs_set(&outputs);
    options.outputs = outputs.infos;
    options.output_count = outputs.noutputs;

    int options_set = 0;
    for (i = 1; i < argc; i++) {
      ////////////////////
      // display options
      if (strcmp(argv[i], "-screens") == 0) {
        /* handled as global, just skipping here, + arg */
        i++;
      } else if (strcmp(argv[i], "-output") == 0) {
        // push previous options set
        if (options_set == 1) {
          if (!push_to_root(root_image, &options))
            break;
        }
        if ((++i) >= argc) {
          fprintf(stderr, "Missing output name\n");
          break;
        }
        options_set = 0;
        options_default(&options);
        OutputInfo* output = outputs_by_name(&outputs, argv[i]);
        if (output == NULL) {
          fprintf (stderr, "Bad output name '%s'\n", argv[i]);
          break;
        }
        // single output mode
        options.outputs = output;
        options.output_count = 1;

      ////////////////////
      // image options
      } else if (strcmp(argv[i], "-fill") == 0) {
        if ((++i) >= argc) {
          fprintf(stderr, "Missing image\n");
          continue;
        }
        options.image_mode = Fill;
        options.image_mode_arg = argv[i];
        options_set = 1;
      } else if (strcmp(argv[i], "-full") == 0) {
        if ((++i) >= argc) {
          fprintf(stderr, "Missing image\n");
          continue;
        }
        options.image_mode = Full;
        options.image_mode_arg = argv[i];
        options_set = 1;
      } else if (strcmp(argv[i], "-extend") == 0) {
        if ((++i) >= argc) {
          fprintf(stderr, "Missing image\n");
          continue;
        }
        options.image_mode = Xtend;
        options.image_mode_arg = argv[i];
        options_set = 1;
      } else if (strcmp(argv[i], "-tile") == 0) {
        if ((++i) >= argc) {
          fprintf(stderr, "Missing image\n");
          continue;
        }
        options.image_mode = Tile;
        options.image_mode_arg = argv[i];
        options_set = 1;
      } else if (strcmp(argv[i], "-center") == 0) {
        if ((++i) >= argc) {
          fprintf(stderr, "Missing image\n");
          continue;
        }
        options.image_mode = Center;
        options.image_mode_arg = argv[i];
        options_set = 1;
      } else if (strcmp(argv[i], "-cover") == 0) {
        if ((++i) >= argc) {
          fprintf(stderr, "Missing image\n");
          continue;
        }
        options.image_mode = Cover;
        options.image_mode_arg = argv[i];
        options_set = 1;

      ////////////////////
      // solid options
       } else if (strcmp(argv[i], "-solid") == 0) {
        Color c;
        if ((++i) >= argc) {
          fprintf(stderr, "Missing color\n");
          continue;
        }
        if (!parse_color(argv[i], &c, 255)) {
          fprintf (stderr, "Bad solid color (%s)\n", argv[i]);
          continue;
        }
        options.set_solid = 1;
        options.solid_colour = argv[i];
        options_set = 1;

      ////////////////////
      // manipulation options
      } else if (strcmp(argv[i], "-alpha") == 0) {
        if ((++i) >= argc) {
          fprintf (stderr, "Missing alpha value\n");
          continue;
        }
        if (!sscanf(argv[i], "%i", &options.alpha_level)) {
          fprintf (stderr, "Bad alpha value (%s)\n", argv[i]);
          continue;
        }
        options.set_alpha = 1;
        options_set = 1;

      ////////////////////
      // global only options
      } else {
        // push previous options set
        if (options_set == 1) {
          if (!push_to_root(root_image, &options))
            break;
          options_set = 0;
          options_default(&options);
        }

        if (modifier != NULL) {
          imlib_apply_color_modifier();
          imlib_free_color_modifier();
        }
        modifier = imlib_create_color_modifier();
        imlib_context_set_color_modifier(modifier);

        if (strcmp(argv[i], "-clear") == 0) {
          imlib_free_color_range();
          imlib_context_set_color_range(imlib_create_color_range());
        } else if (strcmp(argv[i], "-add") == 0) {
          Color c;
          if ((++i) >= argc) {
            fprintf(stderr, "Missing color\n");
            continue;
          }
          if (!parse_color(argv[i], &c, options.alpha_level)) {
            fprintf (stderr, "Bad color (%s)\n", argv[i - 1]);
            continue;
          }
          imlib_context_set_color(c.r, c.g, c.b, c.a);
          imlib_add_color_to_color_range(1);
        } else if (strcmp(argv[i], "-addd") == 0) {
          Color c;
          int distance;
          if ((++i) >= argc) {
            fprintf(stderr, "Missing color\n");
            continue;
          }
          if ((++i) >= argc) {
            fprintf(stderr, "Missing distance\n");
            continue;
          }
          if (!parse_color(argv[i - 1], &c, options.alpha_level)) {
            fprintf (stderr, "Bad color (%s)\n", argv[i - 1]);
            continue;
          }
          if (!sscanf(argv[i], "%i", &distance)) {
            fprintf(stderr, "Bad distance (%s)\n", argv[i]);
            continue;
          }
          imlib_context_set_color(c.r, c.g, c.b, c.a);
          imlib_add_color_to_color_range(distance);
        } else if (strcmp(argv[i], "-gradient") == 0) {
          int angle;
          if ((++i) >= argc) {
            fprintf(stderr, "Missing angle\n");
            continue;
          }
          if (!sscanf(argv[i], "%i", &angle)) {
            fprintf (stderr, "Bad angle (%s)\n", argv[i]);
            continue;
          }
          imlib_image_fill_color_range_rectangle(0, 0, width, height, angle);
        } else if (strcmp(argv[i], "-tint") == 0) {
          Color c;
          DATA8 r[256], g[256], b[256], a[256];
          int j;

          if ((++i) >= argc) {
            fprintf(stderr, "Missing color\n");
            continue;
          }
          if (!parse_color(argv[i], &c, 255)) {
            fprintf(stderr, "Bad color\n");
            continue;
          }

          imlib_get_color_modifier_tables(r, g, b, a);

          for (j = 0; j < 256; j++) {
            r[j] = (DATA8) (((double) r[j] / 255.0) * (double) c.r);
            g[j] = (DATA8) (((double) g[j] / 255.0) * (double) c.g);
            b[j] = (DATA8) (((double) b[j] / 255.0) * (double) c.b);
          }

          imlib_set_color_modifier_tables(r, g, b, a);
        } else if (strcmp(argv[i], "-blur") == 0) {
          int intval;
          if ((++i) >= argc) {
            fprintf(stderr, "Missing value\n");
            continue;
          }
          if (!sscanf(argv[i], "%i", &intval)) {
            fprintf(stderr, "Bad value (%s)\n", argv[i]);
            continue;
          }
          imlib_image_blur(intval);
        } else if (strcmp(argv[i], "-sharpen") == 0) {
          int intval;
          if ((++i) >= argc) {
            fprintf(stderr, "Missing value\n");
            continue;
          }
          if (!sscanf(argv[i], "%i", &intval)) {
            fprintf(stderr, "Bad value (%s)\n", argv[i]);
            continue;
          }
          imlib_image_sharpen(intval);
        } else if (strcmp(argv[i], "-contrast") == 0) {
          double dblval;
          if ((++i) >= argc) {
            fprintf(stderr, "Missing value\n");
            continue;
          }
          if (!sscanf(argv[i], "%lf", &dblval)) {
            fprintf(stderr, "Bad value (%s)\n", argv[i]);
            continue;
          }
          imlib_modify_color_modifier_contrast(dblval);
        } else if (strcmp(argv[i], "-brightness") == 0) {
          double dblval;
          if ((++i) >= argc) {
            fprintf(stderr, "Missing value\n");
            continue;
          }
          if (!sscanf(argv[i], "%lf", &dblval)) {
            fprintf(stderr, "Bad value (%s)\n", argv[i]);
            continue;
          }
          imlib_modify_color_modifier_brightness(dblval);
        } else if (strcmp(argv[i], "-gamma") == 0) {
          double dblval;
          if ((++i) >= argc) {
            fprintf(stderr, "Missing value\n");
            continue;
          }
          if (!sscanf(argv[i], "%lf", &dblval)) {
            fprintf(stderr, "Bad value (%s)\n", argv[i]);
            continue;
          }
          imlib_modify_color_modifier_gamma(dblval);
        } else if (strcmp(argv[i], "-flipv") == 0) {
          imlib_image_flip_vertical();
        } else if (strcmp(argv[i], "-fliph") == 0) {
          imlib_image_flip_horizontal();
        } else if (strcmp(argv[i], "-flipd") == 0) {
          imlib_image_flip_diagonal();
        } else if (strcmp(argv[i], "-write") == 0) {
          if ((++i) >= argc) {
            fprintf(stderr, "Missing filename\n");
            continue;
          }
          imlib_save_image(argv[i]);
        } else {
          imlib_free_image();
          imlib_free_color_range();
          if (modifier != NULL) {
            imlib_context_set_color_modifier(modifier);
            imlib_free_color_modifier();
            modifier = NULL;
          }
          XFreePixmap(display, pixmap);
          usage(argv[0]);
          fprintf(stderr, "\nunsupported option '%s'\n", argv[i]);
          exit(1);
        }
      }
    }

    // push final option set
    if (options_set == 1)
      push_to_root(root_image, &options);

    // render root image and cleanup
    if (modifier != NULL) {
      imlib_context_set_color_modifier(modifier);
      imlib_apply_color_modifier();
      imlib_free_color_modifier();
      modifier = NULL;
    }

    imlib_render_image_on_drawable(0, 0);
    imlib_free_image();
    imlib_free_color_range();

    if (!setRootAtoms(pixmap))
      fprintf(stderr, "Couldn't create atoms...\n");

    XKillClient(display, AllTemporary);
    XSetCloseDownMode(display, RetainTemporary);

    XSetWindowBackgroundPixmap(display, RootWindow(display, screen), pixmap);
    XClearWindow(display, RootWindow(display, screen));

    XFlush(display);
    XSync(display, False);

    imlib_context_pop();
    imlib_context_free(context);

    if (outputs.infos != NULL)
      outputs_free(&outputs);
  }

  return 0;
}
