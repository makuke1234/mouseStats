#include "mouseHook.h"

static HHOOK s_mmh_mouseHook = NULL;
static HWND s_mmh_hwnd = NULL;

LRESULT CALLBACK mmh_mouseHookProc(int nCode, WPARAM wp, LPARAM lp)
{
	if ((nCode >= 0) && SendMessageW(s_mmh_hwnd, MMH_MOUSEHOOK_MSG, wp, lp))
	{
		return 1;
	}
	return CallNextHookEx(s_mmh_mouseHook, nCode, wp, lp);
}
HHOOK mmh_setHook(HINSTANCE hInst, HWND hwnd)
{
	assert(s_mmh_mouseHook == NULL);

	s_mmh_mouseHook = SetWindowsHookExW(
		WH_MOUSE_LL,
		&mmh_mouseHookProc,
		hInst,
		0
	);

	if (s_mmh_mouseHook != NULL)
	{
		s_mmh_hwnd = hwnd;
	}

	return s_mmh_mouseHook;
}
void mmh_removeHook(void)
{
	assert(s_mmh_mouseHook != NULL);

	UnhookWindowsHookEx(s_mmh_mouseHook);
	s_mmh_mouseHook = NULL;
	s_mmh_hwnd      = NULL;
}
