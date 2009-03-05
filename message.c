#include <stdarg.h>
#include <stdio.h>
#include "util.h"

void warning(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	fputs("warning: ", stderr);
	vfprintf(stderr, fmt, args);
	fputs("\n", stderr);
	va_end(args);
}

void die(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	fputs("fatal: ", stderr);
	vfprintf(stderr, fmt, args);
	fputs("\n", stderr);
	va_end(args);
	exit(1);
}
