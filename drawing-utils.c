#include "drawing-utils.h"

void blit_image(cairo_surface_t *src, cairo_t *dest, int dstx, int dsty)
{
	size_t sh = cairo_image_surface_get_height(src);
	size_t sw = cairo_image_surface_get_width(src);

	cairo_save(dest);
	cairo_set_source_surface(dest, src, dstx, dsty);
	cairo_pattern_set_extend(cairo_get_source(dest), CAIRO_EXTEND_REPEAT);
	cairo_rectangle(dest, dstx, dsty, sw, sh);
	cairo_clip(dest);
	cairo_paint(dest);
	cairo_restore(dest);
}

void pattern_image(cairo_surface_t *src, cairo_t *dest, 
		int dstx, int dsty, int w)
{
	size_t sh = cairo_image_surface_get_height(src);

	cairo_save(dest);
	cairo_set_source_surface(dest, src, dstx, dsty);
	cairo_pattern_set_extend(cairo_get_source(dest), CAIRO_EXTEND_REPEAT);
	cairo_rectangle(dest, dstx, dsty, w, sh);
	cairo_clip(dest);
	cairo_paint(dest);
	cairo_restore(dest);
}

void draw_text(cairo_t *cr, PangoLayout *dest, PangoFontDescription *font, 
		const char *text, unsigned char *color, int x, int y, 
		int w, int h, int align)
{
	PangoRectangle r;
	int offsetx = 0, offsety = 0;

	cairo_save(cr);
	cairo_set_source_rgb(cr,
			(double)color[0] / 255.0,
			(double)color[1] / 255.0,
			(double)color[2] / 255.0);
	pango_layout_set_font_description(dest, font);
	pango_layout_set_text(dest, text, -1);
	
	pango_layout_get_pixel_extents(dest, 0, &r);

	offsety = (h - r.height) / 2;
	switch (align) {
	default:
	case TEXT_ALIGN_CENTER:
		offsetx = (w - r.width) / 2;
		break;
	case TEXT_ALIGN_LEFT:
		offsetx = 0;
		break;
	case TEXT_ALIGN_RIGHT:
		offsetx = w - r.width;
		break;
	}
	
	cairo_translate(cr, x + offsetx, y + offsety);
	cairo_rectangle(cr, 0, 0, w, h);
	cairo_clip(cr);
	pango_cairo_update_layout(cr, dest);
	pango_cairo_show_layout(cr, dest);
	cairo_restore(cr);
}
