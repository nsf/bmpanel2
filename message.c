#include <stdarg.h>
#include <stdio.h>
#include "util.h"

int xerror(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	fputs("\n", stderr);
	va_end(args);

	return -1;
}

void xwarning(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	fputs("\n", stderr);
	va_end(args);
}

void xdie(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	fputs("\n", stderr);
	va_end(args);
	exit(EXIT_FAILURE);
}
