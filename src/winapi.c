#include "winapi.h"

bool ms_regClass(const wchar_t * restrict className, WNDPROC winProc)
{
	return ms_regClassBg(className, winProc, GetSysColor(COLOR_3DFACE));
}
bool ms_regClassBg(const wchar_t * restrict className, WNDPROC winProc, COLORREF rgbColor)
{
	WNDCLASSEXW wc   = { 0 };
	wc.cbSize        = sizeof wc;
	wc.lpfnWndProc   = winProc;
	wc.hInstance     = GetModuleHandleW(NULL);
	wc.hIcon         = LoadIconW(wc.hInstance, MAKEINTRESOURCEW(IDI_APPLICATION));
	wc.hCursor       = LoadCursorW(wc.hInstance, MAKEINTRESOURCEW(IDC_ARROW));
	if (wc.hCursor == NULL)
	{
		wc.hCursor = LoadCursorW(NULL, MAKEINTRESOURCEW(IDC_ARROW));
	}
	wc.hbrBackground = CreateSolidBrush(rgbColor);
	wc.lpszMenuName  = NULL;
	wc.lpszClassName = className;
	wc.hIconSm       = wc.hIcon;

	return RegisterClassExW(&wc) != 0;
}
bool ms_compositionEnabled(void)
{
	BOOL compEnabled = FALSE;
	bool result = (DwmIsCompositionEnabled(&compEnabled) == S_OK);
	return result && compEnabled;
}
bool ms_isMaximized(HWND hwnd)
{
	WINDOWPLACEMENT placement;
	if (!GetWindowPlacement(hwnd, &placement))
	{
		return false;
	}
	
	return placement.showCmd == SW_MAXIMIZE;
}
bool ms_isActive(HWND hwnd)
{
	return GetForegroundWindow() == hwnd;
}
UINT ms_dpi(HWND hwnd)
{
	if (hwnd != NULL)
	{
		UINT dpi = ms_GetDpiForWindow(hwnd);
		if (dpi)
		{
			return dpi;
		}
	}
	else
	{
		UINT dpi = ms_GetDpiForSystem();
		if (dpi)
		{
			return dpi;
		}
	}

	HDC hdc = GetDC(hwnd);
	if (hdc != NULL)
	{
		UINT dpi = (UINT)GetDeviceCaps(hdc, LOGPIXELSX);
		ReleaseDC(hwnd, hdc);
		return dpi;
	}
	else
	{
		return USER_DEFAULT_SCREEN_DPI;
	}

}

static pfnGetDpiForSystem_t pfnGetDpiForSystem = NULL;
static pfnGetDpiForWindow_t pfnGetDpiForWindow = NULL;
static pfnDrawShadowText_t  pfnDrawShadowText  = NULL;

bool ms_initSymbols(void)
{
	static bool s_init = false, s_success = false;
	
	if (s_init)
	{
		return s_success;
	}

	HMODULE user32 = GetModuleHandleW(L"user32");

	pfnGetDpiForSystem = (pfnGetDpiForSystem_t)GetProcAddress(user32, "GetDpiForSystem");
	pfnGetDpiForWindow = (pfnGetDpiForWindow_t)GetProcAddress(user32, "GetDpiForWindow");

	HMODULE comctl32 = GetModuleHandleW(L"Comctl32");

	pfnDrawShadowText = (pfnDrawShadowText_t)GetProcAddress(comctl32, "DrawShadowText");

	s_init    = true;
	s_success = (pfnGetDpiForSystem != NULL) && (pfnGetDpiForWindow != NULL) && (pfnDrawShadowText != NULL);

	return s_success;
}

UINT ms_GetDpiForWindow(HWND hwnd)
{
	ms_initSymbols();
	return (pfnGetDpiForWindow != NULL) ? pfnGetDpiForWindow(hwnd) : 0;
}
UINT ms_GetDpiForSystem(void)
{
	ms_initSymbols();
	return (pfnGetDpiForSystem != NULL) ? pfnGetDpiForSystem() : 0;
}
int ms_DrawShadowText(
	HDC hdc,
	LPCWSTR pszText,
	UINT cch,
	RECT * prc,
	DWORD dwFlags,
	COLORREF crText,
	COLORREF crShadow,
	int ixOffset,
	int iyOffset
)
{
	ms_initSymbols();
	return (pfnDrawShadowText != NULL) ? pfnDrawShadowText(
		hdc,
		pszText,
		cch,
		prc,
		dwFlags,
		crText,
		crShadow,
		ixOffset,
		iyOffset
	) : 0;
}

