#include <stdio.h>
#include "util.h"
#include "xutil.h"
#include "gui.h"
#include "theme-parser.h"

struct memory_source msrc_main = MEMSRC("Main", 0, 0, 0);

struct memory_source *msrc_list[] = {
	&msrc_main,
	&msrc_theme
};

int main(int argc, char **argv)
{
	struct x_connection xc;
	struct theme_format_tree tree;
	struct panel_theme pt;

	if (0 != theme_format_load_tree(&tree, "."))
		xdie("Failed to load theme file");

	printf("theme from dir: '%s' loaded\n", tree.dir);

	if (0 != panel_theme_load(&pt, &tree))
		xdie("Failed to parse panel theme");

	printf("position: %d\n", pt.position);

	theme_format_free_tree(&tree);

	if (0 != x_connect(&xc, 0))
		xdie("Failed to connect to X server");

	printf("%d %d %d %d\n", xc.workarea_x, xc.workarea_y, 
			xc.workarea_width, xc.workarea_height);

	xmemstat(msrc_list, 2, false);
	return 0;
}
