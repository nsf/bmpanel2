#ifndef BMPANEL2_UTIL_H 
#define BMPANEL2_UTIL_H

#include <stdlib.h>
#include <stdbool.h>

#define ARRAY_LENGTH(a) (sizeof((a)) / sizeof((a)[0]))

/**************************************************************************
  message utils
**************************************************************************/

void xwarning(const char *fmt, ...) __attribute__((format (printf, 1, 2)));
void xdie(const char *fmt, ...) __attribute__((format (printf, 1, 2)));

/**************************************************************************
  memory utils
**************************************************************************/

/* Memory source helper macro */
#define MEMSRC(name, malloc, free, flags) \
	{name, 0, 0, 0, 0, (malloc), (free), (flags)}

/* Defaults for convenience. */
#define MEMSRC_DEFAULT_MALLOC (0)
#define MEMSRC_DEFAULT_FREE (0)
#define MEMSRC_NO_FLAGS (0)

/* 
 * When this flag is set "x" alloc functions directly return the result of
 * custom malloc (defined in a memory_source) without any other activity. Also
 * allocs pass exact requested size to this custom function (without
 * MEMDEBUG_OVERHEAD added). If a memory source have no custom malloc
 * function, the flag is ignored.
 */
#define MEMSRC_RETURN_IMMEDIATELY (1 << 0)

struct memory_stat {
	struct memory_stat *next;
	struct memory_stat *prev;
	const char *file;
	unsigned int line;
	size_t size;
};

struct memory_source {
	const char *name;
	unsigned int allocs;
	unsigned int frees;
	int bytes;
	struct memory_stat *stat_list;

	void *(*malloc)(size_t, struct memory_source*);
	void (*free)(void*, struct memory_source*);

	unsigned int flags;
};

/* overheads */
#ifdef NDEBUG
 #define MEMDEBUG_OVERHEAD (0)
#else
 #define MEMDEBUG_OVERHEAD (sizeof(struct memory_stat))
#endif

/* functions */
#ifdef NDEBUG
	#define xmalloc(a, s) 	impl_xmalloc((a), (s))
	#define xmallocz(a, s) 	impl_xmallocz((a), (s))
	#define xfree(a, s) 	impl_xfree((a), (s))
	#define xstrdup(a, s) 	impl_xstrdup((a), (s))
	
	void *impl_xmalloc(size_t size, struct memory_source *src);
	void *impl_xmallocz(size_t size, struct memory_source *src);
	void impl_xfree(void *ptr, struct memory_source *src);
	char *impl_xstrdup(const char *str, struct memory_source *src);
#else
	#define xmalloc(a, s) 	impl_xmalloc((a), (s), __FILE__, __LINE__)
	#define xmallocz(a, s) 	impl_xmallocz((a), (s), __FILE__, __LINE__)
	#define xfree(a, s) 	impl_xfree((a), (s), __FILE__, __LINE__)
	#define xstrdup(a, s) 	impl_xstrdup((a), (s), __FILE__, __LINE__)

	void *impl_xmalloc(size_t size, struct memory_source *src, const char *file, unsigned int line);
	void *impl_xmallocz(size_t size, struct memory_source *src, const char *file, unsigned int line);
	void impl_xfree(void *ptr, struct memory_source *src, const char *file, unsigned int line);
	char *impl_xstrdup(const char *str, struct memory_source *src, const char *file, unsigned int line);
#endif

/*
 * Prints out an info table about memory sources array "sources" of size "n".
 * If "ascii" is true, prints using ASCII characters only. Else, uses unicode 
 * box drawing characters.
 */
void xmemstat(struct memory_source **sources, size_t n, bool ascii);

#endif /* BMPANEL2_UTIL_H */
