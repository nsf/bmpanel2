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

#define TEXT_ALIGN_CENTER 0
#define TEXT_ALIGN_LEFT 1
#define TEXT_ALIGN_RIGHT 2

struct text_info {
	PangoFontDescription *pfd;
	unsigned char color[3]; /* r, g, b */
	int offset[2]; /* x, y */
	int align;
};

/* image part */
cairo_surface_t *parse_image_part(struct theme_format_entry *e,
		struct theme_format_tree *tree);
cairo_surface_t *parse_image_part_named(const char *name, struct theme_format_entry *e,
		struct theme_format_tree *tree);

/* triple image */
int parse_triple_image(struct triple_image *tri, struct theme_format_entry *e, 
		struct theme_format_tree *tree);
int parse_triple_image_named(struct triple_image *tri, const char *name,
		struct theme_format_entry *e, struct theme_format_tree *tree);
void free_triple_image(struct triple_image *tri);

/* text info */
int parse_text_info(struct text_info *out, const char *name, 
		struct theme_format_entry *e);
void free_text_info(struct text_info *fi);

/* simple things (int, string, etc.) */
int parse_int(const char *name, struct theme_format_entry *e, int def);
char *parse_string(const char *name, struct theme_format_entry *e, const char *def);

/**************************************************************************
  Drawing utils
**************************************************************************/

void blit_image(cairo_surface_t *src, cairo_t *dest, int dstx, int dsty);
void pattern_image(cairo_surface_t *src, cairo_t *dest, 
		int dstx, int dsty, int w);

void draw_text(cairo_t *cr, PangoLayout *dest, struct text_info *ti, 
		const char *text, int x, int y, int w, int h);
void text_extents(PangoLayout *layout, PangoFontDescription *font,
		const char *text, int *w, int *h);

/**************************************************************************
  X imaging utils
**************************************************************************/

cairo_surface_t *get_window_icon(struct x_connection *c, Window win,
		cairo_surface_t *default_icon);

#endif /* BMPANEL2_WIDGET_UTILS_H */
