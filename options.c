#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

typedef struct {
	const char *long_name;
	char short_name;
	int args;
	const *char args_help;
	const *char opt_help;
} Option;

#define help_section(title) {NULL, 0, 0, NULL, title}
#define help_empty_line {NULL, 0, 0, NULL, ""}

static Option options[] = {
	help_section(									"Generic Options:"),
	{"root",		0,		0,	NULL,					"Treat multiple displays as one big screen (ignore xrandr outputs)"},
	{"screens",		0,		1,	"<int>",				"Set a screenmask to use"},
	help_empty_line,
	help_section(									"Gradients:"),
	{"add",			0,		1,	"<color>",				"Add color to range using distance 1"},
	{"addd",		0,		2,	"<color> <distance>",	"Add color to range using custom distance"},
	{"gradient",	0,		1,	"<angle>",				"Render gradient using specified angle"},
	{"clear",		0,		0,	NULL,					"Clear the color range"},
	help_empty_line,
	help_section(									"Solid:"),
	{"solid",		0,		1,	"<color>",				"Render a solid using the specified color"},
	help_empty_line,
	help_section(									"Image files:"),
	{"center",		0,		1,	"<image>",				"Render an image centered on screen"},
	{"cover",		0,		1,	"<image>",				"Render an image centered on screen scaled to fill the screen fully"},
	{"tile",		0,		1,	"<image>",				"Render an image tiled"},
	{"full",		0,		1,	"<image>",				"Render an image maximum aspect"},
	{"extend",		0,		1,	"<image>",				"Render an image max aspect and fill borders"},
	{"fill",		0,		1,	"<image>",				"Render an image stretched"},
	help_empty_line,
	help_section(									"Manipulations:"),
	{"tint",		0,		1,	"<color>",				"Tint the current image"},
	{"blur",		0,		1,	"<radius>",				"Blur the current image"},
	{"sharpen",		0,		1,	"<radius>",				"Sharpen the current image"},
	{"contrast",	0,		1,	"<amount>",				"Adjust contrast of current image"},
	{"brightness",	0,		1,	"<amount>",				"Adjust brightness of current image"},
	{"gamma",		0,		1,	"<amount>",				"Adjust gamma level of current image"},
	{"flipv",		0,		0,	NULL,					"Flip the current image vertically"},
	{"fliph",		0,		0,	NULL,					"Flip the current image horizontally"},
	{"flipd",		0,		0,	NULL,					"Flip the current image diagonally"},
	help_empty_line,
	help_section(									"Misc:"),
	{"alpha",		0,		1,	"<amount>",				"Adjust alpha level for colors and images"},
	{"write",		0,		1,	"<filename>",			"Write current image to file"},
	{"help",		'h',	0,	NULL,					"This help"},
	{"version",		'V',	0,	NULL,					"Version"},
};

#undef help_empty_line
#undef help_section

int
main(int argc, char **argv)
{
	while (1) {
		int option_index = 0;
		static struct option long_options[] = {
			{"add",     required_argument, 0, 0},
			{"append",  no_argument,       0, 0},
			{"delete",  required_argument, 0, 0},
			{"verbose", no_argument,       0, 0},
			{"create",  required_argument, 0, 'c'},
			{"file",    required_argument, 0, 0},
			{0,         0,                 0, 0},
		};

		int c = getopt_long(argc, argv, "abc:d:012", long_options, &option_index);
		if (c == -1)
			break;

		switch (c) {
		case 0:
			printf("option %s", long_options[option_index].name);
			if (optarg)
				printf(" with arg %s", optarg);
			printf("\n");
			break;

		case '0':
		case '1':
		case '2':
			printf("option %c\n", c);
			break;

		case 'a':
			printf("option a\n");
			break;

		case 'b':
			printf("option b\n");
			break;

		case 'c':
			// 2 arguments
			printf("option c with value '%s'; next %s\n", optarg, argv[optind]);
			optind++;
			break;

		case 'd':
			printf("option d with value '%s'\n", optarg);
			break;

		case '?':
			break;

		default:
			printf("?? getopt returned character code 0%o ??\n", c);
		}
	}

	if (optind < argc) {
		printf("non-option ARGV-elements: ");
		while (optind < argc)
			printf("%s ", argv[optind++]);
		printf("\n");
	}

	return 0;
}
