#include <stdio.h>
#include "gui.h"
#include "theme-parser.h"

int main(int argc, char **argv)
{
	struct theme_format_tree tree;
	struct panel p;

	if (0 != load_theme_format_tree(&tree, "."))
		xdie("Failed to load theme file");

	printf("theme from dir: '%s' loaded\n", tree.dir);
	
	register_taskbar();
	register_clock();

	if (0 != create_panel(&p, &tree))
		xdie("Failed to create panel");

	panel_main_loop(&p);

	destroy_panel(&p);
	free_theme_format_tree(&tree);
	clean_image_cache();

	xmemstat(0, 0, false);
	return EXIT_SUCCESS;
}
