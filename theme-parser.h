#ifndef BMPANEL2_THEME_PARSER_H 
#define BMPANEL2_THEME_PARSER_H

#include "util.h"

struct theme_entry {
	char *name;
	char *value;

	size_t children_n;
	struct theme_entry *children;
};

int theme_format_parse(struct theme_entry *tree, const char *filename);
int theme_format_parse_string(struct theme_entry *tree, const char *str);
void theme_format_free(struct theme_entry *tree);

extern struct memory_source msrc_theme;

#endif /* BMPANEL2_THEME_PARSER_H */
