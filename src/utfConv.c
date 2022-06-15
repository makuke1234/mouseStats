#include "utfConv.h"

char * toutf8(const wchar * restrict utf16)
{
	char * out = NULL;
	toutf8n(utf16, -1, &out);
	return out;
}
int toutf8n(const wchar * restrict utf16, int numChars, char ** restrict putf8)
{
	return toutf8_s(utf16, numChars, putf8, NULL);
}
int toutf8_s(const wchar * restrict utf16, int numChars, char ** restrict putf8, usize * restrict sz)
{
	assert(utf16 != NULL);
	assert(putf8 != NULL);
	const int size = (numChars == 0) ? 1 : WideCharToMultiByte(
		CP_UTF8,
		0,
		utf16,
		numChars,
		NULL,
		0,
		NULL,
		NULL
	);

	// Alloc mem
	if ((sz != NULL) && (*sz < (usize)size))
	{
		vptr mem = realloc(*putf8, (usize)size * sizeof(char));
		if (mem == NULL)
		{
			return 0;
		}
		*putf8 = mem;
		*sz    = (usize)size;
	}
	else if ((*putf8 == NULL) || (sz == NULL))
	{
		*putf8 = malloc((usize)size * sizeof(char));
		if (*putf8 == NULL)
		{
			return 0;
		}
	}

	// Convert
	if (numChars != 0)
	{
		WideCharToMultiByte(
			CP_UTF8,
			0,
			utf16,
			numChars,
			*putf8,
			(int)size,
			NULL,
			NULL
		);
	}
	else
	{
		(*putf8)[0] = '\0';
	}
	return size;
}

wchar * toutf16(const char * restrict utf8)
{
	wchar * out = NULL;
	toutf16n(utf8, -1, &out);
	return out;
}
int toutf16n(const char * restrict utf8, int numChars, wchar ** restrict putf16)
{
	return toutf16_s(utf8, numChars, putf16, NULL);
}
int toutf16_s(const char * restrict utf8, int numBytes, wchar ** restrict putf16, usize * restrict sz)
{
	assert(utf8   != NULL);
	assert(putf16 != NULL);

	// Query the needed size
	const int size = (numBytes == 0) ? 1 : MultiByteToWideChar(
		CP_UTF8,
		MB_PRECOMPOSED,
		utf8,
		numBytes,
		NULL,
		0
	);
	// Try to allocate memory
	if ((sz != NULL) && (*sz < (usize)size))
	{
		vptr mem = realloc(*putf16, (usize)size * sizeof(wchar));
		if (mem == NULL)
		{
			return 0;
		}
		*putf16 = mem;
		*sz     = (usize)size;
	}
	else if ((*putf16 == NULL) || (sz == NULL))
	{
		*putf16 = malloc((usize)size * sizeof(wchar));
		if (*putf16 == NULL)
		{
			return 0;
		}
	}

	if (numBytes != 0)
	{
		// Make conversion
		MultiByteToWideChar(
			CP_UTF8,
			MB_PRECOMPOSED,
			utf8,
			numBytes,
			*putf16,
			(int)size
		);
	}
	else
	{
		(*putf16)[0] = L'\0';
	}
	return size;
}
