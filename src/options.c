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
}
