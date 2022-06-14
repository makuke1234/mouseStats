#ifndef MOUSE_HOOK_H
#define MOUSE_HOOK_H

#include "winapi.h"

LRESULT CALLBACK llmh_mouseHookProc(int nCode, WPARAM wp, LPARAM lp);
HHOOK llmh_setHook();
void llmh_removeHook();

#endif
