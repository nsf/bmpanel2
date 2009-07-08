#include <stdarg.h>
#include <stdio.h>
#include "util.h"

int xerror(const char *file, unsigned int line, const char *fmt, ...)
{
	va_list args;

#ifndef NDEBUG
	fprintf(stderr, "(%s:%u) ", file, line);
#endif
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	fputs("\n", stderr);
	va_end(args);

	return -1;
}

void xwarning(const char *file, unsigned int line, const char *fmt, ...)
{
	va_list args;

#ifndef NDEBUG
	fprintf(stderr, "(%s:%u) ", file, line);
#endif
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	fputs("\n", stderr);
	va_end(args);
}

void xdie(const char *file, unsigned int line, const char *fmt, ...)
{
	va_list args;

#ifndef NDEBUG
	fprintf(stderr, "FATAL (%s:%u) ", file, line);
#endif
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	fputs("\n", stderr);
	va_end(args);
	exit(EXIT_FAILURE);
}

const char *pretty_print_FILE(const char *file)
{
	const char *base = strstr(file, PRETTY_PRINT_FILE_BASE);
	return ((base) ? base + strlen(PRETTY_PRINT_FILE_BASE) + 1 : file);
}

/**************************************************************************
  string buffer
**************************************************************************/

/* TODO: move these somewhere else.. where? */
void strbuf_assign(struct strbuf *sb, const char *str)
{
	size_t len = strlen(str);
	if (sb->alloc == 0) {
		sb->alloc = len + 1;
		sb->buf = xmalloc(sb->alloc);
		strcpy(sb->buf, str);
	} else {
		if (sb->alloc >= len + 1) {
			strcpy(sb->buf, str);
		} else {
			xfree(sb->buf);
			sb->alloc = len + 1;
			sb->buf = xmalloc(sb->alloc);
			strcpy(sb->buf, str);
		}
	}
}

void strbuf_free(struct strbuf *sb)
{
	if (sb->alloc)
		xfree(sb->buf);
}

