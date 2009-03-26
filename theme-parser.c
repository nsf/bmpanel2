#include <stdio.h>
#include <string.h>
#include "theme-parser.h"

struct memory_source msrc_theme = MEMSRC(
	"Theme format", 
	MEMSRC_DEFAULT_MALLOC, 
	MEMSRC_DEFAULT_FREE, 
	MEMSRC_NO_FLAGS
);

/* Tiny structure used for tracking current parsing position and probably other
 * parser related data (if any, currently none). 
 */
struct parse_context {
	char *cur;
};

/* predeclarations */
static int parse_children(struct theme_format_entry *te, int indent_level, struct parse_context *ctx);

/* Count indent symbols (tabs and spaces) and advance parsing context to the
 * first non-indent symbol. 
 *
 * Example string and positions (right *after* these symbols):
 * # - expected begin position
 * $ - expected end position
 *
 * ----------------------------------------------------
 *  monitor samsung
 * #	$width 1440
 * 	height 900
 * ----------------------------------------------------
 *
 * RETURNS
 * 	A number of indent symbols (aka indent level).
 */
static int count_and_skip_indent(struct parse_context *ctx)
{
	int indent = 0;
	while (*ctx->cur == ' ' || *ctx->cur == '\t') {
		indent++;
		ctx->cur++;
	}
	return indent;
}

/* Classify the line using its first non-space character. It's intended to do
 * this right after indent skipping. Parser uses this function to know what to
 * do next. If the line is comment or empty, it is being skipped. If the line
 * is entry, it's being parsed as entry. 
 */
static bool line_is_entry(char first_char)
{
	switch (first_char) {
		case '#':
		case '\n':
			return false;
		default:
			return true;
	}
}

/* Counts a number of child entries of the parent entry with indent level
 * equals to 'indent_level'. Also writes an indent level of the first child to
 * a variable at the address 'chld_indent' if it is not zero. This function
 * doesn't advance parse context position.
 *
 * Example string and positions (right *after* these symbols):
 * # - expected begin position
 * $ - expected end position
 *
 * ----------------------------------------------------
 *  monitor samsung
 * #$	width 1440
 *    bad_param skipped
 *         bad_param2 skipped
 * 	height 900
 *  mouse logitech
 *  	buttons 8
 * ----------------------------------------------------
 * 
 * (when #$ are next to each other, it means function doesn't advance parse
 * context)
 *
 * In this example function will return 2 and will write 1 to the
 * "chld_indent". As you can see it thinks of the child indent level as an
 * indent level of the first child. Also function skips all bad-formed children
 * (their indent level differs from an indent level of the first child). And of
 * course function stops counting when there is an entry with indent level
 * lower or equals to that the parent has.
 *
 * RETURNS
 * 	A number of children.
 */
static size_t count_children(int indent_level, struct parse_context *ctx, int *chld_indent)
{
	char *pos = ctx->cur; /* remember context position */
	size_t children_count = 0;
	int children_indent = -1;
	while (*ctx->cur) {
		int indent = count_and_skip_indent(ctx);
		if (line_is_entry(*ctx->cur)) {
			if (indent > indent_level) {
				/* if this is the first child, remember its indent level */
				if (children_indent == -1)
					children_indent = indent;
				/* count all children with that indent level (including
				   the first one) */
				if (indent == children_indent)
					children_count++;
			} else
				/* we're done, stop counting */
				break;
		}
		/* skip line */
		while (*ctx->cur != '\n' && *ctx->cur != '\0')
			ctx->cur++;
		switch (*ctx->cur) {
			case '\n':
				ctx->cur++;
				continue;
			case '\0':
				break;
		}
		break;
	}
	ctx->cur = pos; /* restore position */
	if (chld_indent)
		*chld_indent = children_indent;
	return children_count;
}

/* Parse an entry (name with optional value) and recurse to its children
 * parsing. Function writes parsed info to "te", and "te" shouldn't point to
 * zero. Function expects the parse context to be at the first non-indent
 * symbol of the line. And "indent_level" should be the number of indent
 * symbols on that line. This is tricky, but in my opinion it is required for a
 * nice recursion here. Also it is worth to notice that function modifies
 * buffer, because of in-situ parsing.
 * 
 * Example string and positions (right *after* these symbols):
 * # - expected begin position
 * $ - expected end position
 *
 * ----------------------------------------------------
 *  monitor samsung
 * 	#width 1440
 * $	height 900
 *  mouse logitech
 *  	buttons 8
 * ----------------------------------------------------
 * 
 * or another example with children:
 * 
 * ----------------------------------------------------
 *  monitor samsung
 *  	#parameters
 *  		width 1440
 *  		height 900
 * $mouse logitech
 *  	buttons 8
 * ----------------------------------------------------
 *
 * Yes, this function ends after its children, because of recursion. But before
 * calling "parse_children" it ends right after the line it was called on (like
 * in the first example). 
 *
 * RETURNS
 * 	Zero, always (can be safely ignored).
 */
static int parse_format_entry(struct theme_format_entry *te, int indent_level, struct parse_context *ctx)
{
	/* extract name */
	char *start = ctx->cur;
	while (*ctx->cur != ' ' 
		&& *ctx->cur != '\t' 
		&& *ctx->cur != '\n' 
		&& *ctx->cur != '\0')
	{
		ctx->cur++;
	}

	/* we got the "end" here, but we're not nullifing it yet, since it may
	   cause problems */
	char *end = ctx->cur;
	te->name = start;

	/* skip spaces between name and value */
	if (*ctx->cur == ' ' || *ctx->cur == '\t')
		while (*ctx->cur == ' ' || *ctx->cur == '\t')
			ctx->cur++;

	char *vstart;
	char *vend;
	switch (*ctx->cur) {
		/* these two cases mean we're done here */
		case '\n':
			ctx->cur++; /* next line */
		case '\0':
			break;
		default:
			/* extract value if it exists */
			vstart = ctx->cur;
			while (*ctx->cur != '\n' && *ctx->cur != '\0')
				ctx->cur++;
			vend = ctx->cur;

			te->value = vstart;
			*vend = '\0';
			ctx->cur++; /* next line */
	}
	
	/* delayed nullifing */
	*end = '\0';
	
	/* recurse to our children (function will decide if any) */
	return parse_children(te, indent_level, ctx);
}

/* Parse children entries of the entry "te" which has an indent level equals to
 * the "indent_level". Function takes first children (first entry with indent
 * level more than "indent_level") and then thinks of all next entries with the
 * same indent level as other children. Other entries are skipped. Function
 * stops when entry with indent lower or equals to "indent_level" is found.
 * 
 * Example string and positions (right *after* these symbols):
 * # - expected begin position
 * $ - expected end position
 *
 * ----------------------------------------------------
 *  monitor samsung
 * #	width 1440
 * 	height 900
 * $mouse logitech
 *  	buttons 8
 * ----------------------------------------------------
 *
 * RETURNS
 * 	A number of child entries were parsed.
 */
static int parse_children(struct theme_format_entry *te, int indent_level, struct parse_context *ctx)
{
	int children_indent = -1;
	size_t children = 0;
	char *pos;
	te->children_n = count_children(indent_level, ctx, &children_indent);
	/* if there is no children, return immediately */
	if (!te->children_n)
		return 0;

	/* allocate space for child entries */
	te->children = xmallocz(sizeof(struct theme_format_entry) * te->children_n, &msrc_theme);

	/* ok, this is the *main* parse loop actually, since parser starts from
	   virtual root's children. */
	while (*ctx->cur) {
		/* skip indent */
		int indent = count_and_skip_indent(ctx);
		if (indent == children_indent && line_is_entry(*ctx->cur)) {
			/* we're interested in this line (it's a child line) */
			parse_format_entry(&te->children[children], indent, ctx);
			/* remember position after line [and its children] */
			pos = ctx->cur; 
			children++; /* we're did one more child */

			/* are we done? */
			if (children == te->children_n)
				break;

			/* check the *end of all world* condition */
			if (*ctx->cur)
				continue;
		} else {
			/* skip line */
			while (*ctx->cur != '\n' && *ctx->cur != '\0')
				ctx->cur++;
			if (*ctx->cur) {
				ctx->cur++;
				continue;
			}
		}
		pos = ctx->cur; /* remember position after skipped line */
		break;
	}
	ctx->cur = pos; /* restore position */

	return children;
}

/* Parse theme format tree from a null-terminated string. 
 *
 * RETURNS
 * 	Non-zero on success. 
 * 	Zero on fail.
 */
static int theme_format_parse_string(struct theme_format_entry *tree, char *str)
{
	struct parse_context ctx = {str};
	memset(tree, 0, sizeof(struct theme_format_entry));
	/* trick the parser with -1 and parse zero-indent entries as children
	   of the root entry */
	return parse_children(tree, -1, &ctx);
}

int theme_format_load_tree(struct theme_format_tree *tree, const char *path)
{
	long fsize;
	size_t size;
	size_t read;
	char *buf;
	FILE *f;
	char *theme_file;

	theme_file = xmalloc(strlen(path) + 7, &msrc_theme);
	sprintf(theme_file, "%s/theme", path);
	f = fopen(theme_file, "rb");
	xfree(theme_file, &msrc_theme);
	if (!f)
		return THEME_FORMAT_BAD_FILE;

	if (fseek(f, 0, SEEK_END) == -1)
		return THEME_FORMAT_READ_ERROR;
	fsize = ftell(f);
	if (fsize == -1)
		return THEME_FORMAT_READ_ERROR;
	size = (size_t)fsize;
	if (fseek(f, 0, SEEK_SET) == -1)
		return THEME_FORMAT_READ_ERROR;

	/* read file contents to buffer */
	buf = xmalloc(size+1, &msrc_theme);
	buf[size] = '\0';
	read = fread(buf, 1, size, f);
	if (read != size) {
		xfree(buf, &msrc_theme);
		return THEME_FORMAT_READ_ERROR;
	}

	fclose(f);

	/* use string parsing function to actually parse */
	int children_n = theme_format_parse_string(&tree->root, buf);
	if (children_n == 0) {
		xfree(buf, &msrc_theme);
		return THEME_FORMAT_FILE_IS_EMPTY;
	}

	/* assign buffer and dir */
	tree->buf = buf;
	tree->dir = xstrdup(path, &msrc_theme);
	return 0;
}

/* Free theme format tree */
void theme_format_free_entry(struct theme_format_entry *e)
{
	size_t i;
	for (i = 0; i < e->children_n; i++)
		theme_format_free_entry(&e->children[i]);
	if (e->children)
		xfree(e->children, &msrc_theme);
}

void theme_format_free_tree(struct theme_format_tree *tree)
{
	theme_format_free_entry(&tree->root);
	xfree(tree->buf, &msrc_theme);
	xfree(tree->dir, &msrc_theme);
}

struct theme_format_entry *theme_format_find_entry(struct theme_format_entry *e, 
		const char *name)
{
	int i;
	for (i = 0; i < e->children_n; ++i) {
		if (strcmp(e->children[i].name, name) == 0)
			return &e->children[i];
	}
	return 0;
}

const char *theme_format_find_entry_value(struct theme_format_entry *e, 
		const char *name)
{
	struct theme_format_entry *ee = theme_format_find_entry(e, name);
	return (ee) ? ee->value : 0;
}
