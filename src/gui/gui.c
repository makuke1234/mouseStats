#include "gui.h"
#include "../core/app.h"
#include "../core/mouseHook.h"

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
	case WM_MOUSEHOOK_MSG:
	{
		mh_data_t data;

		if (mh_decode(&data, wp, lp))
		{
			// Print all information
			/*printf(
				"Event: %s; Pos: %ld, %ld; Wheel: %hd; Hwheel: %hd; Time: %ld\n",
				mh_eventName(data.eventType),
				data.cursorPos.x,
				data.cursorPos.y,
				data.wheelDelta,
				data.hwheelDelta,
				data.timeStamp
			);*/

			// Save information to disk
			if (!mh_recs_add(&This->mouseData, &data))
			{
				ePrinti(eDataRecord);
			}
		}

		break;
	}
	case WM_TRAYICON_MSG:
		ti_context(&This->tidata, wp, lp);
		break;
	case WM_SYSCOMMAND:
		if (!mgui_handleSystemMenu(hwnd, wp))
		{
			def = true;
		}
		break;
	case WM_COMMAND:
		mgui_handleCommand(This, hwnd, wp);
		break;
	case WM_NCRBUTTONDOWN:
		This->rclick = true;
		break;
	case WM_RBUTTONUP:
		This->rclick = false;
		/* fall through */
	case WM_CONTEXTMENU:
	{
		RECT window;
		POINT cursor;
		if (!GetWindowRect(hwnd, &window) || !GetCursorPos(&cursor) ||
			!mgui_captionHit(&window, cursor, &This->titleRect))
		{
			mgui_handleContextMenu(hwnd, This->contextMenu, lp, false);
		}
		else
		{
			mgui_handleContextMenu(hwnd, This->sysMenu, lp, true);
		}

		break;
	}
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hwnd, &ps);

		// Draw draggable titlebar
		mgui_calcTitleRect(hwnd, &This->titleRect, &This->yBegin, This->dpi);
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
		mgui_calcTitleRect(hwnd, &This->titleRect, &This->yBegin, This->dpi);
		InvalidateRect(hwnd, &This->titleRect, FALSE);
		break;
	case WM_SIZE:
		{
			// Move button
			RECT rc;
			GetClientRect(hwnd, &rc);
			SetWindowPos(This->closeBtn, NULL, rc.right - ms_defcdpi(MS_TITLEBTN_SIZE),     0, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER);
			SetWindowPos(This->minBtn,   NULL, rc.right - ms_defcdpi(2 * MS_TITLEBTN_SIZE), 0, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER);

		}
		SetWindowPos(This->testBtn, NULL, ms_defcdpi(MS_MARGIN), This->yBegin + ms_defcdpi(MS_MARGIN), 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER);

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
		//InvalidateRect(This->closeBtn, NULL, FALSE);
		return mgui_hitTest(
			hwnd,
			(POINT){ .x = GET_X_LPARAM(lp), .y = GET_Y_LPARAM(lp) },
			&This->titleRect,
			This->resizeEnable,
			This->rclick
		);
	case WM_NCACTIVATE:
		if (!ms_isCompositionEnabled())
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
		This->dpi = ms_dpi(hwnd);

		This->titleBrush = CreateSolidBrush(MS_TITLE_COLOR);
		This->titleBrushInactive = CreateSolidBrush(MS_TITLE_INACTIVE_COLOR);
		This->titleTextFont = CreateFontW(
			ms_deffdpi(MS_TITLEFONT_SIZEPT), 0, 0, 0,
			FW_MEDIUM, FALSE, FALSE, FALSE,
			DEFAULT_CHARSET,
			OUT_CHARACTER_PRECIS,
			CLIP_DEFAULT_PRECIS,
			DEFAULT_QUALITY,
			DEFAULT_PITCH,
			L"Segoe UI"
		);
		This->titleBtnFont = CreateFontW(
			ms_deffdpi(MS_TITLEFONT_SIZEPT), 0, 0, 0,
			FW_NORMAL, FALSE, FALSE, FALSE,
			DEFAULT_CHARSET,
			OUT_CHARACTER_PRECIS,
			CLIP_DEFAULT_PRECIS,
			DEFAULT_QUALITY,
			DEFAULT_PITCH,
			L"Marlett"
		);
		This->btnFont = CreateFontW(
			ms_deffdpi(MS_FONT_SIZEPT), 0, 0, 0,
			FW_MEDIUM, FALSE, FALSE, FALSE,
			DEFAULT_CHARSET,
			OUT_CHARACTER_PRECIS,
			CLIP_DEFAULT_PRECIS,
			DEFAULT_QUALITY,
			DEFAULT_PITCH,
			L"Segoe UI"
		);
		This->normFont = CreateFontW(
			ms_deffdpi(MS_FONT_SIZEPT), 0, 0, 0,
			FW_NORMAL, FALSE, FALSE, FALSE,
			DEFAULT_CHARSET,
			OUT_CHARACTER_PRECIS,
			CLIP_DEFAULT_PRECIS,
			DEFAULT_QUALITY,
			DEFAULT_PITCH,
			L"Segoe UI"
		);

		This->sysMenu = GetSystemMenu(hwnd, FALSE);
		if (This->sysMenu != NULL)
		{
			AppendMenuW(This->sysMenu, MF_STRING, IDM_EXIT, L"&nipi-tiri");
		}

		This->contextMenu = CreatePopupMenu();
		if (This->contextMenu != NULL)
		{
			AppendMenuW(This->contextMenu, MF_SEPARATOR, 0, NULL);
			AppendMenuW(This->contextMenu, MF_STRING | MF_DEFAULT, IDM_EXIT, L"&Close");
		}

		const int sz = ms_defcdpi(MS_TITLEBTN_SIZE);
		RECT rc;
		GetClientRect(hwnd, &rc);
		This->closeBtn = mgui_btnCreate(
			This->dpi,
			0,
			L"\x72",
			WS_CHILD | WS_VISIBLE,
			rc.right - sz, 0,
			sz, ms_defcdpi(MS_TITLE_HEIGHT),
			hwnd,
			(HMENU)IDM_EXIT,
			(HINSTANCE)GetWindowLongPtrW(hwnd, GWLP_HINSTANCE),
			MS_CLOSEBTN_COLOR,
			This->titleBtnFont,
			false
		);
		This->minBtn = mgui_btnCreate(
			This->dpi,
			0,
			L"\x30",
			WS_CHILD | WS_VISIBLE,
			rc.right - 2 * sz, 0,
			sz, ms_defcdpi(MS_TITLE_HEIGHT),
			hwnd,
			(HMENU)IDM_MIN,
			(HINSTANCE)GetWindowLongPtrW(hwnd, GWLP_HINSTANCE),
			MS_MINBTN_COLOR,
			This->titleBtnFont,
			false
		);
		This->testBtn = mgui_btnCreate(
			This->dpi,
			0,
			L"Activate",
			WS_CHILD | WS_VISIBLE | WS_TABSTOP,
			ms_defcdpi(MS_MARGIN), This->yBegin + ms_defcdpi(MS_MARGIN),
			ms_defcdpi(MS_BTNW), ms_defcdpi(MS_BTNH),
			hwnd,
			(HMENU)IDM_TEST,
			(HINSTANCE)GetWindowLongPtrW(hwnd, GWLP_HINSTANCE),
			MS_CALCBTN_COLOR(MS_DEFAULT_COLOR),
			This->btnFont,
			false
		);

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

LRESULT mgui_hitTest(HWND hwnd, POINT cursor, const RECT * restrict titleRect, bool borderless_resize, bool rclick)
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

	const LRESULT drag = mgui_captionHit(&window, cursor, titleRect) ? (rclick ? HTCLIENT : HTCAPTION) : HTCLIENT;


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
bool mgui_captionHit(const RECT * restrict window, POINT cursor, const RECT * restrict titleRect)
{
	return (cursor.x >= (window->left + titleRect->left)) && (cursor.x <= (window->left + titleRect->right)) &&
		   (cursor.y >= (window->top  + titleRect->top))  && (cursor.y <= (window->top  + titleRect->bottom));
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
void mgui_calcTitleRect(HWND hwnd, RECT * restrict rect, int * restrict yBegin, int dpi)
{
	RECT cl;
	if (!GetClientRect(hwnd, &cl))
	{
		return;
	}

	rect->left   = cl.left;
	rect->right  = cl.right;
	rect->top    = cl.top;
	rect->bottom = rect->top + ms_cdpi(dpi, MS_TITLE_HEIGHT);
	*yBegin      = rect->bottom;
}

void mgui_handleContextMenu(HWND hwnd, HMENU hmenu, LPARAM lp, bool sysmenu)
{
	POINT p = { .x = GET_X_LPARAM(lp), .y = GET_Y_LPARAM(lp) };
	ClientToScreen(hwnd, &p);

	// Append to menu

	const int cmd = TrackPopupMenu(hmenu, TPM_RIGHTBUTTON | (sysmenu ? TPM_RETURNCMD : 0), p.x, p.y, 0, hwnd, NULL);
	if (sysmenu && cmd)
	{
		SendMessageW(hwnd, WM_SYSCOMMAND, (WPARAM)cmd, 0);
	}
}
void mgui_handleCommand(msdata_t * restrict This, HWND hwnd, WPARAM wp)
{
	switch (LOWORD(wp))
	{
	case IDM_TEST:
		if (!ace_cmdAsync(&This->asyncCmd, acmd_test, This, NULL))
		{
			ePrinti(eAsyncCmd);
		}
		break;
	case IDM_MIN:
		ShowWindow(hwnd, SW_MINIMIZE);
		break;
	case IDM_EXIT:
		SendMessageW(hwnd, WM_CLOSE, 0, 0);
		break;
	case IDM_SHOW:
		if (IsWindowVisible(hwnd))
		{
			ShowWindow(hwnd, SW_HIDE);
		}
		else
		{
			ShowWindow(hwnd, SW_SHOW);
			SetForegroundWindow(hwnd);
		}
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

#define DELOBJ_COND(obj) ((obj != NULL) ? DeleteObject(obj) : (0))

static inline void mgui_btnCreateCleanup(
	mgui_btnBmps_t * bmps,
	HBRUSH nb,
	HBRUSH pb,
	HBRUSH hb,
	HDC dc,
	HBITMAP hbn,
	HBITMAP hbp,
	HBITMAP hbh
)
{
	free(bmps);
	DELOBJ_COND(nb);
	DELOBJ_COND(pb);
	DELOBJ_COND(hb);

	if (dc != NULL)
	{
		DeleteDC(dc);
	}

	DELOBJ_COND(hbn);
	DELOBJ_COND(hbp);
	DELOBJ_COND(hbh);
}
#undef DELOBJ_COND

HWND mgui_btnCreate(
	int dpi,
	DWORD dwExStyle,
	LPCWSTR name,
	DWORD dwStyle,
	int x, int y,
	int width, int height,
	HWND parent,
	HMENU hmenu,
	HINSTANCE hInstance,
	COLORREF color,
	HFONT font,
	bool border
)
{
	mgui_btnBmps_t * bmps = malloc(sizeof(mgui_btnBmps_t));
	if (bmps == NULL)
	{
		return NULL;
	}
	bmps->font           = font;
	bmps->tracking       = false;
	bmps->hover          = false;
	bmps->press          = false;
	bmps->pressTextColor = border ? RGB(0, 0, 0) : MS_BTN_PRESSCOLOR;

	// Create button brushes
	HBRUSH normBrush  = CreateSolidBrush(color);
	const uint32_t r  = GetRValue(color), g = GetGValue(color), b = GetBValue(color);
	HBRUSH pressBrush = CreateSolidBrush(border ? MS_BTN_PRESSCOLOR : MS_CALCBTN_PRESS_COLOR(r, g, b));
	HBRUSH highBrush  = CreateSolidBrush(MS_CALCBTN_HIGH_COLOR(r, g, b));

	const int margin      = border ? ms_cdpi(dpi, 2) : 0;
	const RECT borderRect = { .left = 0,      .right = width,          .top = 0,      .bottom = height };
	const RECT btnRect    = { .left = margin, .right = width - margin, .top = margin, .bottom = height - margin };


	if ((normBrush == NULL) || (pressBrush == NULL) || (highBrush == NULL))
	{
		mgui_btnCreateCleanup(bmps, normBrush, pressBrush, highBrush, NULL, NULL, NULL, NULL);
		return NULL;
	}

	// Padded line length calculation
	HDC dc = CreateCompatibleDC(NULL);
	if (dc == NULL)
	{
		mgui_btnCreateCleanup(bmps, normBrush, pressBrush, highBrush, NULL, NULL, NULL, NULL);
		return NULL;
	}
	bmps->hbmNormal = CreateBitmap(width, height, 1, 32, NULL);
	if (bmps->hbmNormal == NULL)
	{
		mgui_btnCreateCleanup(bmps, normBrush, pressBrush, highBrush, dc, NULL, NULL, NULL);
		return NULL;
	}
	HGDIOBJ oldbmp = SelectObject(dc, bmps->hbmNormal);

	// Draw normal button
	if (border)
	{
		FillRect(dc, &borderRect, GetStockObject(WHITE_BRUSH));
	}
	FillRect(dc, &btnRect, normBrush);


	bmps->hbmPressed = CreateBitmap(width, height, 1, 32, NULL);
	if (bmps->hbmPressed == NULL)
	{
		mgui_btnCreateCleanup(bmps, normBrush, pressBrush, highBrush, dc, bmps->hbmNormal, NULL, NULL);
		return NULL;
	}
	SelectObject(dc, bmps->hbmPressed);

	// Draw pressed button
	if (border)
	{
		FillRect(dc, &borderRect, GetStockObject(WHITE_BRUSH));
	}
	FillRect(dc, &btnRect, pressBrush);


	bmps->hbmHighlight = CreateBitmap(width, height, 1, 32, NULL);
	if (bmps->hbmHighlight == NULL)
	{
		mgui_btnCreateCleanup(bmps, normBrush, pressBrush, highBrush, dc, bmps->hbmNormal, bmps->hbmPressed, NULL);
		return NULL;
	}
	SelectObject(dc, bmps->hbmHighlight);

	// Draw highlighted button
	if (border)
	{
		FillRect(dc, &borderRect, GetStockObject(WHITE_BRUSH));
	}
	FillRect(dc, &btnRect, highBrush);


	bmps->btnRect = btnRect;


	SelectObject(dc, oldbmp);
	mgui_btnCreateCleanup(NULL, normBrush, pressBrush, highBrush, dc, NULL, NULL, NULL);


	HWND hbtn = CreateWindowExW(
		dwExStyle,
		L"button",
		name,
		BS_OWNERDRAW | dwStyle,
		x, y,
		width, height,
		parent,
		hmenu,
		hInstance,
		NULL
	);

	SetWindowSubclass(hbtn, &mgui_btnOwnerDrawProc, 1, (DWORD_PTR)bmps);
	return hbtn;
}


LRESULT CALLBACK mgui_btnOwnerDrawProc(
	HWND hwnd, UINT umsg, WPARAM wp, LPARAM lp,
	UINT_PTR uidsubclass, DWORD_PTR dwRefData
)
{
	msUNUSED(uidsubclass);

	mgui_btnBmps_t * restrict hbmps = (mgui_btnBmps_t *)dwRefData;

	BITMAP bitmap01;
	HDC hdcmem01;
	HGDIOBJ oldbitmap01;

	switch (umsg)
	{
	case WM_LBUTTONDOWN:
		hbmps->press = true;
		InvalidateRect(hwnd, NULL, FALSE);
		break;
	case WM_LBUTTONUP:
		hbmps->press = false;
		InvalidateRect(hwnd, NULL, FALSE);
		break;
	case WM_MOUSEMOVE:
		if (!hbmps->tracking)
		{
			TRACKMOUSEEVENT tme = { 0 };
			tme.cbSize = sizeof tme;
			tme.dwFlags = TME_HOVER | TME_LEAVE;
			tme.dwHoverTime = 1;
			tme.hwndTrack = hwnd;
			TrackMouseEvent(&tme);
			hbmps->tracking = true;
		}
		if (hbmps->press)
		{
			InvalidateRect(hwnd, NULL, FALSE);
		}
		break;
	case WM_MOUSEHOVER:
		hbmps->hover = true;
		InvalidateRect(hwnd, NULL, FALSE);
		break;
	case WM_MOUSELEAVE:
		hbmps->hover = false;
		hbmps->press = false;
		InvalidateRect(hwnd, NULL, FALSE);
		hbmps->tracking = false;
		break;
	case WM_GETDLGCODE:
		switch (wp)
		{
		case VK_TAB:
			break;
		case VK_RETURN:
			SendMessageW(GetParent(hwnd), WM_COMMAND, (WPARAM)GetMenu(hwnd), 0);
			break;
		case VK_SPACE:
			hbmps->press = true;
			break;
		}
		break;
	case WM_KEYUP:
		if (wp == VK_SPACE)
		{
			hbmps->press = false;
		}
		break;
	case WM_KILLFOCUS:
		InvalidateRect(hwnd, NULL, FALSE);
		break;
	case WM_PAINT:
	{
		// Detect mouse position
		POINT p;
		GetCursorPos(&p);
		ScreenToClient(hwnd, &p);

		RECT rc;
		GetClientRect(hwnd, &rc);

		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hwnd, &ps);
		hdcmem01 = CreateCompatibleDC(hdc);


		COLORREF tCol = MS_BTN_PRESSCOLOR;
		if (hbmps->press || (hbmps->hover && (p.x >= rc.left) && (p.x <= rc.right) && (p.y >= rc.top) && (p.y <= rc.bottom)))
		{
			tCol = hbmps->press ? hbmps->pressTextColor : tCol;
			oldbitmap01 = SelectObject(hdcmem01, hbmps->press ? hbmps->hbmPressed : hbmps->hbmHighlight);
		}
		else
		{
			oldbitmap01 = SelectObject(hdcmem01, hbmps->hbmNormal);
		}

		RECT cr;
		GetClientRect(hwnd, &cr);

		wchar_t txt[MAX_PATH];
		GetWindowTextW(hwnd, txt, MAX_PATH);
		SetBkMode(hdcmem01, TRANSPARENT);
		SetTextColor(hdcmem01, tCol);
		if (hbmps->font != NULL)
		{
			SelectObject(hdcmem01, hbmps->font);
		}
		RECT textrect;
		DrawTextW(hdcmem01, txt, -1, &textrect, DT_CALCRECT);
		cr.top = cr.top + ((cr.bottom - cr.top) - (textrect.bottom - textrect.top)) / 2;
		DrawTextW(hdcmem01, txt, -1, &cr, DT_CENTER);

		GetObjectW(hbmps->hbmNormal, sizeof bitmap01, &bitmap01);
		BitBlt(hdc, 0, 0, bitmap01.bmWidth, bitmap01.bmHeight, hdcmem01, 0, 0, SRCCOPY);

		SelectObject(hdcmem01, oldbitmap01);
		DeleteDC(hdcmem01);
		EndPaint(hwnd, &ps);

		break;
	}
	case WM_NCDESTROY:
		RemoveWindowSubclass(hwnd, &mgui_btnOwnerDrawProc, 1);

		// Destroy bitmaps
		DeleteObject(hbmps->hbmNormal);
		DeleteObject(hbmps->hbmPressed);
		DeleteObject(hbmps->hbmHighlight);
		free(hbmps);

		break;
	}
	return DefSubclassProc(hwnd, umsg, wp, lp);
}
