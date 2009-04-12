#ifndef BMPANEL2_WIDGET_UTILS_H 
#define BMPANEL2_WIDGET_UTILS_H

#include "gui.h"

/**************************************************************************
  Parsing utils
**************************************************************************/

struct triple_image {
	cairo_surface_t *left;
	cairo_surface_t *center;
	cairo_surface_t *right;
};

struct font_info {
	PangoFontDescription *pfd;
	unsigned char color[3];
};

int parse_triple_image(struct triple_image *tri, const char *name, 
		struct theme_format_entry *e, struct theme_format_tree *tree);
void free_triple_image(struct triple_image *tri);

cairo_surface_t *parse_image_part(struct theme_format_entry *e,
		struct theme_format_tree *tree);
cairo_surface_t *parse_image_part_named(const char *name, struct theme_format_entry *e,
		struct theme_format_tree *tree);
int parse_font_info(struct font_info *out, const char *name, 
		struct theme_format_entry *e, struct theme_format_tree *tree);
void free_font_info(struct font_info *fi);

/**************************************************************************
  Drawing utils
**************************************************************************/

void blit_image(cairo_surface_t *src, cairo_t *dest, int dstx, int dsty);
void pattern_image(cairo_surface_t *src, cairo_t *dest, 
		int dstx, int dsty, int w);

#define TEXT_ALIGN_CENTER 0
#define TEXT_ALIGN_LEFT 1
#define TEXT_ALIGN_RIGHT 2

void draw_text(cairo_t *cr, PangoLayout *dest, PangoFontDescription *font, 
		const char *text, unsigned char *color, int x, int y, 
		int w, int h, int align);
void text_extents(PangoLayout *layout, PangoFontDescription *font,
		const char *text, int *w, int *h);

#endif /* BMPANEL2_WIDGET_UTILS_H */
