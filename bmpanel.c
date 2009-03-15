#include <stdio.h>
#include "util.h"
#include "theme-parser.h"

struct memory_source msrc_main = MEMSRC("Main", 0, 0, 0);

struct memory_source *msrc_list[] = {
	&msrc_main,
	&msrc_theme
};

static void print_tree(struct theme_entry *te, int indent)
{
	int i;
	for (i = 0; i < indent; ++i) 
		printf("* ");

	printf("%s = %s\n", te->name, te->value);
	for (i = 0; i < te->children_n; ++i)
		print_tree(&te->children[i], indent+1);
}

int main(int argc, char **argv)
{
	struct theme_entry *te = theme_format_parse("test.txt");
	print_tree(te, 0);
	theme_format_free(te);
	xmemstat(msrc_list, 2, false);
	return 0;
}
