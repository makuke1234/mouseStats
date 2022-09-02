#include "trayIcon.h"

static HMENU s_ti_createMenu()
{
	HMENU hmenu = CreateMenu();
	
	assert(hmenu != NULL);
	assert(hmenu != INVALID_HANDLE_VALUE);
	
	return hmenu;
}

bool ti_create(ti_data_t * restrict This, const wchar * tipInfo, const wchar * defaultInfoTitle, HWND parent, HICON hIcon)
{
	assert(This != NULL);
	assert(tipInfo != NULL);
	assert(defaultInfoTitle != NULL);
	assert(parent != NULL);
	assert(parent != INVALID_HANDLE_VALUE);
	assert(hIcon != NULL);
	assert(hIcon != INVALID_HANDLE_VALUE);
	
	This->nid = (NOTIFYICONDATAW){
		.cbSize = sizeof(NOTIFYICONDATAW),
		.hWnd = parent,
		.uID = TRAYICON_ID,
		.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP | NIF_SHOWTIP | NIF_INFO,
		.uCallbackMessage = WM_TRAYICON_MSG,
		.hIcon = hIcon,
		.dwState = 0,
		.dwStateMask = 0,
		.uVersion = NOTIFYICON_VERSION_4,
		.dwInfoFlags = NIIF_NONE | NIIF_RESPECT_QUIET_TIME | NIIF_USER | NIIF_LARGE_ICON,
		.guidItem = 0,
		.hBalloonIcon = NULL
	};
	
	wcscpy_s(This->nid.szTip, ARRAYSIZE(This->nid.szTip), tipInfo);
	wcscpy_s(This->nid.szInfoTitle, ARRAYSIZE(This->nid.szInfoTitle), defaultInfoTitle);
	
	This->hPopupMenu = s_ti_createMenu();
	
	return Shell_NotifyIconW(NIM_ADD, &This->nid) &&
	       Shell_NotifyIconW(NIM_SETVERSION, &This->nid);
}
bool ti_destroy(ti_data_t * restrict This)
{
	assert(This != NULL);
	
	DeleteObject(This->hPopupMenu);
	This->hPopupMenu = NULL;
	
	return Shell_NotifyIconW(NIM_DELETE, &This->nid);
}
bool ti_sendMessage(ti_data_t * restrict This, const wchar * msg, const wchar * title, ti_msgType_e msgType)
{
	assert(This != NULL);
	
	(title != NULL) && wcscpy_s(This->nid.szInfoTitle, ARRAYSIZE(This->szInfoTitle), title);
	wcscpy_s(This->nid.szInfo, ARRAYSIZE(This->nid.szInfo), msg != NULL ? msg : L"");
	
	DWORD icon = This->dwInfoFlags & NIIF_USER;
	
	switch (msgType)
	{
	case ti_default:
		icon ^= NIIF_USER;
		break;
	case ti_none:
		icon |= NIIF_NONE;
		break;
	case ti_error:
		icon |= NIIF_ERROR;
		break;
	case ti_warn:
		icon |= NIIF_WARNING;
		break;
	case ti_info:
		icon |= NIIF_INFO;
		break;
	}
	
	This->dwInfoFlags ^= icon;
	const bool ret = Shell_NotifyIconW(NIM_MODIFY, nid);
	This->dwInfoFlags ^= icon;
	
	return ret;
}
bool ti_tipInfo(ti_data_t * restrict This, const wchar * restrict tipInfo)
{
	assert(This != NULL);
	
	wcscpy_s(This->nid.szTip, ARRAYSIZE(This->nid.szTip), tipInfo);
	
	return Shell_NotifyIconW(NIM_MODIFY, &This->nid);
}
bool ti_context(ti_data_t * restrict This)
{
	POINT p;
	return GetCursorPos(&p) &&
	       TrackPopupMenu(This->hPopupMenu, TPM_TOPALIGN | TPM_LEFTALIGN, p.x, p.y, 0, This->nid.hWnd, NULL);
}
