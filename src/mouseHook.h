#ifndef MOUSE_HOOK_H
#define MOUSE_HOOK_H

#include "winapi.h"

#define MMH_MOUSEHOOK_MSG WM_USER

LRESULT CALLBACK mmh_mouseHookProc(int nCode, WPARAM wp, LPARAM lp);
HHOOK mmh_setHook(HINSTANCE hInst, HWND hwnd);
void mmh_removeHook(void);

#endif
