#include "gui.h"
#include "app.h"

LRESULT CALLBACK mgui_winProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
	msdata_t * restrict This = NULL;

	if (msg == WM_NCCREATE)
	{
		This = ((CREATESTRUCTW *)lp)->lpCreateParams;
		assert(This != NULL);

		SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)This);
		This->hwnd = hwnd;
	}
	else
	{
		This = (msdata_t *)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
	}

	if (This == NULL)
	{
		return DefWindowProcW(hwnd, msg, wp, lp);
	}

	bool def = false;

	switch (msg)
	{
	case WM_SYSCOMMAND:
		if (!mgui_handleSystemMenu(hwnd, wp))
		{
			def = true;
		}
		break;
	case WM_COMMAND:
		mgui_handleCommand(hwnd, wp);
		break;
	case WM_RBUTTONUP:
	case WM_CONTEXTMENU:
		/* Allow window dragging from any point */
		mgui_handleContextMenu(hwnd, lp);
		break;
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hwnd, &ps);

		// Draw draggable titlebar
		mgui_calcTitleRect(hwnd, &This->titleRect, This->dpi);
		FillRect(hdc, &This->titleRect, This->isActive ? This->titleBrush : This->titleBrushInactive);

		wchar_t titleText[MAX_TITLETEXT];
		GetWindowTextW(hwnd, titleText, MAX_TITLETEXT);

		SelectObject(hdc, This->titleTextFont);
		SetBkMode(hdc, TRANSPARENT);

		ms_DrawShadowText(
			hdc,
			titleText, (UINT)wcslen(titleText),
			&This->titleRect,
			DT_SINGLELINE | DT_CENTER | DT_VCENTER,
			RGB(255, 255, 255), RGB(0, 0, 0),
			2, 2
		);
		
		EndPaint(hwnd, &ps);
		break;
	}
	case WM_ACTIVATE:
		This->isActive = (wp == WA_ACTIVE) || (wp == WA_CLICKACTIVE);
		mgui_calcTitleRect(hwnd, &This->titleRect, This->dpi);
		InvalidateRect(hwnd, &This->titleRect, FALSE);
		break;
	case WM_SIZE:
		InvalidateRect(hwnd, NULL, FALSE);
		break;
	case WM_NCCALCSIZE:
		if (wp == TRUE)
		{
			NCCALCSIZE_PARAMS * restrict params = (NCCALCSIZE_PARAMS *)lp;
			// Adjust maximized client rect
			mgui_adjustMaximizedClientRect(hwnd, &params->rgrc[0]);
			break;
		}
		def = true;
		break;
	case WM_NCHITTEST:
		return mgui_hitTest(hwnd, (POINT){ .x = GET_X_LPARAM(lp), .y = GET_Y_LPARAM(lp) }, &This->titleRect, This->resizeEnable);
	case WM_NCACTIVATE:
		if (!ms_compositionEnabled())
		{
			return 1;
		}
		def = true;
		break;
	case WM_CLOSE:
		DestroyWindow(hwnd);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_CREATE:
		This->dpi = (int)ms_dpi(hwnd);

		This->titleBrush = CreateSolidBrush(RGB(50, 127, 255));
		This->titleBrushInactive = CreateSolidBrush(RGB(192, 192, 192));
		This->titleTextFont = CreateFontW(
			-MulDiv(12, This->dpi, 72), 0, 0, 0,
			FW_MEDIUM, FALSE, FALSE, FALSE,
			ANSI_CHARSET,
			OUT_CHARACTER_PRECIS,
			CLIP_DEFAULT_PRECIS,
			DEFAULT_QUALITY,
			DEFAULT_PITCH,
			L"Segoe UI"
		);

		HMENU sysmenu = GetSystemMenu(hwnd, FALSE);
		if (sysmenu != NULL)
		{
			AppendMenuW(sysmenu, MF_STRING, IDM_EXIT, L"&nipi-tiri");
		}

		break;
	default:
		def = true;
	}

	if (def)
	{
		return DefWindowProcW(hwnd, msg, wp, lp);
	}

	return 0;
}

LRESULT mgui_hitTest(HWND hwnd, POINT cursor, RECT * restrict titleRect, bool borderless_resize)
{
	const POINT border = {
		.x = GetSystemMetrics(SM_CXFRAME) + GetSystemMetrics(SM_CXPADDEDBORDER),
		.y = GetSystemMetrics(SM_CYFRAME) + GetSystemMetrics(SM_CXPADDEDBORDER),
	};

	RECT window;
	if (!GetWindowRect(hwnd, &window))
	{
		return HTNOWHERE;
	}

	const LRESULT drag = (cursor.x >= (window.left + titleRect->left)) && (cursor.x <= (window.left + titleRect->right)) &&
						 (cursor.y >= (window.top  + titleRect->top))  && (cursor.y <= (window.top  + titleRect->bottom)) ?
						 HTCAPTION : HTCLIENT;


	enum regionMask
	{
		client = 0b0000,
        left   = 0b0001,
        right  = 0b0010,
        top    = 0b0100,
        bottom = 0b1000,
	};

	const int result =
        left    * (cursor.x < (window.left   + border.x)) |
        right   * (cursor.x >= (window.right  - border.x)) |
        top     * (cursor.y < (window.top    + border.y)) |
        bottom  * (cursor.y >= (window.bottom - border.y));

	switch (result)
	{
        case left          : return borderless_resize ? HTLEFT        : drag;
        case right         : return borderless_resize ? HTRIGHT       : drag;
        case top           : return borderless_resize ? HTTOP         : drag;
        case bottom        : return borderless_resize ? HTBOTTOM      : drag;
        case top | left    : return borderless_resize ? HTTOPLEFT     : drag;
        case top | right   : return borderless_resize ? HTTOPRIGHT    : drag;
        case bottom | left : return borderless_resize ? HTBOTTOMLEFT  : drag;
        case bottom | right: return borderless_resize ? HTBOTTOMRIGHT : drag;
        case client        : return drag;
        default            : return HTNOWHERE;
    }

}
void mgui_adjustMaximizedClientRect(HWND window, RECT * restrict rect)
{
	if (!ms_isMaximized(window))
	{
		return;
	}

	HMONITOR monitor = MonitorFromWindow(window, MONITOR_DEFAULTTONULL);
	if (monitor == NULL)
	{
		return;
	}

	MONITORINFO monitorInfo = { 0 };
	monitorInfo.cbSize = sizeof monitorInfo;

	if (!GetMonitorInfoW(monitor, &monitorInfo))
	{
		return;
	}

	*rect = monitorInfo.rcWork;
}
void mgui_calcTitleRect(HWND hwnd, RECT * restrict rect, int dpi)
{
	RECT cl;
	if (!GetClientRect(hwnd, &cl))
	{
		return;
	}

	rect->left   = cl.left;
	rect->right  = cl.right;
	rect->top    = cl.top;
	rect->bottom = rect->top + MulDiv(30, dpi, 96);
}

void mgui_handleContextMenu(HWND hwnd, LPARAM lp)
{
	HMENU hmenu = CreatePopupMenu();

	POINT p = { .x = GET_X_LPARAM(lp), .y = GET_Y_LPARAM(lp) };
	ClientToScreen(hwnd, &p);

	// Append to menu
	AppendMenuW(hmenu, MF_SEPARATOR, 0, NULL);
	AppendMenuW(hmenu, MF_STRING | MF_DEFAULT, IDM_EXIT, L"&Close");

	TrackPopupMenu(hmenu, TPM_RIGHTBUTTON, p.x, p.y, 0, hwnd, NULL);
	DestroyMenu(hmenu);
}
void mgui_handleCommand(HWND hwnd, WPARAM wp)
{
	switch (LOWORD(wp))
	{
	case IDM_EXIT:
		SendMessageW(hwnd, WM_CLOSE, 0, 0);
		break;
	}
}
bool mgui_handleSystemMenu(HWND hwnd, WPARAM wp)
{
	switch (wp)
	{
	case IDM_EXIT:
		SendMessageW(hwnd, WM_CLOSE, 0, 0);
		break;
	default:
		return false;
	}

	return true;
}
