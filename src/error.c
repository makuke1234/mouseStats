#include "error.h"

int eCon(const char * restrict format, ...)
{
	assert(format != NULL);
	
	va_list ap;
	va_start(format, ap);
	
	int ret = evCon(format, ap);
	
	va_end(ap);
	
	return ret;
}
int evCon(const char * restrict format, va_list ap)
{
	
}

int eMsg(const char * restrict format, ...)
{
	assert(format != NULL);
	
	va_list ap;
	va_start(format, ap);
	
	int ret = evMsg(format, ap);
	
	va_end(ap);
	
	return ret;
}
int evMsg(const char * restrict format, va_list ap)
{
	
}
