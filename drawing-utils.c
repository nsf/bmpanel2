#include "drawing-utils.h"

void blit_image_part(struct image_part *src, cairo_t *dest, int dstx, int dsty)
{
	cairo_set_source_surface(dest, src->img->surface, src->x, src->y);
	cairo_pattern_set_extend(cairo_get_source(dest), CAIRO_EXTEND_REPEAT);
	cairo_rectangle(dest, dstx, dsty, src->width, src->height);
	cairo_clip(dest);
	cairo_paint(dest);

	cairo_reset_clip(dest);
}

void pattern_image_part(struct image_part *src, cairo_t *dest, 
		int dstx, int dsty, int w)
{
	cairo_set_source_surface(dest, src->img->surface, src->x, src->y);
	cairo_pattern_set_extend(cairo_get_source(dest), CAIRO_EXTEND_REPEAT);
	cairo_rectangle(dest, dstx, dsty, w, src->height);
	cairo_clip(dest);
	cairo_paint(dest);

	cairo_reset_clip(dest);
}
