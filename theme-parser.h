#ifndef BMPANEL2_THEME_PARSER_H 
#define BMPANEL2_THEME_PARSER_H

#include "util.h"

/* A named theme format entry with optional associated value and children. It
 * is capable of building trees of entries. 
 */
struct theme_format_entry {
	char *name; 
	char *value;

	size_t children_n;
	struct theme_format_entry *children;
};

/* A theme format tree representation. 
 *
 * "buf" is a buffer containing modified theme format data (used for in-situ
 * parsing). Usually it's a pointer to a zero-terminated string which
 * represents modified contents of a theme config file. Normally not used
 * directly (private data). 
 *
 * "root" is the root of theme format entries tree. Name and value of this root
 * always point to zero. Only "children_n" and "children" values are
 * meaningful. 
 */
struct theme_format_tree {
	char *buf;
	struct theme_format_entry root;
};

/* Load the "tree" from a file with name "filename". The "tree" structure
 * should be empty (all zeroes) or uninitialized (stack garbage). After
 * successful loading "tree" should be released using "theme_format_free_tree"
 * function when the data isn't needed anymore.
 *
 * RETURNS
 * 	0 (zero) - success, also fills "tree" structure with appropriate data.
 * 	Non-zero on error.
 */
int theme_format_load_tree(struct theme_format_tree *tree, const char *filename);
void theme_format_free_tree(struct theme_format_tree *tree);

/* Memory source used for working with theme format trees. */
extern struct memory_source msrc_theme;

#endif /* BMPANEL2_THEME_PARSER_H */
