#include "util.h"
#include "array.h"

struct memory_source msrc_main = MEMSRC("Main", 0, 0, 0);

struct memory_source *msrc_list[] = {
	&msrc_main
};

int main(int argc, char **argv)
{
	DYN_ARRAY(int, a);
	DYN_ARRAY_ALLOCATE(a, 100, &msrc_main);
	xmemstat(msrc_list, 1, false);
	return 0;
}
