#ifndef BMPANEL2_PARSING_UTILS_H 
#define BMPANEL2_PARSING_UTILS_H

#include "theme-parser.h"
#include "gui.h"

cairo_surface_t *parse_image_part(struct theme_format_entry *e,
		struct theme_format_tree *tree);
cairo_surface_t *parse_image_part_named(const char *name, struct theme_format_entry *e,
		struct theme_format_tree *tree);

#endif /* BMPANEL2_PARSING_UTILS_H */
