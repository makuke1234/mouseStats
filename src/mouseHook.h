#ifndef MOUSE_HOOK_H
#define MOUSE_HOOK_H

#include "winapi.h"

#define mh_MOUSEHOOK_MSG WM_USER

LRESULT CALLBACK mh_mouseHookProc(int nCode, WPARAM wp, LPARAM lp);
HHOOK mh_setHook(HINSTANCE hInst, HWND hwnd);
void mh_removeHook(void);

typedef enum mh_type
{
	mh_lbdown,
	mh_lbup,
	mh_rbdown,
	mh_rbup,
	mh_move,
	mh_wheel,
	mh_hwheel,

	mh_num_types

} mh_type_e;

typedef struct mh_data
{
	mh_type_e eventType;
	POINT cursorPos;
	int16_t wheelDelta, hwheelDelta;
	DWORD timeStamp;

} mh_data_t;

bool mh_decode(mh_data_t * restrict ptr, WPARAM wp, LPARAM lp);
const char * mh_eventName(mh_type_e type);

#endif
