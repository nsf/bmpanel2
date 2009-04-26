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

	size_t line;
	/* TODO: line counter */
};

/* A theme format tree representation. 
 *
 * "dir" is a directory of a theme file.
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
	char *dir;
	char *buf;
	struct theme_format_entry root;
};

/* Load the "tree" from a file located at "path"/"filename". The "tree"
 * structure should be empty (all zeroes) or uninitialized (stack garbage).
 * After successful loading "tree" should be released using
 * "free_theme_format_tree" function when the data isn't needed anymore.
 *
 * RETURNS
 * 	true - success.
 * 	false - error (logged to stderr).
 */
int load_theme_format_tree(struct theme_format_tree *tree, const char *path,
			   const char *filename);
void free_theme_format_tree(struct theme_format_tree *tree);

/* Find child entry of entry "e" with name "name".
 *
 * RETURNS
 * 	Zero - not found.
 * 	Non-zero - The pointer to the entry.
 */
struct theme_format_entry *find_theme_format_entry(struct theme_format_entry *e, 
		const char *name);

/* Same as above, but returns "value" or 0 if not found or no value. */
const char *find_theme_format_entry_value(struct theme_format_entry *e, 
		const char *name);

#endif /* BMPANEL2_THEME_PARSER_H */
