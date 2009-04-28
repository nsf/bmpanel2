#include <stdio.h>
#include "gui.h"
#include "config-parser.h"
#include "xdg.h"
#include "settings.h"

void load_theme(struct config_format_tree *tree, const char *name)
{
	char buf[4096];
	size_t data_dirs_len;
	char **data_dirs = get_XDG_DATA_DIRS(&data_dirs_len);
	int found = 0;

	size_t i;
	for (i = 0; i < data_dirs_len; ++i) {
		snprintf(buf, sizeof(buf), "%s/bmpanel2/themes/%s/theme",
			 data_dirs[i], name);
		buf[sizeof(buf)-1] = '\0';
		if (is_file_exists(buf)) {
			found = 1;
			break;
		}
	}
	free_XDG(data_dirs);

	if (found) {
		if (0 != load_config_format_tree(tree, buf))
			XDIE("Failed to load theme: %s", name);
	} else {
		/* try to load it in-place */
		snprintf(buf, sizeof(buf), "%s/theme", name);
		if (0 != load_config_format_tree(tree, buf))
			XDIE("Failed to load theme: %s", name);
	}
}

int main(int argc, char **argv)
{
	struct config_format_tree tree;
	struct panel p;

	load_settings();
	const char *theme_name = find_config_format_entry_value(&g_settings.root,
								"theme");
	if (!theme_name)
		theme_name = "native";

	load_theme(&tree, theme_name);
	
	register_desktop_switcher();
	register_taskbar();
	register_clock();
	register_decor();

	if (0 != create_panel(&p, &tree))
		XDIE("Failed to create a panel");

	panel_main_loop(&p);

	destroy_panel(&p);
	free_config_format_tree(&tree);
	clean_image_cache();

	free_settings();

	xmemstat(0, 0, 1);
	return EXIT_SUCCESS;
}
