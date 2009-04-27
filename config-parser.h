#ifndef BMPANEL2_CONFIG_PARSER_H 
#define BMPANEL2_CONFIG_PARSER_H

#include "util.h"

/* A named config format entry with optional associated value and children. It
 * is capable of building trees of entries.
 */
struct config_format_entry {
	char *name; 
	char *value;

	struct config_format_entry *parent;

	size_t children_n;
	struct config_format_entry *children;

	size_t line;
};

/* A config format tree representation. 
 *
 * "dir" is a directory of a config file.
 *
 * "buf" is a buffer containing modified config format data (used for in-situ
 * parsing). Usually it's a pointer to a zero-terminated string which
 * represents modified contents of a config file. Normally not used
 * directly (private data). 
 *
 * "root" is the root of config format entries tree. Name and value of this root
 * always point to zero. Only "children_n" and "children" values are
 * meaningful. 
 */
struct config_format_tree {
	char *dir;
	char *buf;
	struct config_format_entry root;
};

/* Load the "tree" from a file located at "path". After successful loading
 * "tree" should be released using "free_config_format_tree" function when the
 * data isn't needed anymore.
 *
 * RETURNS
 * 	0 - success.
 * 	-1 - error (logged to stderr).
 */
int load_config_format_tree(struct config_format_tree *tree, const char *path);
void free_config_format_tree(struct config_format_tree *tree);

/* Find child entry of entry "e" with name "name".
 *
 * RETURNS
 * 	Null pointer if not found, a pointer to the entry on success.
 */
struct config_format_entry *find_config_format_entry(struct config_format_entry *e, 
		const char *name);

/* Same as above, but returns the "value" or 0 if not found or no "value". */
const char *find_config_format_entry_value(struct config_format_entry *e, 
		const char *name);

/* Write a full path to config format entry "e" to a buffer "buf", using simple
 * notation: "name/name/name/e->name"
 */
void config_format_entry_path(char *buf, size_t size, struct config_format_entry *e);

#endif /* BMPANEL2_CONFIG_PARSER_H */
