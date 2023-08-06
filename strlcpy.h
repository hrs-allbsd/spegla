/* $Id: strlcpy.h,v 1.1 1999/09/28 20:58:33 jens Exp $ */
#ifndef STRLCPY__H
#define STRLCPY__H

#if !(defined(__APPLE__) && defined(__clang__))
/* On macOS with Clang a conflict with the OS headers arises */
size_t strlcpy(char *dst, const char *src, size_t size);
size_t strlcat(char *dst, const char *src, size_t size);
#endif

#endif /* STRLCPY__H */
