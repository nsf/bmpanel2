#include <stdio.h>
#include "util.h"
#include "xutil.h"
#include "gui.h"
#include "theme-parser.h"

struct memory_source msrc_main = MEMSRC("Main", 0, 0, 0);

struct memory_source *msrc_list[] = {
	&msrc_main,
	&msrc_theme,
	&msrc_panel
};

int main(int argc, char **argv)
{
	struct x_connection *xc;
	struct theme_format_tree tree;
	struct panel p;

	if (0 != theme_format_load_tree(&tree, "."))
		xdie("Failed to load theme file");

	printf("theme from dir: '%s' loaded\n", tree.dir);

	if (0 != panel_create(&p, &tree))
		xdie("Failed to create panel");

	sleep(3);

	xc = &p.conn;
	printf("%d %d %d %d\n", xc->workarea_x, xc->workarea_y, 
			xc->workarea_width, xc->workarea_height);

	panel_destroy(&p);
	theme_format_free_tree(&tree);

	xmemstat(msrc_list, 3, false);
	return 0;
}
