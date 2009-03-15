#include "util.h"
#include "theme-parser.h"

struct memory_source msrc_main = MEMSRC("Main", 0, 0, 0);

struct memory_source *msrc_list[] = {
	&msrc_main
};

int main(int argc, char **argv)
{
	theme_format_parse("test.txt");
	return 0;
}
