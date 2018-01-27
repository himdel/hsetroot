#ifndef __OPTIONS_H__
#define __OPTIONS_H__

#include <Imlib2.h>

typedef enum { Full, Fill, Center, Tile, Xtend, Cover } ImageMode;

typedef struct {
  OutputInfo *outputs;
  int output_count;
  ImageMode image_mode;
  const char *image_mode_arg;
  int alpha;
} Options;

void options_default(Options*);

#endif	// __OPTIONS_H__
