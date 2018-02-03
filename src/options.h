//////////////////////////////////////////////////////////////////////
/*

  Copyright (C) 2018 Pete Beardmore

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program. If not, see <http://www.gnu.org/licenses/>.

  http://www.gnu.org/licenses/gpl-2.0.txt

*/
//////////////////////////////////////////////////////////////////////

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
