#include <stdio.h>
#include <string.h>
#include "theme-parser.h"

struct memory_source msrc_theme = MEMSRC(
	"Theme format", 
	MEMSRC_DEFAULT_MALLOC, 
	MEMSRC_DEFAULT_FREE, 
	MEMSRC_NO_FLAGS
);

struct parse_context {
	char *cur;
};

static int count_and_skip_indent(struct parse_context *ctx)
{
	int indent = 0;
	while (*ctx->cur == ' ' || *ctx->cur == '\t') {
		indent++;
		ctx->cur++;
	}
	return indent;
}

static int count_indent(struct parse_context *ctx)
{
	int indent = 0;
	const char *tmp = ctx->cur;
	while (*tmp == ' ' || *tmp == '\t') {
		indent++;
		tmp++;
	}
	return indent;
}

#define LINE_COMMENT 0
#define LINE_EMPTY 1
#define LINE_ENTRY 2

/* Classify line using its first non-space character */
static int classify_line(char first_char)
{
	switch (first_char) {
		case '#':
			return LINE_COMMENT;
		case '\n':
			return LINE_EMPTY;
		default:
			return LINE_ENTRY;
	}
}

/* Counts a number of children entries of the entry with indent level equals to
 * 'indent_level'. Also returns an indent level of first children
 * ('chld_indent').
 *
 * Returns:
 * 	A number of children.
 */
static size_t count_children(int indent_level, struct parse_context *ctx, int *chld_indent)
{
	char *pos = ctx->cur;
	size_t children_count = 0;
	int children_indent = -1;
	while (*ctx->cur) {
		int indent = count_and_skip_indent(ctx);
		if (indent > indent_level && classify_line(*ctx->cur) == LINE_ENTRY) {
			if (children_indent == -1)
				children_indent = indent;
			if (indent == children_indent) {
				children_count++;
			}
		} else if (indent <= indent_level && classify_line(*ctx->cur) == LINE_ENTRY)
			break;
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
	ctx->cur = pos;
	if (chld_indent)
		*chld_indent = children_indent;
	return children_count;
}

static int parse_children(struct theme_entry *te, int indent_level, struct parse_context *ctx);

/* Parse name and value of entry ('ctx->cur' should point at the beginning of
 * the string) with indent level equals to 'indent_level'. And then parse its
 * children.
 *
 * Returns:
 * 	0 always, no errors.
 */
#if 0
static int parse_entry(struct theme_entry *te, int indent_level, struct parse_context *ctx)
{
	/* extract name */
	const char *start = ctx->cur;
	while (*ctx->cur != ' ' 
		&& *ctx->cur != '\t' 
		&& *ctx->cur != '\n' 
		&& *ctx->cur != '\0')
	{
		ctx->cur++;
	}
	const char *end = ctx->cur;
	size_t nlen = end - start;

	te->name = xmalloc(nlen+1, &msrc_theme);
	te->name[nlen] = '\0';
	strncpy(te->name, start, nlen);

	/* skip spaces between name and value */
	if (*ctx->cur == ' ' || *ctx->cur == '\t')
		while (*ctx->cur == ' ' || *ctx->cur == '\t')
			ctx->cur++;

	/* extract value if it exists */
	const char *vstart, *vend;
	size_t vlen;
	switch (*ctx->cur) {
		case '\n':
			ctx->cur++; /* next line */
		case '\0':
			break;
		default:
			vstart = ctx->cur;
			while (*ctx->cur != '\n' && *ctx->cur != '\0')
				ctx->cur++;
			vend = ctx->cur;
			vlen = vend - vstart;

			te->value = xmalloc(vlen+1, &msrc_theme);
			te->value[vlen] = '\0';
			strncpy(te->value, vstart, vlen);
			ctx->cur++; /* next line */
	}

	/* continue parsing */
	return parse_children(te, indent_level, ctx);
}
#endif

/* Parse name and value of entry ('ctx->cur' should point at the beginning of
 * the string) with indent level equals to 'indent_level'. And then parse its
 * children.
 *
 * IN-SITU version. Uses and modifies buffer memory in 'ctx'.
 *
 * Returns:
 * 	0 always, no errors.
 */
static int parse_entry_insitu(struct theme_entry *te, int indent_level, struct parse_context *ctx)
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
	char *end = ctx->cur;

	te->name = start;

	/* skip spaces between name and value */
	if (*ctx->cur == ' ' || *ctx->cur == '\t')
		while (*ctx->cur == ' ' || *ctx->cur == '\t')
			ctx->cur++;

	/* extract value if it exists */
	char *vstart;
	char *vend;
	switch (*ctx->cur) {
		case '\n':
			ctx->cur++; /* next line */
		case '\0':
			break;
		default:
			vstart = ctx->cur;
			while (*ctx->cur != '\n' && *ctx->cur != '\0')
				ctx->cur++;
			vend = ctx->cur;

			te->value = vstart;
			*vend = '\0';
			ctx->cur++; /* next line */
	}
	
	*end = '\0';
	
	/* continue parsing */
	return parse_children(te, indent_level, ctx);
}

/* Parse children entries of the entry 'te' with indent equals to
 * 'indent_level'. Function takes first children (first entry with indent level
 * more than 'indent_level') and then thinks of all next entries with the same
 * indent level as other children. Other entries are skipped. Function stops
 * when entry with indent <= 'indent_level' is found.
 *
 * example:
 *
 * parent value
 *    children1
 *   not_children__skipped
 *    children2
 *     children_of_2
 *    children3
 *
 * So, it's kinda error-proof. Parser will parse eventually any text file.
 *
 * Returns:
 * 	0 always, no errors.
 */
static int parse_children(struct theme_entry *te, int indent_level, struct parse_context *ctx)
{
	int children_indent = -1;
	size_t children = 0;
	char *pos;
	te->children_n = count_children(indent_level, ctx, &children_indent);
	if (!te->children_n)
		return 0;

	te->children = xmallocz(sizeof(struct theme_entry) * te->children_n, &msrc_theme);
	
	/* HELL LOGIC */
	while (*ctx->cur) {
		int indent = count_and_skip_indent(ctx);
		if (indent == children_indent && classify_line(*ctx->cur) == LINE_ENTRY) {
			/* we're interested in this line */
			parse_entry_insitu(&te->children[children], indent, ctx);
			pos = ctx->cur; /* remember position after line */
			children++;
			if (children == te->children_n)
				break;
			switch (*ctx->cur) {
				case '\0':
					break;
				default: 
					continue;
			}
		} else {
			/* skip line */
			while (*ctx->cur != '\n' && *ctx->cur != '\0')
				ctx->cur++;
			switch (*ctx->cur) {
				case '\n':
					ctx->cur++;
					continue;
				case '\0':
					;
			}
		}
		pos = ctx->cur;
		break;
	}
	ctx->cur = pos;

	return 0;
}

/* Parse theme format tree from null-terminated string. 
 *
 * Returns:
 * 	0 on success.
 * 	non-zero on error. 
 */
int theme_format_parse_string(struct theme_entry *tree, char *str)
{
	struct parse_context ctx = {str};
	memset(tree, 0, sizeof(struct theme_entry));
	parse_children(tree, -1, &ctx);
	return 0;
}

/* Read and parse theme format tree from a file.
 *
 * Returns:
 * 	0 on error.
 * 	non-zero on error. 
 */
int theme_format_parse(struct theme_data *data, const char *filename)
{
	size_t size;
	size_t read;
	char *buf;
	FILE *f = fopen(filename, "rb");

	if (!f)
		return -1;

	fseek(f, 0, SEEK_END);
	size = ftell(f);
	fseek(f, 0, SEEK_SET);

	/* read file contents to buffer */
	buf = xmalloc(size+1, &msrc_theme);
	buf[size] = '\0';
	read = fread(buf, 1, size, f);
	if (read != size) {
		xfree(buf, &msrc_theme);
		return -1;
	}

	fclose(f);

	/* use string parsing function to actually parse */
	int ret = theme_format_parse_string(&data->root, buf);

	/* assign buffer */
	data->buffer = buf;
	return ret;
}

void theme_data_free(struct theme_data *data)
{
	theme_format_free(&data->root);
	xfree(data->buffer, &msrc_theme);
}

/* Free theme format tree */
void theme_format_free(struct theme_entry *tree)
{
	size_t i;
	for (i = 0; i < tree->children_n; i++)
		theme_format_free(&tree->children[i]);
	if (tree->children)
		xfree(tree->children, &msrc_theme);
}
