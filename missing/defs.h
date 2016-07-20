#ifndef DEFS__H
#define DEFS__H

#define __P(x) x
#define __BEGIN_DECLS
#define __END_DECLS

#ifdef ULTRIX
typedef int ssize_t;
#endif

#if defined(vax) || defined (__sparc__)
#ifndef __sparc__
typedef __signed char              int8_t;
typedef short                     int16_t;
typedef int                       int32_t;
/* LONGLONG */
typedef long long                 int64_t;
#endif

typedef unsigned char            u_int8_t;
typedef unsigned short          u_int16_t;
typedef unsigned int            u_int32_t;
/* LONGLONG */
typedef unsigned long long      u_int64_t;
#endif

#endif
