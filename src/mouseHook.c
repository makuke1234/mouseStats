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

bool mmh_decode(mmh_data_t * restrict ptr, WPARAM wp, LPARAM lp)
{
	assert(ptr != NULL);

	// Determine message type
	switch (wp)
	{
	case WM_LBUTTONDOWN:
		ptr->eventType = mmh_lbdown;
		break;
	case WM_LBUTTONUP:
		ptr->eventType = mmh_lbup;
		break;
	case WM_RBUTTONDOWN:
		ptr->eventType = mmh_rbdown;
		break;
	case WM_RBUTTONUP:
		ptr->eventType = mmh_rbup;
		break;
	case WM_MOUSEMOVE:
		ptr->eventType = mmh_move;
		break;
	case WM_MOUSEWHEEL:
		ptr->eventType = mmh_wheel;
		break;
	case WM_MOUSEHWHEEL:
		ptr->eventType = mmh_hwheel;
		break;
	}
	
	const MSLLHOOKSTRUCT * restrict hstruct = (MSLLHOOKSTRUCT *)lp;
	if (hstruct == NULL)
	{
		return false;
	}
	ptr->cursorPos = hstruct->pt;
	
	if (ptr->eventType == mmh_wheel)
	{
		ptr->wheelDelta  = (int16_t)HIWORD(hstruct->mouseData);
		ptr->hwheelDelta = 0;
	}
	else if (ptr->eventType == mmh_hwheel)
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
const char * mmh_eventName(mmh_type_e type)
{
	static const char * eventNames[mmh_num_types] = {
		"lbdown",
		"lbup",
		"rbdown",
		"rbup",
		"move",
		"wheel",
		"hwheel"
	};

	assert(type < mmh_num_types);

	return eventNames[type];
}
