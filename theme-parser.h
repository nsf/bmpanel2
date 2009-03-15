#ifndef BMPANEL2_THEME_PARSER_H 
#define BMPANEL2_THEME_PARSER_H

#include "util.h"

struct theme_entry {
	char *name;
	char *value;

	size_t children_n;
	struct theme_entry *children;
};

struct theme_entry *theme_format_parse(const char *filename);
struct theme_entry *theme_format_parse_string(const char *str);
void theme_format_free(struct theme_entry *tree);

extern struct memory_source msrc_theme;

#endif /* BMPANEL2_THEME_PARSER_H */
