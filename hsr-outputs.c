#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xinerama.h>

void
print_text(XineramaScreenInfo *si) {
	if (! si) {
		printf("\n");
		return;
	}

	printf("screen_number: %d\n", si->screen_number);
	printf("\tx_org:\t%d\n", si->x_org);
	printf("\ty_org:\t%d\n", si->y_org);
	printf("\twidth:\t%d\n", si->width);
	printf("\theight:\t%d\n", si->height);
}

void
print_csv(XineramaScreenInfo *si) {
	if (! si)
		return;

	printf("%d,%d,%d,%d,%d\n", si->screen_number, si->x_org, si->y_org, si->width, si->height);
}

void
print_json(XineramaScreenInfo *si) {
	if (! si) {
		printf(",\n");
		return;
	}

	printf("  {\n");
	printf("    \"screen\": %d,\n", si->screen_number);
	printf("    \"x\": %d,\n", si->x_org);
	printf("    \"y\": %d,\n", si->y_org);
	printf("    \"width\": %d,\n", si->width);
	printf("    \"height\": %d\n", si->height);
	printf("  }");
}

enum Format { TEXT, CSV, JSON };

int
main(int argc, char **argv)
{
	Display *display = XOpenDisplay(NULL);

	enum Format format = TEXT;
	void (*print)(XineramaScreenInfo*) = print_text;

	if ((argc > 1) && !strcmp(argv[1], "--csv")) {
		print = print_csv;
		format = CSV;
	}
	if ((argc > 1) && !strcmp(argv[1], "--json")) {
		print = print_json;
		format = JSON;
	}

	if (format == CSV)
		printf("screen_number,x_org,y_org,width,height\n");
	if (format == JSON)
		printf("[\n");

	int nscreens = 0;
	XineramaScreenInfo *xisi = XineramaQueryScreens(display, &nscreens);
	for (int i = 0; i < nscreens; i++) {
		if (i)
			print(NULL);
		print(xisi + i);
	}
	XFree(xisi);

	if (format == JSON)
		printf("\n]\n");

	XCloseDisplay(display);

	return 0;
}
