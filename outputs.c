#include <X11/Xlib.h>
#include <X11/extensions/Xinerama.h>
#include <X11/extensions/Xrandr.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "outputs.h"

// RandR monitors, or Xinerama heads, or just the whole screen
Output *
query_outputs(Display *display, int screen, int *noutputs)
{
  int event, error, major = 0, minor = 0, n = 0;
  Output *outputs;

  if (XRRQueryExtension(display, &event, &error) && XRRQueryVersion(display, &major, &minor) && (major > 1 || minor >= 5)) {
    XRRMonitorInfo *m = XRRGetMonitors(display, RootWindow(display, screen), True, &n);
    outputs = calloc(n > 0 ? n : 1, sizeof(Output));
    for (int i = 0; i < n; i++)
      outputs[i] = (Output) { XGetAtomName(display, m[i].name), m[i].x, m[i].y, m[i].width, m[i].height };
    XRRFreeMonitors(m);
  } else if (XineramaIsActive(display)) {
    XineramaScreenInfo *x = XineramaQueryScreens(display, &n);
    outputs = calloc(n > 0 ? n : 1, sizeof(Output));
    for (int i = 0; i < n; i++) {
      char name[16];
      snprintf(name, sizeof(name), "%d", i);
      outputs[i] = (Output) { strdup(name), x[i].x_org, x[i].y_org, x[i].width, x[i].height };
    }
    XFree(x);
  } else {
    n = 1;
    outputs = calloc(1, sizeof(Output));
    outputs[0] = (Output) { "screen", 0, 0, DisplayWidth(display, screen), DisplayHeight(display, screen) };
  }

  *noutputs = n > 0 ? n : 0;
  return outputs;
}
