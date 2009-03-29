#include <stdio.h>
#include "util.h"
#include "xutil.h"
#include "gui.h"
#include "theme-parser.h"

void print_image(struct image *img) 
{
	printf("image: %s (refs: %d, ptr: %p)\n", img->filename, img->ref_count,
			img->surface);
}

int main(int argc, char **argv)
{
	struct theme_format_tree tree;
	struct panel p;

	if (0 != theme_format_load_tree(&tree, "."))
		xdie("Failed to load theme file");

	printf("theme from dir: '%s' loaded\n", tree.dir);

	if (0 != panel_create(&p, &tree))
		xdie("Failed to create panel");

	sleep(3);

	panel_destroy(&p);
	theme_format_free_tree(&tree);

	xmemstat(0, 0, false);
	return EXIT_SUCCESS;
}
