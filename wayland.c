#define _GNU_SOURCE
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <wayland-client.h>
#include "wlr-layer-shell-unstable-v1-client-protocol.h"
#include "xdg-output-unstable-v1-client-protocol.h"
#include "outputs.h"

typedef struct {
  struct wl_output *wl;
  Output o;
  int scale;
  struct wl_surface *surface;
  struct zwlr_layer_surface_v1 *layer;
  struct wl_buffer *buffer;
} WlOutput;

#define MAXOUTPUTS 32

static struct wl_display *wl;
static struct wl_compositor *compositor;
static struct wl_shm *shm;
static struct zwlr_layer_shell_v1 *layer_shell;
static struct zxdg_output_manager_v1 *xdg_output_manager;
static WlOutput outs[MAXOUTPUTS];
static int nouts;
static const unsigned int *pixels;
static int stride;

static void
output_geometry(void *data, struct wl_output *wl_output, int32_t x, int32_t y, int32_t pw, int32_t ph, int32_t subpixel, const char *make, const char *model, int32_t transform) {}

// wl_output's own position is 0,0 on wlroots
static void
xdg_position(void *data, struct zxdg_output_v1 *xdg_output, int32_t x, int32_t y)
{
  ((WlOutput *) data)->o.x = x;
  ((WlOutput *) data)->o.y = y;
}

static void
xdg_size(void *data, struct zxdg_output_v1 *xdg_output, int32_t w, int32_t h) {}

static void
xdg_done(void *data, struct zxdg_output_v1 *xdg_output) {}

static void
xdg_name(void *data, struct zxdg_output_v1 *xdg_output, const char *name) {}

static void
xdg_description(void *data, struct zxdg_output_v1 *xdg_output, const char *description) {}

static const struct zxdg_output_v1_listener xdg_output_listener = {
  .logical_position = xdg_position,
  .logical_size = xdg_size,
  .done = xdg_done,
  .name = xdg_name,
  .description = xdg_description,
};

static void
output_mode(void *data, struct wl_output *wl_output, uint32_t flags, int32_t w, int32_t h, int32_t refresh)
{
  if (flags & WL_OUTPUT_MODE_CURRENT) {
    ((WlOutput *) data)->o.width = w;
    ((WlOutput *) data)->o.height = h;
  }
}

static void
output_scale(void *data, struct wl_output *wl_output, int32_t factor)
{
  ((WlOutput *) data)->scale = factor;
}

static void
output_name(void *data, struct wl_output *wl_output, const char *name)
{
  ((WlOutput *) data)->o.name = strdup(name);
}

static void
output_done(void *data, struct wl_output *wl_output) {}

static void
output_description(void *data, struct wl_output *wl_output, const char *description) {}

static const struct wl_output_listener output_listener = {
  .geometry = output_geometry,
  .mode = output_mode,
  .done = output_done,
  .scale = output_scale,
  .name = output_name,
  .description = output_description,
};

static void
global(void *data, struct wl_registry *registry, uint32_t id, const char *interface, uint32_t version)
{
  if (!strcmp(interface, wl_compositor_interface.name)) {
    compositor = wl_registry_bind(registry, id, &wl_compositor_interface, 4);
  } else if (!strcmp(interface, wl_shm_interface.name)) {
    shm = wl_registry_bind(registry, id, &wl_shm_interface, 1);
  } else if (!strcmp(interface, zwlr_layer_shell_v1_interface.name)) {
    layer_shell = wl_registry_bind(registry, id, &zwlr_layer_shell_v1_interface, 1);
  } else if (!strcmp(interface, zxdg_output_manager_v1_interface.name)) {
    xdg_output_manager = wl_registry_bind(registry, id, &zxdg_output_manager_v1_interface, 2);
  } else if (!strcmp(interface, wl_output_interface.name) && version >= 4 && nouts < MAXOUTPUTS) {
    WlOutput *out = &outs[nouts++];
    out->scale = 1;
    out->wl = wl_registry_bind(registry, id, &wl_output_interface, 4);
    wl_output_add_listener(out->wl, &output_listener, out);
  }
}

static void
global_remove(void *data, struct wl_registry *registry, uint32_t id) {}

static const struct wl_registry_listener registry_listener = { .global = global, .global_remove = global_remove };

// outputs at physical size, positions moved to 0,0 and scaled so they can't overlap
Output *
wl_query_outputs(int *noutputs, int *width, int *height)
{
  int minx = 0, miny = 0, maxscale = 1;

  if (!(wl = wl_display_connect(NULL))) {
    fprintf(stderr, "Cannot open Wayland display!\n");
    exit(123);
  }
  wl_registry_add_listener(wl_display_get_registry(wl), &registry_listener, NULL);
  wl_display_roundtrip(wl);

  if (!compositor || !shm || !layer_shell || !xdg_output_manager) {
    fprintf(stderr, "No wlr-layer-shell, this compositor isn't supported yet (GNOME? KDE Plasma?)\n");
    exit(1);
  }
  for (int i = 0; i < nouts; i++)
    zxdg_output_v1_add_listener(zxdg_output_manager_v1_get_xdg_output(xdg_output_manager, outs[i].wl), &xdg_output_listener, &outs[i]);
  wl_display_roundtrip(wl);

  Output *outputs = calloc(nouts ? nouts : 1, sizeof(Output));
  for (int i = 0; i < nouts; i++) {
    minx = (i == 0 || outs[i].o.x < minx) ? outs[i].o.x : minx;
    miny = (i == 0 || outs[i].o.y < miny) ? outs[i].o.y : miny;
    maxscale = outs[i].scale > maxscale ? outs[i].scale : maxscale;
  }
  *width = *height = 1;
  for (int i = 0; i < nouts; i++) {
    Output *o = &outs[i].o;
    o->x = (o->x - minx) * maxscale;
    o->y = (o->y - miny) * maxscale;
    *width = o->x + o->width > *width ? o->x + o->width : *width;
    *height = o->y + o->height > *height ? o->y + o->height : *height;
    outputs[i] = *o;
  }

  *noutputs = nouts;
  return outputs;
}

static struct wl_buffer *
create_buffer(Output *o)
{
  int size = o->width * o->height * 4;
  int fd = memfd_create("hsetroot", MFD_CLOEXEC);

  if (fd < 0 || ftruncate(fd, size) < 0) {
    perror("hsetroot: memfd");
    exit(1);
  }
  unsigned int *data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  for (int y = 0; y < o->height; y++)
    memcpy(data + y * o->width, pixels + (o->y + y) * stride + o->x, o->width * 4);
  munmap(data, size);

  struct wl_shm_pool *pool = wl_shm_create_pool(shm, fd, size);
  struct wl_buffer *buffer = wl_shm_pool_create_buffer(pool, 0, o->width, o->height, o->width * 4, WL_SHM_FORMAT_XRGB8888);
  wl_shm_pool_destroy(pool);
  close(fd);
  return buffer;
}

static void
layer_configure(void *data, struct zwlr_layer_surface_v1 *layer, uint32_t serial, uint32_t w, uint32_t h)
{
  WlOutput *out = data;

  zwlr_layer_surface_v1_ack_configure(layer, serial);
  if (!out->buffer)
    out->buffer = create_buffer(&out->o);
  // the buffer must be a multiple of the scale
  if (out->o.width % out->scale == 0 && out->o.height % out->scale == 0)
    wl_surface_set_buffer_scale(out->surface, out->scale);
  wl_surface_attach(out->surface, out->buffer, 0, 0);
  wl_surface_damage_buffer(out->surface, 0, 0, INT32_MAX, INT32_MAX);
  wl_surface_commit(out->surface);
}

static void
layer_closed(void *data, struct zwlr_layer_surface_v1 *layer)
{
  WlOutput *out = data;

  zwlr_layer_surface_v1_destroy(out->layer);
  wl_surface_destroy(out->surface);
  out->layer = NULL;
}

static const struct zwlr_layer_surface_v1_listener layer_listener = { .configure = layer_configure, .closed = layer_closed };

// one background surface per output showing its part of the image, then stay running
void
wl_show(const unsigned int *data, int width)
{
  pixels = data;
  stride = width;

  for (int i = 0; i < nouts; i++) {
    WlOutput *out = &outs[i];
    struct wl_region *empty = wl_compositor_create_region(compositor);

    out->surface = wl_compositor_create_surface(compositor);
    wl_surface_set_input_region(out->surface, empty);
    wl_region_destroy(empty);
    out->layer = zwlr_layer_shell_v1_get_layer_surface(layer_shell, out->surface, out->wl, ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND, "wallpaper");
    zwlr_layer_surface_v1_set_size(out->layer, 0, 0);
    zwlr_layer_surface_v1_set_anchor(out->layer, ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP | ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM | ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT | ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT);
    zwlr_layer_surface_v1_set_exclusive_zone(out->layer, -1);
    zwlr_layer_surface_v1_add_listener(out->layer, &layer_listener, out);
    wl_surface_commit(out->surface);
  }

  while (wl_display_dispatch(wl) != -1)
    ;
  exit(0);
}
