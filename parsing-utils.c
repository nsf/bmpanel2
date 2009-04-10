#include <string.h>
#include <stdio.h>
#include <alloca.h>
#include "parsing-utils.h"

static int parse_image_dimensions(int *x, int *y, int *w, int *h,
		struct theme_format_entry *e)
{
	const char *v = find_theme_format_entry_value(e, "xywh");
	if (v) {
		if (4 == sscanf(v, "%d:%d:%d:%d", x, y, w, h))
			return 0;
	}
	return -1;
}

cairo_surface_t *parse_image_part(struct theme_format_entry *e,
		struct theme_format_tree *tree)
{
	cairo_surface_t *img;
	int x,y,w,h;
	x = y = w = h = -1;
	char *file;
	size_t filestrlen = strlen(tree->dir) + 1 + strlen(e->value) + 1;

	/* compute path */
	if (filestrlen > MAX_ALLOCA)
		file = xmalloc(filestrlen);
	else
		file = alloca(filestrlen);
	sprintf(file, "%s/%s", tree->dir, e->value);

	/* load file */
	if (parse_image_dimensions(&x,&y,&w,&h,e) == 0)
		img = get_image_part(file,x,y,w,h);
	else
		img = get_image(file);
	
	if (!img)
		xwarning("Failed to get image: %s", file);

	/* free path */
	if (filestrlen > MAX_ALLOCA)
		xfree(file);

	return img;
}

cairo_surface_t *parse_image_part_named(const char *name, struct theme_format_entry *e,
		struct theme_format_tree *tree)
{
	struct theme_format_entry *ee = find_theme_format_entry(e, name);
	if (!ee)
		return 0;
	return parse_image_part(ee, tree);
}

