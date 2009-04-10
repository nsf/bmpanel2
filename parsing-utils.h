#ifndef BMPANEL2_PARSING_UTILS_H 
#define BMPANEL2_PARSING_UTILS_H

#include "gui.h"

/**************************************************************************
  Parsing utils
**************************************************************************/

struct triple_image {
	cairo_surface_t *left;
	cairo_surface_t *center;
	cairo_surface_t *right;
};

int parse_triple_image(struct triple_image *tri, const char *name, 
		struct theme_format_entry *e, struct theme_format_tree *tree);
void free_triple_image(struct triple_image *tri);

cairo_surface_t *parse_image_part(struct theme_format_entry *e,
		struct theme_format_tree *tree);
cairo_surface_t *parse_image_part_named(const char *name, struct theme_format_entry *e,
		struct theme_format_tree *tree);

#endif /* BMPANEL2_PARSING_UTILS_H */
