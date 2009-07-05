#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "gui.h"
#include "config-parser.h"
#include "xdg.h"
#include "settings.h"
#include "widget-utils.h"
#include "builtin-widgets.h"

static int try_load_theme(struct config_format_tree *tree, const char *name)
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
	
static struct config_format_tree tree;
static struct panel p;

static void sigint_handler(int xxx)
{
	XWARNING("sigint signal received, stopping main loop...");
	g_main_loop_quit(p.loop);
}

static void sigterm_handler(int xxx)
{
	XWARNING("sigterm signal received, stopping main loop...");
	g_main_loop_quit(p.loop);
}

static void mysignal(int sig, void (*handler)(int))
{
	struct sigaction sa;
	sa.sa_handler = handler;
	sa.sa_flags = 0;
	sigaction(sig, &sa, 0);
}

int main(int argc, char **argv)
{
	int theme_load_status = -1;

	load_settings();
	const char *theme_name = find_config_format_entry_value(&g_settings.root,
								"theme");
	if ( argc > 1 ) {
		theme_name = argv[1];
	}
	
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
	register_widget_interface(&launchbar_interface);

	init_panel(&p, &tree);

	mysignal(SIGINT, sigint_handler);
	mysignal(SIGTERM, sigterm_handler);

	panel_main_loop(&p);
	
	free_panel(&p);
	free_config_format_tree(&tree);
	clean_image_cache();
	clean_static_buf();
	free_settings();
	xmemstat(0, 0, 1);
	return EXIT_SUCCESS;
}
