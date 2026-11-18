#include <X11/Xlib.h>

typedef struct {
  char *name;
  int x, y, width, height;
} Output;

Output *query_outputs(Display *display, int screen, int *noutputs);

#ifdef WAYLAND
Output *wl_query_outputs(int *noutputs, int *width, int *height);
void wl_show(const unsigned int *data, int width);
#endif
