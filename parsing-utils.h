#ifndef BMPANEL2_PARSING_UTILS_H 
#define BMPANEL2_PARSING_UTILS_H

#include "gui.h"
#include "builtin-widgets.h"

int parse_image_part(struct image_part *imgp, struct theme_format_entry *e,
		struct theme_format_tree *tree);

int parse_taskbar_button_theme(struct taskbar_button_theme *tbt,
		struct theme_format_entry *e, struct theme_format_tree *tree);

#endif /* BMPANEL2_PARSING_UTILS_H */
