#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include "util.h"

#if MEMDEBUG_LEVEL == 0
/**************************************************************************
  Memory debug level 0
**************************************************************************/
void *impl_xmalloc(size_t size, struct memory_source *src)
{
	void *ret = 0;

	if (src->malloc) {
		if (src->flags & MEMSRC_RETURN_IMMEDIATELY)
			return (*src->malloc)(size, src);
		else
			ret = (*src->malloc)(size + MEMDEBUG_OVERHEAD, src);
	}
		
	if (!ret)
		ret = malloc(size + MEMDEBUG_OVERHEAD);

	if (!ret)
		die("Out of memory, xmalloc failed.");

	return ret;
}

void *impl_xmallocz(size_t size, struct memory_source *src)
{
	void *ret = impl_xmalloc(size, src);
	memset(ret, 0, size);
	return ret;
}

void impl_xfree(void *ptr, struct memory_source *src)
{
	if (src->free)
		(*src->free)(ptr, src);
	else	
		free(ptr);
}

char *impl_xstrdup(const char *str, struct memory_source *src)
{
	size_t len = strlen(str);
	char *ret = impl_xmalloc(len, src);
	return strcpy(ret, str);
}
#elif MEMDEBUG_LEVEL == 1
/**************************************************************************
  Memory debug level 1
**************************************************************************/
void *impl_xmalloc(size_t size, struct memory_source *src)
{
	void *ret = 0;

	if (src->malloc) {
		if (src->flags & MEMSRC_RETURN_IMMEDIATELY)
			return (*src->malloc)(size, src);
		else
			ret = (*src->malloc)(size + MEMDEBUG_OVERHEAD, src);
	}
	
	if (!ret)
		ret = malloc(size + MEMDEBUG_OVERHEAD);

	if (!ret)
		die("Out of memory, xmalloc(z) failed.");

	*((size_t*)ret) = size;
	src->allocs++;
	src->bytes += size;

	ret += sizeof(size_t);
	return ret;
}

void *impl_xmallocz(size_t size, struct memory_source *src)
{
	void *ret = impl_xmalloc(size, src);
	memset(ret, 0, size);
	return ret;
}

void impl_xfree(void *ptr, struct memory_source *src)
{
	if (src->free && (src->flags & MEMSRC_RETURN_IMMEDIATELY)) {
		(*src->free)(ptr, src);
		return;
	}

	size_t *rptr = ptr - sizeof(size_t);

	src->frees++;
	src->bytes -= *rptr;

	if (src->free)
		(*src->free)(rptr, src);
	else
		free(rptr);
}

char *impl_xstrdup(const char *str, struct memory_source *src)
{
	size_t len = strlen(str);
	char *ret = impl_xmalloc(len, src);
	return strcpy(ret, str);
}
#elif MEMDEBUG_LEVEL == 2
/**************************************************************************
  Memory debug level 2
**************************************************************************/
void *impl_xmalloc(size_t size, struct memory_source *src, const char *file, unsigned int line)
{
	void *ret = 0;

	if (src->malloc) {
		if (src->flags & MEMSRC_RETURN_IMMEDIATELY)
			return (*src->malloc)(size, src);
		else
			ret = (*src->malloc)(size + MEMDEBUG_OVERHEAD, src);
	}
	
	if (!ret)
		ret = malloc(size + MEMDEBUG_OVERHEAD);

	if (!ret)
		die("Out of memory, xmalloc(z) failed.");

	struct memory_stat *stat = (struct memory_stat*)ret;
	stat->file = file;
	stat->line = line;
	stat->size = size;
	stat->prev = 0;

	if (!src->stat_list) {
		stat->next = 0;
		src->stat_list = stat;
	} else {
		stat->next = src->stat_list;
		src->stat_list->prev = stat;
		src->stat_list = stat;
	}
	src->allocs++;
	src->bytes += size;

	ret += sizeof(struct memory_stat);
	return ret;
}

void *impl_xmallocz(size_t size, struct memory_source *src, const char *file, unsigned int line)
{
	void *ret = impl_xmalloc(size, src, file, line);
	memset(ret, 0, size);
	return ret;
}

void impl_xfree(void *ptr, struct memory_source *src, const char *file, unsigned int line)
{
	if (src->free && (src->flags & MEMSRC_RETURN_IMMEDIATELY)) {
		(*src->free)(ptr, src);
		return;
	}

	struct memory_stat *memstat = ptr - sizeof(struct memory_stat);
	
	if (memstat->next)
		memstat->next->prev = (memstat->prev) ? memstat->prev : 0;
	if (memstat->prev)
		memstat->prev->next = (memstat->next) ? memstat->next : 0;
	if (src->stat_list == memstat)
		src->stat_list = memstat->next;

	src->frees++;
	src->bytes -= memstat->size;
	
	if (src->free)
		(*src->free)(memstat, src);
	else
		free(memstat);
}

char *impl_xstrdup(const char *str, struct memory_source *src, const char *file, unsigned int line)
{
	size_t len = strlen(str);
	char *ret = impl_xmalloc(len, src, file, line);
	return strcpy(ret, str);
}
#endif
/**************************************************************************
  Debug report utils
**************************************************************************/
static void print_source_stat(struct memory_source *src)
{
	int diff = (int)src->allocs - src->frees;

	printf("┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓\n");
	printf("┃ Source: %-63s ┃\n", src->name);
	printf("┠─────────────────────────────────────────────────────────────────────────┨\n");
	printf("┃ Allocs:      %-58u ┃\n", src->allocs);
	printf("┃ Frees:       %-58u ┃\n", src->frees);
	printf("┃ Diff:        %-58d ┃\n", diff);
	printf("┃ Bytes taken: %-58u ┃\n", src->bytes);
	printf("┃  + overhead: %-58d ┃\n", diff * MEMDEBUG_OVERHEAD);
	if (!diff || !src->stat_list) {
		printf("┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛\n");
	} else {
		printf("┠────────────┬────────────┬───────────────────────────────────────────────┨\n");
		printf("┃     ptr    │    size    │                   location                    ┃\n");
		printf("┠────────────┼────────────┼───────────────────────────────────────────────┨\n");
		struct memory_stat *stat = src->stat_list;
		while (stat) {
			char location[50];
			snprintf(location, sizeof(location), "%s:%u", stat->file, stat->line);
			location[sizeof(location)-1] = '\0';
			printf("┃ %10p │ %10u │ %-45s ┃\n", stat+1, stat->size, 
				location);
			stat = stat->next;
		}
		printf("┗━━━━━━━━━━━━┷━━━━━━━━━━━━┷━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛\n");
	}
}

static void print_source_stat_ascii(struct memory_source *src)
{
	int diff = (int)src->allocs - src->frees;

	printf("/=========================================================================\\\n");
	printf("| Source: %-63s |\n", src->name);
	printf("+-------------------------------------------------------------------------+\n");
	printf("| Allocs:      %-58u |\n", src->allocs);
	printf("| Frees:       %-58u |\n", src->frees);
	printf("| Diff:        %-58d |\n", diff);
	printf("| Bytes taken: %-58u |\n", src->bytes);
	printf("|  + overhead: %-58d |\n", diff * MEMDEBUG_OVERHEAD);
	if (!diff || !src->stat_list) {
		printf("\\=========================================================================/\n");
	} else {
		printf("+------------+------------+-----------------------------------------------+\n");
		printf("|     ptr    |    size    |                   location                    |\n");
		printf("+------------+------------+-----------------------------------------------+\n");
		struct memory_stat *stat = src->stat_list;
		while (stat) {
			char location[50];
			snprintf(location, sizeof(location), "%s:%u", stat->file, stat->line);
			location[sizeof(location)-1] = '\0';
			printf("| %10p | %10u | %-45s |\n", stat+1, stat->size, 
				location);
			stat = stat->next;
		}
		printf("\\============+============+===============================================/\n");
	}
}

void xmemstat(struct memory_source **sources, size_t n, bool ascii)
{
	size_t i;

	if (!MEMDEBUG_LEVEL) {
		printf("Memory debug report level: disabled\n");
		return;
	}

	printf("Memory debug level: %u\n", MEMDEBUG_LEVEL);
	printf("Memory debug overhead: %u bytes\n", MEMDEBUG_OVERHEAD);
	
	for (i = 0; i < n; ++i) {
		if (i % 2)
			printf("\033[36m");
		else 
			printf("\033[0m");

		if (ascii)
			print_source_stat_ascii(sources[i]);
		else
			print_source_stat(sources[i]);
	}
	printf("\033[0m");
	fflush(stdout);
}
