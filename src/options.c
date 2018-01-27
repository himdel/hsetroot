#include <stdlib.h>
#include <unistd.h>

#include <string.h>
#include "outputs.h"
#include "options.h"

void
options_default(Options* options)
{
  memset(options, 0, sizeof(Options));
  options->image_mode = Fill;
  options->image_mode_arg = "#000000";
  options->alpha = 255;
}
