#ifndef INCLUDE_STDIO_H_
#define INCLUDE_STDIO_H_

#include<stdarg.h>

/* kernel printf */
extern int printk(const char *fmt, ...);
extern int vprintk(const char *fmt, va_list va);

/* user printk */
extern int printf(const char *fmt, ...);//

#endif
