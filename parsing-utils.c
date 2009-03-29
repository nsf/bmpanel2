#include <string.h>
#include <stdio.h>
#include <alloca.h>
#include "gui.h"

int parse_image_part(struct image_part *imgp, struct theme_format_entry *e,
		struct theme_format_tree *tree)
{
	char *file;
	size_t filestrlen = strlen(tree->dir) + 1 + strlen(e->value) + 1;
	if (filestrlen > MAX_ALLOCA)
		file = xmalloc(filestrlen);
	else
		file = alloca(filestrlen);
	sprintf(file, "%s/%s", tree->dir, e->value);

	imgp->img = get_image(file);
	if (filestrlen > MAX_ALLOCA)
		xfree(file);

	if (!imgp->img)
		return xerror("Error parsing image part, image is missing");

	acquire_image(imgp->img);

	imgp->x = 0;
	imgp->y = 0;
	imgp->width = cairo_image_surface_get_width(imgp->img->surface);
	imgp->height = cairo_image_surface_get_height(imgp->img->surface);

	const char *v = theme_format_find_entry_value(e, "xywh");
	if (v) {
		sscanf(v, "%d:%d:%d:%d", &imgp->x, &imgp->y, 
				&imgp->width, &imgp->height);
	} else {
		v = theme_format_find_entry_value(e, "x");
		if (v)
			sscanf(v, "%d", &imgp->x);

		v = theme_format_find_entry_value(e, "y");
		if (v)
			sscanf(v, "%d", &imgp->y);
		
		v = theme_format_find_entry_value(e, "width");
		if (v)
			sscanf(v, "%d", &imgp->width);
		
		v = theme_format_find_entry_value(e, "height");
		if (v)
			sscanf(v, "%d", &imgp->height);
	}

	return 0;
}
