#ifndef BMPANEL2_THEME_PARSER_H 
#define BMPANEL2_THEME_PARSER_H

#include "util.h"

struct theme_entry {
	char *name;
	char *value;

	size_t children_n;
	struct theme_entry *children;
};

struct theme_data {
	char *buffer;
	struct theme_entry root;
};

int theme_format_parse(struct theme_data *data, const char *filename);
int theme_format_parse_string(struct theme_entry *tree, char *str);
void theme_data_free(struct theme_data *data);
void theme_format_free(struct theme_entry *tree);

extern struct memory_source msrc_theme;

#endif /* BMPANEL2_THEME_PARSER_H */
