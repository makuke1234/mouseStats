#include "mouseHook.h"

static HHOOK s_mh_mouseHook = NULL;
static HWND s_mh_hwnd = NULL;

LRESULT CALLBACK mh_mouseHookProc(int nCode, WPARAM wp, LPARAM lp)
{
	if ((nCode >= 0) && SendMessageW(s_mh_hwnd, mh_MOUSEHOOK_MSG, wp, lp))
	{
		return 1;
	}
	return CallNextHookEx(s_mh_mouseHook, nCode, wp, lp);
}
HHOOK mh_setHook(HINSTANCE hInst, HWND hwnd)
{
	assert(s_mh_mouseHook == NULL);

	s_mh_mouseHook = SetWindowsHookExW(
		WH_MOUSE_LL,
		&mh_mouseHookProc,
		hInst,
		0
	);

	if (s_mh_mouseHook != NULL)
	{
		s_mh_hwnd = hwnd;
	}

	return s_mh_mouseHook;
}
void mh_removeHook(void)
{
	assert(s_mh_mouseHook != NULL);

	UnhookWindowsHookEx(s_mh_mouseHook);
	s_mh_mouseHook = NULL;
	s_mh_hwnd      = NULL;
}

bool mh_decode(mh_data_t * restrict ptr, WPARAM wp, LPARAM lp)
{
	assert(ptr != NULL);

	// Determine message type
	switch (wp)
	{
	case WM_LBUTTONDOWN:
		ptr->eventType = mh_lbdown;
		break;
	case WM_LBUTTONUP:
		ptr->eventType = mh_lbup;
		break;
	case WM_RBUTTONDOWN:
		ptr->eventType = mh_rbdown;
		break;
	case WM_RBUTTONUP:
		ptr->eventType = mh_rbup;
		break;
	case WM_MOUSEMOVE:
		ptr->eventType = mh_move;
		break;
	case WM_MOUSEWHEEL:
		ptr->eventType = mh_wheel;
		break;
	case WM_MOUSEHWHEEL:
		ptr->eventType = mh_hwheel;
		break;
	}
	
	const MSLLHOOKSTRUCT * restrict hstruct = (MSLLHOOKSTRUCT *)lp;
	if (hstruct == NULL)
	{
		return false;
	}
	ptr->cursorPos = hstruct->pt;
	
	if (ptr->eventType == mh_wheel)
	{
		ptr->wheelDelta  = (int16_t)HIWORD(hstruct->mouseData);
		ptr->hwheelDelta = 0;
	}
	else if (ptr->eventType == mh_hwheel)
	{
		ptr->wheelDelta  = 0;
		ptr->hwheelDelta = (int16_t)HIWORD(hstruct->mouseData);
	}
	else
	{
		ptr->wheelDelta  = 0;
		ptr->hwheelDelta = 0;
	}

	ptr->timeStamp = hstruct->time;
	return true;
}
const char * mh_eventName(mh_type_e type)
{
	static const char * eventNames[mh_num_types] = {
		"lbdown",
		"lbup",
		"rbdown",
		"rbup",
		"move",
		"wheel",
		"hwheel"
	};

	assert(type < mh_num_types);

	return eventNames[type];
}
