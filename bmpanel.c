#include <stdio.h>
#include "util.h"
#include "xutil.h"
#include "gui.h"
#include "theme-parser.h"

void print_widgets(struct panel *p)
{
	size_t i;
	for (i = 0; i < p->widgets_n; ++i) {
		struct widget *w = &p->widgets[i];
		printf("widget: %s (%d) [%d %d %d %d]\n", w->interface->theme_name, 
				w->interface->size_type, w->x, w->y, w->width,
				w->height);
	}
}

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
