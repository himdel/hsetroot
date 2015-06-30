#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <Imlib2.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

typedef enum
{ Full, Fill, Center, Tile, Xtend } ImageMode;

void
usage (char *commandline)
{
  printf ("hsetroot - yet another wallpaper application\n"
	  "\n"
	  "Syntaxis: %s [command1 [arg1..]] [command2 [arg1..]]..."
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
	  " -tile <image>              Render an image tiled\n"
	  " -full <image>              Render an image maximum aspect\n"
	  " -extend <image>            Render an image max aspect and fill borders\n"
	  " -fill <image>              Render an image strechted\n"
	  "\n"
	  "Manipulations:\n"
	  " -tint <color>	             Tint the current image\n"
	  " -blur <radius>             Blur the current image\n"
	  " -sharpen <radius>          Sharpen the current image\n"
	  " -contrast <amount>         Adjust contrast of current image\n"
	  " -brightness <amount>       Adjust brightness of current image\n"
	  " -gamma <amount>            Adjust gamma level of current image\n"
	  " -flipv                     Flip the current image vertically\n"
	  " -fliph                     Flip the current image horizontally\n"
	  " -flipd                     Flip the current image diagonally\n"
	  "\n"
	  "Misc:\n"
	  " -alpha <amount>            Adjust alpha level for colors and images\n"
	  " -write <filename>          Write current image to file\n"
	  "\n"
	  "Colors are in the #rrbbgg or #rrggbbaa format.\n"
	  "\n"
	  "Send bugreports to: https://github.com/himdel/hsetroot\n\n", commandline);

}

// Globals:
Display *display;
int screen;

// Adapted from fluxbox' bsetroot
int
setRootAtoms (Pixmap pixmap)
{
  Atom atom_root, atom_eroot, type;
  unsigned char *data_root, *data_eroot;
  int format;
  unsigned long length, after;

  atom_root = XInternAtom (display, "_XROOTMAP_ID", True);
  atom_eroot = XInternAtom (display, "ESETROOT_PMAP_ID", True);

  // doing this to clean up after old background
  if (atom_root != None && atom_eroot != None)
    {
      XGetWindowProperty (display, RootWindow (display, screen),
			  atom_root, 0L, 1L, False, AnyPropertyType,
			  &type, &format, &length, &after, &data_root);
      if (type == XA_PIXMAP)
	{
	  XGetWindowProperty (display, RootWindow (display, screen),
			      atom_eroot, 0L, 1L, False, AnyPropertyType,
			      &type, &format, &length, &after, &data_eroot);
	  if (data_root && data_eroot && type == XA_PIXMAP &&
	      *((Pixmap *) data_root) == *((Pixmap *) data_eroot))
	    {
	      XKillClient (display, *((Pixmap *) data_root));
	    }
	}
    }

  atom_root = XInternAtom (display, "_XROOTPMAP_ID", False);
  atom_eroot = XInternAtom (display, "ESETROOT_PMAP_ID", False);

  if (atom_root == None || atom_eroot == None)
    return 0;

  // setting new background atoms
  XChangeProperty (display, RootWindow (display, screen),
		   atom_root, XA_PIXMAP, 32, PropModeReplace,
		   (unsigned char *) &pixmap, 1);
  XChangeProperty (display, RootWindow (display, screen), atom_eroot,
		   XA_PIXMAP, 32, PropModeReplace, (unsigned char *) &pixmap,
		   1);

  return 1;
}

typedef struct
{
  int r, g, b, a;
} Color, *PColor;

int
getHex (char c)
{
  switch (c)
    {
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
parse_color (char *arg, PColor c, int a)
{
  if (arg[0] != '#')
    return 0;

  if ((strlen (arg) != 7) && (strlen (arg) != 9))
    return 0;

  c->r = getHex (arg[1]) * 16 + getHex (arg[2]);
  c->g = getHex (arg[3]) * 16 + getHex (arg[4]);
  c->b = getHex (arg[5]) * 16 + getHex (arg[6]);
  c->a = a;
  if (strlen (arg) == 9)
    c->a = getHex (arg[7]) * 16 + getHex (arg[8]);

  return 1;
}

int
load_image (ImageMode mode, const char *arg, int rootW, int rootH, int alpha,
	    Imlib_Image rootimg)
{
  int imgW, imgH, o;
  Imlib_Image buffer = imlib_load_image (arg);

  if (!buffer)
    return 0;

  imlib_context_set_image (buffer);
  imgW = imlib_image_get_width (), imgH = imlib_image_get_height ();

  if (alpha < 255)
    {
      // Create alpha-override mask
      imlib_image_set_has_alpha (1);
      Imlib_Color_Modifier modifier = imlib_create_color_modifier ();
      imlib_context_set_color_modifier (modifier);

      DATA8 red[256], green[256], blue[256], alph[256];
      imlib_get_color_modifier_tables (red, green, blue, alph);
      for (o = 0; o < 256; o++)
	alph[o] = (DATA8) alpha;
      imlib_set_color_modifier_tables (red, green, blue, alph);

      imlib_apply_color_modifier ();
      imlib_free_color_modifier ();
    }

  imlib_context_set_image (rootimg);
  if (mode == Fill)
    {
      imlib_blend_image_onto_image (buffer, 0, 0, 0, imgW, imgH,
				    0, 0, rootW, rootH);
    }
  else if ((mode == Full) || (mode == Xtend))
    {
      double aspect = ((double) rootW) / imgW;
      int top, left;
      if ((int) (imgH * aspect) > rootH)
	aspect = (double) rootH / (double) imgH;
      top = (rootH - (int) (imgH * aspect)) / 2;
      left = (rootW - (int) (imgW * aspect)) / 2;

      imlib_blend_image_onto_image (buffer, 0, 0, 0, imgW, imgH,
				    left, top, (int) (imgW * aspect),
				    (int) (imgH * aspect));

      if (mode == Xtend) {
        int w;
        if ( left >0 ) {
          int right = left - 1 + (int) (imgW * aspect);
          /* check only the right border - left is int divided so the right border is larger */
          for (w = 1; (right + w < rootW); w <<= 1) {
            imlib_image_copy_rect (left + 1 - w, 0, w, rootH, left + 1 - w - w, 0);
            imlib_image_copy_rect (right, 0, w, rootH, right + w, 0);
          }
        }
        if (top >0 ) {
          int bottom = top - 1 + (int) (imgH * aspect);
          for (w = 1; (bottom + w < rootH); w <<= 1) {
            imlib_image_copy_rect (0, top + 1 - w, rootW, w, 0, top + 1 - w - w);
            imlib_image_copy_rect (0, bottom, rootW, w, 0, bottom + w);
          }
        }
      }
    }
  else
    {
      int left = (rootW - imgW) / 2, top = (rootH - imgH) / 2;

      if (mode == Tile)
	{
	  int x, y;
	  for (; left > 0; left -= imgW);
	  for (; top > 0; top -= imgH);

	  for (x = left; x < rootW; x += imgW)
	    for (y = top; y < rootH; y += imgH)
	      imlib_blend_image_onto_image (buffer, 0, 0, 0, imgW, imgH,
					    x, y, imgW, imgH);
	}
      else
	{
	  imlib_blend_image_onto_image (buffer, 0, 0, 0, imgW, imgH,
					left, top, imgW, imgH);
	}
    }

  imlib_context_set_image (buffer);
  imlib_free_image ();

  imlib_context_set_image (rootimg);

  return 1;
}

int
main (int argc, char **argv)
{
  Visual *vis;
  Colormap cm;
  Display *_display;
  Imlib_Context *context;
  Imlib_Image image;
  int width, height, depth, i, alpha;
  Pixmap pixmap;
  Imlib_Color_Modifier modifier = NULL;
  _display = XOpenDisplay (NULL);

  if(!_display){
    fprintf (stderr, "Cannot open X display!\n");
    exit (123);
  }

  for (screen = 0; screen < ScreenCount (_display); screen++)
    {
      display = XOpenDisplay (NULL);

      context = imlib_context_new ();
      imlib_context_push (context);

      imlib_context_set_display (display);
      vis = DefaultVisual (display, screen);
      cm = DefaultColormap (display, screen);
      width = DisplayWidth (display, screen);
      height = DisplayHeight (display, screen);
      depth = DefaultDepth (display, screen);

      pixmap =
	XCreatePixmap (display, RootWindow (display, screen), width, height,
		       depth);

      imlib_context_set_visual (vis);
      imlib_context_set_colormap (cm);
      imlib_context_set_drawable (pixmap);
      imlib_context_set_color_range (imlib_create_color_range ());

      image = imlib_create_image (width, height);
      imlib_context_set_image (image);

      imlib_context_set_color (0, 0, 0, 255);
      imlib_image_fill_rectangle (0, 0, width, height);

      imlib_context_set_dither (1);
      imlib_context_set_blend (1);

      alpha = 255;

      for (i = 1; i < argc; i++)
	{
	  if (modifier != NULL)
	    {
	      imlib_apply_color_modifier ();
	      imlib_free_color_modifier ();
	    }
	  modifier = imlib_create_color_modifier ();
	  imlib_context_set_color_modifier (modifier);

	  if (strcmp (argv[i], "-alpha") == 0)
	    {
	      if ((++i) >= argc)
		{
		  fprintf (stderr, "Missing alpha\n");
		  continue;
		}
	      if (sscanf (argv[i], "%i", &alpha) == 0)
		{
		  fprintf (stderr, "Bad alpha (%s)\n", argv[i]);
		  continue;
		}

	    }
	  else if (strcmp (argv[i], "-solid") == 0)
	    {
	      Color c;
	      if ((++i) >= argc)
		{
		  fprintf (stderr, "Missing color\n");
		  continue;
		}
	      if (parse_color (argv[i], &c, alpha) == 0)
		{
		  fprintf (stderr, "Bad color (%s)\n", argv[i]);
		  continue;
		}
	      imlib_context_set_color (c.r, c.g, c.b, c.a);
	      imlib_image_fill_rectangle (0, 0, width, height);
	    }
	  else if (strcmp (argv[i], "-clear") == 0)
	    {
	      imlib_free_color_range ();
	      imlib_context_set_color_range (imlib_create_color_range ());
	    }
	  else if (strcmp (argv[i], "-add") == 0)
	    {
	      Color c;
	      if ((++i) >= argc)
		{
		  fprintf (stderr, "Missing color\n");
		  continue;
		}
	      if (parse_color (argv[i], &c, alpha) == 0)
		{
		  fprintf (stderr, "Bad color (%s)\n", argv[i - 1]);
		  continue;
		}
	      imlib_context_set_color (c.r, c.g, c.b, c.a);
	      imlib_add_color_to_color_range (1);
	    }
	  else if (strcmp (argv[i], "-addd") == 0)
	    {
	      Color c;
	      int distance;
	      if ((++i) >= argc)
		{
		  fprintf (stderr, "Missing color\n");
		  continue;
		}
	      if ((++i) >= argc)
		{
		  fprintf (stderr, "Missing distance\n");
		  continue;
		}
	      if (parse_color (argv[i - 1], &c, alpha) == 0)
		{
		  fprintf (stderr, "Bad color (%s)\n", argv[i - 1]);
		  continue;
		}
	      if (sscanf (argv[i], "%i", &distance) == 0)
		{
		  fprintf (stderr, "Bad distance (%s)\n", argv[i]);
		  continue;
		}
	      imlib_context_set_color (c.r, c.g, c.b, c.a);
	      imlib_add_color_to_color_range (distance);
	    }
	  else if (strcmp (argv[i], "-gradient") == 0)
	    {
	      int angle;
	      if ((++i) >= argc)
		{
		  fprintf (stderr, "Missing angle\n");
		  continue;
		}
	      if (sscanf (argv[i], "%i", &angle) == 0)
		{
		  fprintf (stderr, "Bad angle (%s)\n", argv[i]);
		  continue;
		}
	      imlib_image_fill_color_range_rectangle (0, 0, width, height,
						      angle);
	    }
	  else if (strcmp (argv[i], "-fill") == 0)
	    {
	      if ((++i) >= argc)
		{
		  fprintf (stderr, "Missing image\n");
		  continue;
		}
	      if (load_image (Fill, argv[i], width, height, alpha, image) ==
		  0)
		{
		  fprintf (stderr, "Bad image (%s)\n", argv[i]);
		  continue;
		}
	    }
	  else if (strcmp (argv[i], "-full") == 0)
	    {
	      if ((++i) >= argc)
		{
		  fprintf (stderr, "Missing image\n");
		  continue;
		}
	      if (load_image (Full, argv[i], width, height, alpha, image) ==
		  0)
		{
		  fprintf (stderr, "Bad image (%s)\n", argv[i]);
		  continue;
		}
	    }
	  else if (strcmp (argv[i], "-extend") == 0)
	    {
	      if ((++i) >= argc)
		{
		  fprintf (stderr, "Missing image\n");
		  continue;
		}
	      if (load_image (Xtend, argv[i], width, height, alpha, image) ==
		  0)
		{
		  fprintf (stderr, "Bad image (%s)\n", argv[i]);
		  continue;
		}
	    }
	  else if (strcmp (argv[i], "-tile") == 0)
	    {
	      if ((++i) >= argc)
		{
		  fprintf (stderr, "Missing image\n");
		  continue;
		}
	      if (load_image (Tile, argv[i], width, height, alpha, image) ==
		  0)
		{
		  fprintf (stderr, "Bad image (%s)\n", argv[i]);
		  continue;
		}
	    }
	  else if (strcmp (argv[i], "-center") == 0)
	    {
	      if ((++i) >= argc)
		{
		  fprintf (stderr, "Missing image\n");
		  continue;
		}
	      if (load_image (Center, argv[i], width, height, alpha, image) ==
		  0)
		{
		  fprintf (stderr, "Bad image (%s)\n", argv[i]);
		  continue;
		}
	    }
	  else if (strcmp (argv[i], "-tint") == 0)
	    {
	      Color c;
	      DATA8 r[256], g[256], b[256], a[256];
	      int j;

	      if ((++i) >= argc)
		{
		  fprintf (stderr, "Missing color\n");
		  continue;
		}
	      if (parse_color (argv[i], &c, 255) == 0)
		{
		  fprintf (stderr, "Bad color\n");
		  continue;
		}

	      imlib_get_color_modifier_tables (r, g, b, a);

	      for (j = 0; j < 256; j++)
		{
		  r[j] = (DATA8) (((double) r[j] / 255.0) * (double) c.r);
		  g[j] = (DATA8) (((double) g[j] / 255.0) * (double) c.g);
		  b[j] = (DATA8) (((double) b[j] / 255.0) * (double) c.b);
		}

	      imlib_set_color_modifier_tables (r, g, b, a);
	    }
	  else if (strcmp (argv[i], "-blur") == 0)
	    {
	      int intval;
	      if ((++i) >= argc)
		{
		  fprintf (stderr, "Missing value\n");
		  continue;
		}
	      if (sscanf (argv[i], "%i", &intval) == 0)
		{
		  fprintf (stderr, "Bad value (%s)\n", argv[i]);
		  continue;
		}
	      imlib_image_blur (intval);
	    }
	  else if (strcmp (argv[i], "-sharpen") == 0)
	    {
	      int intval;
	      if ((++i) >= argc)
		{
		  fprintf (stderr, "Missing value\n");
		  continue;
		}
	      if (sscanf (argv[i], "%i", &intval) == 0)
		{
		  fprintf (stderr, "Bad value (%s)\n", argv[i]);
		  continue;
		}
	      imlib_image_sharpen (intval);
	    }
	  else if (strcmp (argv[i], "-contrast") == 0)
	    {
	      double dblval;
	      if ((++i) >= argc)
		{
		  fprintf (stderr, "Missing value\n");
		  continue;
		}
	      if (sscanf (argv[i], "%lf", &dblval) == 0)
		{
		  fprintf (stderr, "Bad value (%s)\n", argv[i]);
		  continue;
		}
	      imlib_modify_color_modifier_contrast (dblval);
	    }
	  else if (strcmp (argv[i], "-brightness") == 0)
	    {
	      double dblval;
	      if ((++i) >= argc)
		{
		  fprintf (stderr, "Missing value\n");
		  continue;
		}
	      if (sscanf (argv[i], "%lf", &dblval) == 0)
		{
		  fprintf (stderr, "Bad value (%s)\n", argv[i]);
		  continue;
		}
	      imlib_modify_color_modifier_brightness (dblval);
	    }
	  else if (strcmp (argv[i], "-gamma") == 0)
	    {
	      double dblval;
	      if ((++i) >= argc)
		{
		  fprintf (stderr, "Missing value\n");
		  continue;
		}
	      if (sscanf (argv[i], "%lf", &dblval) == 0)
		{
		  fprintf (stderr, "Bad value (%s)\n", argv[i]);
		  continue;
		}
	      imlib_modify_color_modifier_gamma (dblval);
	    }
	  else if (strcmp (argv[i], "-flipv") == 0)
	    {
	      imlib_image_flip_vertical ();
	    }
	  else if (strcmp (argv[i], "-fliph") == 0)
	    {
	      imlib_image_flip_horizontal ();
	    }
	  else if (strcmp (argv[i], "-flipd") == 0)
	    {
	      imlib_image_flip_diagonal ();
	    }
	  else if (strcmp (argv[i], "-write") == 0)
	    {
	      if ((++i) >= argc)
		{
		  fprintf (stderr, "Missing filename\n");
		  continue;
		}
	      imlib_save_image (argv[i]);
	    }
	  else
	    {
	      usage (argv[0]);
	      imlib_free_image ();
	      imlib_free_color_range ();
	      if (modifier != NULL)
		{
		  imlib_context_set_color_modifier (modifier);
		  imlib_free_color_modifier ();
		  modifier = NULL;
		}
	      XFreePixmap (display, pixmap);
	      exit (1);
	    }
	}

      if (modifier != NULL)
	{
	  imlib_context_set_color_modifier (modifier);
	  imlib_apply_color_modifier ();
	  imlib_free_color_modifier ();
	  modifier = NULL;
	}
      imlib_render_image_on_drawable (0, 0);
      imlib_free_image ();
      imlib_free_color_range ();

      if (setRootAtoms (pixmap) == 0)
	fprintf (stderr, "Couldn't create atoms...\n");

      XKillClient (display, AllTemporary);
      XSetCloseDownMode (display, RetainTemporary);

      XSetWindowBackgroundPixmap (display, RootWindow (display, screen),
				  pixmap);
      XClearWindow (display, RootWindow (display, screen));

      XFlush (display);
      XSync (display, False);

      imlib_context_pop ();
      imlib_context_free (context);
    }

  return 0;
}
