#ifndef MOUSE_HOOK_H
#define MOUSE_HOOK_H

#include "winapi.h"

#define MMH_MOUSEHOOK_MSG WM_USER

LRESULT CALLBACK mmh_mouseHookProc(int nCode, WPARAM wp, LPARAM lp);
HHOOK mmh_setHook(HINSTANCE hInst, HWND hwnd);
void mmh_removeHook(void);

typedef enum mmh_type
{
	mmh_lbdown,
	mmh_lbup,
	mmh_rbdown,
	mmh_rbup,
	mmh_move,
	mmh_wheel,
	mmh_hwheel,

	mmh_num_types

} mmh_type_e;

typedef struct mmh_data
{
	mmh_type_e eventType;
	POINT cursorPos;
	int16_t wheelDelta, hwheelDelta;
	DWORD timeStamp;

} mmh_data_t;

void mmh_decode(mmh_data_t * restrict ptr, WPARAM wp, LPARAM lp);
const char * mmh_eventName(mmh_type_e type);

#endif
