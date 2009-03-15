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
	const char *buf;
	const char *cur;
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

static size_t count_children(int indent_level, struct parse_context *ctx, int *chld_indent)
{
	const char *pos = ctx->cur;
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

static int parse_entry(struct theme_entry *te, int indent_level, struct parse_context *ctx)
{
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
	
	te->name = xmalloc(nlen, &msrc_theme);
	te->name[nlen] = '\0';
	strncpy(te->name, start, nlen);

	if (*ctx->cur == ' ' || *ctx->cur == '\t')
		while (*ctx->cur == ' ' || *ctx->cur == '\t')
			ctx->cur++;

	const char *vstart, *vend;
	size_t vlen;
	switch (*ctx->cur) {
		case '\n':
		case '\0':
			break;
		default:
			vstart = ctx->cur;
			while (*ctx->cur != '\n' && *ctx->cur != '\0')
				ctx->cur++;
			vend = ctx->cur;
			vlen = vend - vstart;

			te->value = xmalloc(vlen, &msrc_theme);
			te->value[vlen] = '\0';
			strncpy(te->value, vstart, vlen);
	}

	parse_children(te, indent_level, ctx);
}

static int parse_children(struct theme_entry *te, int indent_level, struct parse_context *ctx)
{
	/* fast forward count children */
	int children_indent = -1;
	size_t children = 0;
	const char *pos;
	te->children_n = count_children(indent_level, ctx, &children_indent);
	if (!te->children_n)
		return 0;

	te->children = xmallocz(sizeof(struct theme_entry) * te->children_n, &msrc_theme);
	
	while (*ctx->cur) {
		pos = ctx->cur;
		int indent = count_and_skip_indent(ctx);
		if (indent == children_indent && classify_line(*ctx->cur) == LINE_ENTRY) {
			parse_entry(&te->children[children], indent, ctx);
			children++;
		}
		if (children == te->children_n)
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

	return 0;
}

struct theme_entry *theme_format_parse_string(const char *str)
{
	struct parse_context ctx = {str, str};
	struct theme_entry *te = xmallocz(sizeof(struct theme_entry), &msrc_theme);
	parse_children(te, -1, &ctx);
	return te;
}

struct theme_entry *theme_format_parse(const char *filename)
{
	size_t size;
	size_t read;
	char *buf;
	FILE *f = fopen(filename, "rb");

	if (!f)
		return 0;

	fseek(f, 0, SEEK_END);
	size = ftell(f);
	fseek(f, 0, SEEK_SET);

	buf = xmalloc(size+1, &msrc_theme);
	buf[size] = '\0';
	read = fread(buf, 1, size, f);
	if (read != size) {
		xfree(buf, &msrc_theme);
		xwarning("theme_format_parse: read error occured");
		return 0;
	}

	fclose(f);

	/* parse */
	struct theme_entry *ret = theme_format_parse_string(buf);

	xfree(buf, &msrc_theme);
	return ret;
}

static void free_non_root(struct theme_entry *tree)
{
	size_t i;
	if (tree->name)
		xfree(tree->name, &msrc_theme);
	if (tree->value)
		xfree(tree->value, &msrc_theme);
	for (i = 0; i < tree->children_n; i++)
		free_non_root(&tree->children[i]);
	if (tree->children)
		xfree(tree->children, &msrc_theme);
}

void theme_format_free(struct theme_entry *tree)
{
	free_non_root(tree);
	xfree(tree, &msrc_theme);
}
