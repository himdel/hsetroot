typedef struct {
  char *name;
  int x, y, width, height;
} Output;

Output *query_outputs(Display *display, int screen, int *noutputs);
