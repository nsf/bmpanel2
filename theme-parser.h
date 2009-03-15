#ifndef BMPANEL2_THEME_PARSER_H 
#define BMPANEL2_THEME_PARSER_H

#include "util.h"

struct theme_entry {
	const char *name;
	const char *value;

	size_t children_n;
	struct theme_entry *children;
};

struct theme_entry *theme_format_parse(const char *filename);
void theme_format_free(struct theme_entry *tree);

#endif /* BMPANEL2_THEME_PARSER_H */
