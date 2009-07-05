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

#define ALIGN_CENTER 0
#define ALIGN_LEFT 1
#define ALIGN_RIGHT 2
#define ALIGN_TOP 3
#define ALIGN_BOTTOM 4

struct text_info {
	PangoFontDescription *pfd;
	unsigned char color[3]; /* r, g, b */
	int offset_x;
	int offset_y;
	int align;
};

/* image part */
cairo_surface_t *parse_image_part(struct config_format_entry *e, 
				  struct config_format_tree *tree,
				  int required);
cairo_surface_t *parse_image_part_named(const char *name, 
					struct config_format_entry *e, 
					struct config_format_tree *tree,
					int required);
/* triple image */
int parse_triple_image(struct triple_image *tri, 
		       struct config_format_entry *e, 
		       struct config_format_tree *tree,
		       int required);
int parse_triple_image_named(struct triple_image *tri, 
			     const char *name, 
			     struct config_format_entry *e, 
			     struct config_format_tree *tree,
			     int required);
void free_triple_image(struct triple_image *tri);

/* text info */
int parse_text_info(struct text_info *out, 
		    struct config_format_entry *e);
int parse_text_info_named(struct text_info *out, const char *name,
			  struct config_format_entry *e, int required);
void free_text_info(struct text_info *fi);

/* complicated strings */
int parse_2ints(int *out1, int *out2, const char *name, struct config_format_entry *e);

/* align */
int parse_align(const char *name, struct config_format_entry *e);

/* simple things (int, string, etc.) */
int parse_int_or_percents(const char *name, struct config_format_entry *e,
			  int def, int *ispercents);
int parse_int(const char *name, struct config_format_entry *e, int def);
int parse_bool(const char *name, struct config_format_entry *e);
char *parse_string(const char *name, struct config_format_entry *e, 
		   const char *def);

/* nice error message */
void required_entry_not_found(struct config_format_entry *e, const char *name);

/**************************************************************************
  Drawing utils
**************************************************************************/

/* null tolerant */
int image_width(cairo_surface_t *img);
int image_height(cairo_surface_t *img);

void blit_image(cairo_surface_t *src, cairo_t *dest, int dstx, int dsty);
void blit_image_ex(cairo_surface_t *src, cairo_t *dest, int srcx, int srcy,
		   int width, int height, int dstx, int dsty);
void pattern_image(cairo_surface_t *src, cairo_t *dest, 
		int dstx, int dsty, int w, int h);

void draw_text(cairo_t *cr, PangoLayout *dest, struct text_info *ti, 
	       const char *text, int x, int y, int w, int h);
void text_extents(PangoLayout *layout, PangoFontDescription *font, 
		  const char *text, int *w, int *h);

/**************************************************************************
  X imaging utils
**************************************************************************/

cairo_t *create_cairo_for_pixmap(struct x_connection *c, Pixmap p, int w, int h);
cairo_t *create_cairo_for_bitmap(struct x_connection *c, Pixmap p, int w, int h);
cairo_surface_t *create_cairo_surface_for_pixmap(struct x_connection *c, Pixmap p,
						 int w, int h);
cairo_surface_t *get_window_icon(struct x_connection *c, Window win, 
				 cairo_surface_t *default_icon);
cairo_surface_t *copy_resized(cairo_surface_t *source, int w, int h);

/**************************************************************************
  Buffer utils
**************************************************************************/

#define STATIC_BUF_SIZE (256*256*4)

void *get_static_buf_or_xalloc(size_t size);
void free_static_buf(void *ptr);
void clean_static_buf();

#endif /* BMPANEL2_WIDGET_UTILS_H */
