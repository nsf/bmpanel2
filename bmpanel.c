#include <stdio.h>
#include "util.h"
#include "xutil.h"
#include "gui.h"
#include "theme-parser.h"
#include "drawing-utils.h"

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

	if (0 != create_panel(&p, &tree))
		xdie("Failed to create panel");

	printf("panel created: %d %d %d %d\n", p.x, p.y, p.width, p.height);
	print_widgets(&p);

	cairo_save(p.cr);
	cairo_identity_matrix(p.cr);
	
	PangoFontDescription *fontd = pango_font_description_from_string("Verdana 8");
	unsigned char white[] = {255, 255, 255};
	unsigned char black[] = {0, 0, 0};
	draw_text(p.cr, p.layout, fontd, "This is a test", white, 0, 1, 100, p.height,
			TEXT_ALIGN_CENTER);
	
	pango_font_description_free(fontd);
	cairo_restore(p.cr);
	
	expose_panel(&p, 0, 0, p.width, p.height);
	
	panel_main_loop(&p);

	destroy_panel(&p);
	free_theme_format_tree(&tree);
	clean_image_cache();

	xmemstat(0, 0, false);
	return EXIT_SUCCESS;
}
