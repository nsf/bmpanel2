#include <stdio.h>
#include "gui.h"
#include "theme-parser.h"
#include "xdg.h"

int main(int argc, char **argv)
{
	struct theme_format_tree tree;
	struct panel p;

	size_t dirs_n, i;
	char **dirs = get_XDG_CONFIG_DIRS(&dirs_n);
	printf("---- config dirs ----\n");
	for (i = 0; i < dirs_n; ++i)
		printf("%s\n", dirs[i]);
	free_XDG(dirs);
	
	dirs = get_XDG_DATA_DIRS(&dirs_n);
	printf("---- data dirs ----\n");
	for (i = 0; i < dirs_n; ++i)
		printf("%s\n", dirs[i]);
	free_XDG(dirs);

	if (0 != load_theme_format_tree(&tree, ".", "theme"))
		XDIE("Failed to load theme file");
	
	register_taskbar();
	register_clock();

	if (0 != create_panel(&p, &tree))
		XDIE("Failed to create a panel");

	panel_main_loop(&p);

	destroy_panel(&p);
	free_theme_format_tree(&tree);
	clean_image_cache();

	xmemstat(0, 0, false);
	return EXIT_SUCCESS;
}
