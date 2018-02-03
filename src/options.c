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

#include <stdlib.h>
#include <unistd.h>

#include <string.h>
#include <stdbool.h>
#include "outputs.h"
#include "options.h"

void
options_default(Options* options)
{
  memset(options, 0, sizeof(Options));
  options->image_mode = DEFAULT_IMAGE_MODE;
  options->image_mode_arg = DEFAULT_IMAGE_ARG;
  options->effects_mode = DEFAULT_EFFECTS_MODE;
  options->alpha_level = DEFAULT_ALPHA_LEVEL;
  options->set_solid = false;
  options->set_alpha = false;
  options->set_blur = false;
  options->set_sharpen = false;
}
