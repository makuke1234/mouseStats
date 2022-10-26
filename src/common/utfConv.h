#ifndef UTF_CONV_H
#define UTF_CONV_H

#include "winapi.h"
#include "comTypes.h"

char * toutf8(const wchar * restrict str);
int toutf8n(const wchar * restrict str, int len, char ** restrict pout);
int toutf8_s(const wchar * restrict str, int len, char ** restrict pout, usize * restrict psizeBytes);

wchar * toutf16(const char * restrict str);
int toutf16n(const char * restrict str, int len, wchar ** restrict pout);
int toutf16_s(const char * restrict str, int len, wchar ** restrict pout, usize * restrict psizeBytes);


#endif
