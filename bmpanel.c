#include "util.h"

struct memory_source msrc_main = MEMSRC("Main", 0, 0, 0);

struct memory_source *msrc_list[] = {
	&msrc_main
};

int main(int argc, char **argv)
{
	xmemstat(msrc_list, 1, false);
	return 0;
}
