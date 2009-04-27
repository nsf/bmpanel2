#include <stdio.h>
#include "gui.h"
#include "config-parser.h"
#include "xdg.h"
#include "settings.h"

int main(int argc, char **argv)
{
	struct config_format_tree tree;
	struct panel p;

	load_settings();
	const char *theme_name = find_config_format_entry_value(&g_settings.root,
								"theme");
	if (!theme_name)
		theme_name = "native";

	if (0 != load_config_format_tree(&tree, "theme"))
		XDIE("Failed to load theme file");
	
	register_taskbar();
	register_clock();

	if (0 != create_panel(&p, &tree))
		XDIE("Failed to create a panel");

	panel_main_loop(&p);

	destroy_panel(&p);
	free_config_format_tree(&tree);
	clean_image_cache();

	free_settings();

	xmemstat(0, 0, false);
	return EXIT_SUCCESS;
}
