#include "error.h"

int eCon(const char * restrict format, ...)
{
	assert(format != NULL);

	va_list ap;
	va_start(ap, format);

	int ret = evCon(format, ap);

	va_end(ap);

	return ret;
}
int evCon(const char * restrict format, va_list ap)
{
	return vfprintf(stderr, format, ap);
}

int eMsg(const char * restrict format, ...)
{
	assert(format != NULL);

	va_list ap;
	va_start(ap, format);

	int ret = evMsg(format, ap);

	va_end(ap);

	return ret;
}
int evMsg(const char * restrict format, va_list ap)
{
	char temparr[MAX_EMSG];
	int ch = vsprintf_s(temparr, MAX_EMSG, format, ap);
	MessageBoxA(NULL, temparr, MOUSE_STATS_TITLE_ASCII, MB_ICONERROR | MB_OK);
	return ch;
}
