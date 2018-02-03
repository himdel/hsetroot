#ifndef __OPTIONS_H__
#define __OPTIONS_H__

#include <Imlib2.h>

typedef enum { Unset, Full, Fill, Center, Tile, Xtend, Cover } ImageMode;
typedef enum { Root, Output } EffectsMode;
#define DEFAULT_IMAGE_MODE 0;
#define DEFAULT_EFFECTS_MODE 1;
#define DEFAULT_IMAGE_ARG "";
#define DEFAULT_SOLID_COLOUR "#000000";
#define DEFAULT_ALPHA_LEVEL 255

typedef struct {
  OutputInfo *outputs;
  int output_count;
  ImageMode image_mode;
  EffectsMode effects_mode;
  const char *image_mode_arg;
  char *solid_colour;
  int alpha_level;
  int blur_level;
  int sharpen_level;
  // option switches
  bool set_solid;
  bool set_alpha;
  bool set_blur;
  bool set_sharpen;
} Options;

void options_default(Options*);

#endif	// __OPTIONS_H__
