#include <stdio.h>
#include "gui.h"
#include "config-parser.h"
#include "xdg.h"
#include "settings.h"
#include "builtin-widgets.h"

struct memory_source msrc_titles = MEMSRC(
	"Titles",
	MEMSRC_DEFAULT_MALLOC,
	MEMSRC_DEFAULT_FREE,
	MEMSRC_NO_FLAGS
);

struct memory_source *msrcs[] = {
	&msrc_titles
};

int try_load_theme(struct config_format_tree *tree, const char *name)
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
			return -1;
	} else {
		/* try to load it in-place */
		snprintf(buf, sizeof(buf), "%s/theme", name);
		if (0 != load_config_format_tree(tree, buf))
			return -1;
	}

	return 0;
}

int main(int argc, char **argv)
{
	struct config_format_tree tree;
	struct panel p;
	int theme_load_status = -1;

	load_settings();
	const char *theme_name = find_config_format_entry_value(&g_settings.root,
								"theme");
	if (theme_name)
		theme_load_status = try_load_theme(&tree, theme_name);

	if (theme_load_status < 0) {
		if (theme_name)
			XWARNING("Failed to load theme: %s, "
				 "trying default \"native\"", theme_name);
		theme_load_status = try_load_theme(&tree, "native");
	}

	if (theme_load_status < 0)
		XDIE("Failed to load theme: native");
	
	register_widget_interface(&desktops_interface);
	register_widget_interface(&taskbar_interface);
	register_widget_interface(&clock_interface);
	register_widget_interface(&decor_interface);
	register_widget_interface(&systray_interface);

	create_panel(&p, &tree);
	panel_main_loop(&p);

	destroy_panel(&p);
	free_config_format_tree(&tree);
	clean_image_cache();

	free_settings();

	xmemstat(msrcs, 1, 1);
	return EXIT_SUCCESS;
}
