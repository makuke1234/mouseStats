#include "../gui/trayIcon.h"
#include "../resources/resource.h"

static HMENU s_ti_createMenu()
{
	HMENU hmenu = CreatePopupMenu();
	
	assert(hmenu != NULL);
	assert(hmenu != INVALID_HANDLE_VALUE);
	
	
	bool ret = AppendMenuW(hmenu, MF_STRING, IDM_SHOW, L"&Show/hide") &&
	           SetMenuDefaultItem(hmenu, 0, IDM_SHOW) &&
	           AppendMenuW(hmenu, MF_SEPARATOR, 0, NULL) &&
	           AppendMenuW(hmenu, MF_STRING, IDM_EXIT, L"&Close");
	
	if (!ret)
	{
		DestroyMenu(hmenu);
		return NULL;
	}
	
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
		.guidItem = { 0 },
		.hBalloonIcon = NULL
	};
	
	wcscpy_s(This->nid.szTip, ARRAYSIZE(This->nid.szTip), tipInfo);
	wcscpy_s(This->nid.szInfoTitle, ARRAYSIZE(This->nid.szInfoTitle), defaultInfoTitle);
	
	This->hPopupMenu = s_ti_createMenu();
	
	return (This->hPopupMenu != NULL) && 
	       Shell_NotifyIconW(NIM_ADD, &This->nid) &&
	       Shell_NotifyIconW(NIM_SETVERSION, &This->nid);
}
bool ti_destroy(ti_data_t * restrict This)
{
	assert(This != NULL);
	
	DestroyMenu(This->hPopupMenu);
	This->hPopupMenu = NULL;
	
	return Shell_NotifyIconW(NIM_DELETE, &This->nid);
}
bool ti_sendMessage(ti_data_t * restrict This, const wchar * msg, const wchar * title, ti_msgType_e msgType)
{
	assert(This != NULL);
	
	(void)((title != NULL) && wcscpy_s(This->nid.szInfoTitle, ARRAYSIZE(This->nid.szInfoTitle), title));
	wcscpy_s(This->nid.szInfo, ARRAYSIZE(This->nid.szInfo), msg != NULL ? msg : L"");
	
	DWORD icon = This->nid.dwInfoFlags & NIIF_USER;
	
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
	
	This->nid.dwInfoFlags ^= icon;
	const bool ret = Shell_NotifyIconW(NIM_MODIFY, &This->nid);
	This->nid.dwInfoFlags ^= icon;
	
	return ret;
}
bool ti_tipInfo(ti_data_t * restrict This, const wchar * restrict tipInfo)
{
	assert(This != NULL);
	
	wcscpy_s(This->nid.szTip, ARRAYSIZE(This->nid.szTip), tipInfo);
	
	return Shell_NotifyIconW(NIM_MODIFY, &This->nid);
}
bool ti_context(ti_data_t * restrict This, WPARAM wp, LPARAM lp)
{
	(void)wp;
	
	switch (LOWORD(lp))
	{
	case WM_LBUTTONUP:
		SendMessageW(This->nid.hWnd, WM_COMMAND, IDM_SHOW, 0);
		return true;
	case WM_RBUTTONUP:
	case WM_CONTEXTMENU:
		POINT p;
		return GetCursorPos(&p) &&
			   TrackPopupMenu(This->hPopupMenu, TPM_TOPALIGN | TPM_LEFTALIGN, p.x, p.y, 0, This->nid.hWnd, NULL);	
	}
	
	return false;
}
