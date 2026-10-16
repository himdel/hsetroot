#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include "outputs.h"

void
print_text(Output *o) {
	if (! o) {
		printf("\n");
		return;
	}

	printf("name: %s\n", o->name);
	printf("\tx:\t%d\n", o->x);
	printf("\ty:\t%d\n", o->y);
	printf("\twidth:\t%d\n", o->width);
	printf("\theight:\t%d\n", o->height);
}

void
print_csv(Output *o) {
	if (! o)
		return;

	printf("%s,%d,%d,%d,%d\n", o->name, o->x, o->y, o->width, o->height);
}

void
print_json(Output *o) {
	if (! o) {
		printf(",\n");
		return;
	}

	printf("  {\n");
	printf("    \"name\": \"%s\",\n", o->name);
	printf("    \"x\": %d,\n", o->x);
	printf("    \"y\": %d,\n", o->y);
	printf("    \"width\": %d,\n", o->width);
	printf("    \"height\": %d\n", o->height);
	printf("  }");
}

enum Format { TEXT, CSV, JSON };

int
main(int argc, char **argv)
{
	Display *display = XOpenDisplay(NULL);
	if (! display) {
		fprintf(stderr, "Cannot open X display!\n");
		exit(123);
	}

	enum Format format = TEXT;
	void (*print)(Output*) = print_text;

	if ((argc > 1) && !strcmp(argv[1], "--csv")) {
		print = print_csv;
		format = CSV;
	}
	if ((argc > 1) && !strcmp(argv[1], "--json")) {
		print = print_json;
		format = JSON;
	}

	if (format == CSV)
		printf("name,x,y,width,height\n");
	if (format == JSON)
		printf("[\n");

	int noutputs = 0;
	Output *outputs = query_outputs(display, DefaultScreen(display), &noutputs);
	for (int i = 0; i < noutputs; i++) {
		if (i)
			print(NULL);
		print(outputs + i);
	}
	free(outputs);

	if (format == JSON)
		printf("\n]\n");

	XCloseDisplay(display);

	return 0;
}
