#include <string.h>
#include <stdio.h>
#include <alloca.h>
#include "widget-utils.h"

/**************************************************************************
  Parsing utils
**************************************************************************/

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

static int parse_color(unsigned char *out, struct theme_format_entry *e)
{
	const char *v = find_theme_format_entry_value(e, "color");
	if (v) {
		if (3 == sscanf(v, "%d %d %d", &out[0], &out[1], &out[2]))
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

int parse_triple_image(struct triple_image *tbt,
		const char *name, struct theme_format_entry *e, 
		struct theme_format_tree *tree)
{
	struct theme_format_entry *ee = find_theme_format_entry(e, name);
	if (!ee)
		return -1;

	tbt->center = parse_image_part_named("center", ee, tree);
	if (!tbt->center)
		return xerror("Can't parse 'center' image of taskbar button theme");

	tbt->left = parse_image_part_named("left", ee, tree);
	tbt->right = parse_image_part_named("right", ee, tree);

	return 0;
}

void free_triple_image(struct triple_image *tbt)
{
	cairo_surface_destroy(tbt->center);
	if (tbt->left) 
		cairo_surface_destroy(tbt->left);
	if (tbt->right) 
		cairo_surface_destroy(tbt->right);
}

int parse_font_info(struct font_info *out, const char *name, 
		struct theme_format_entry *e, struct theme_format_tree *tree)
{
	struct theme_format_entry *ee = find_theme_format_entry(e, name);
	if (!ee || !ee->value)
		return -1;

	out->pfd = pango_font_description_from_string(ee->value);
	out->color[0] = out->color[1] = out->color[2] = 0;
	parse_color(out->color, ee);

	return 0;
}

void free_font_info(struct font_info *fi)
{
	pango_font_description_free(fi->pfd);
}

/**************************************************************************
  Drawing utils
**************************************************************************/

void blit_image(cairo_surface_t *src, cairo_t *dest, int dstx, int dsty)
{
	size_t sh = cairo_image_surface_get_height(src);
	size_t sw = cairo_image_surface_get_width(src);

	cairo_save(dest);
	cairo_set_source_surface(dest, src, dstx, dsty);
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
	cairo_set_source_surface(dest, src, 0, 0);
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
	
	cairo_translate(cr, x + offsetx, y + offsety + 1);
	cairo_rectangle(cr, 0, 0, w, h);
	cairo_clip(cr);
	pango_cairo_update_layout(cr, dest);
	pango_cairo_show_layout(cr, dest);
	cairo_restore(cr);
}

void text_extents(PangoLayout *layout, PangoFontDescription *font,
		const char *text, int *w, int *h)
{
	PangoRectangle r;
	pango_layout_set_font_description(layout, font);
	pango_layout_set_text(layout, text, -1);
	pango_layout_get_pixel_extents(layout, 0, &r);
	if (w)
		*w = r.width;
	if (h)
		*h = r.height;
}
