/* $Id: snprintf.c,v 1.1 1999/05/30 11:20:52 jens Exp $ */

#include <sys/types.h>
#include <stdarg.h>

int vsprintf(char *str, const char *fmt, va_list ap);

int
vsnprintf(char *str, size_t size, const char *fmt, va_list ap)
{
	/*
	 * XXX
	 * Make a prayer and hope for no buffer overruns ;-)
	 */
	return vsprintf(str, fmt, ap);		
}

int
snprintf(char *str, size_t size, const char *fmt, ...)
{
	va_list	ap;
	int	res;

	va_start(ap, fmt);
	res = vsnprintf(str, size, fmt, ap)
	va_end(ap);
	return res;
}

