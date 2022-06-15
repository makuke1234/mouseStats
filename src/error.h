#ifndef ERROR_H
#define ERROR_H

#include "winapi.h"
#include "resource.h"

#include <stdarg.h>

#define MAX_EMSG 2048

#ifdef _DEBUG
	#define ePrint(...) eCon(__VA_ARGS__)
#else
	#define ePrint(...) eMsg(__VA_ARGS__)
#endif

int eCon(const char * restrict format, ...);
int evCon(const char * restrict format, va_list ap);

int eMsg(const char * restrict format, ...);
int evMsg(const char * restrict format, va_list ap);

#endif
