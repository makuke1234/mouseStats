#ifndef WINAPI_H
#define WINAPI_H

#ifndef NOMINMAX
	#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN
#endif
#ifndef WIN32_EXTRA_LEAN
	#define WIN32_EXTRA_LEAN
#endif

#include <Windows.h>
#include <windowsx.h>
#include <CommCtrl.h>
#include <dwmapi.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>
#include <time.h>
#include <math.h>
#include <assert.h>

#include "comTypes.h"

#define msUNUSED(x) ((void)x)
#define FARPROC_cast(type, func) (type)func

#define msWS_WINDOWED         (WS_OVERLAPPEDWINDOW | WS_THICKFRAME | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX)
#define msWS_AERO_BORDERLESS  (WS_POPUP            | WS_THICKFRAME | WS_CAPTION | WS_SYSMENU | WS_MAXIMIZEBOX | WS_MINIMIZEBOX)
#define msWS_BASIC_BORDERLESS (WS_POPUP            | WS_THICKFRAME              | WS_SYSMENU | WS_MAXIMIZEBOX | WS_MINIMIZEBOX)

#define msWS_BORDERLESS (ms_isCompositionEnabled() ? msWS_AERO_BORDERLESS : msWS_BASIC_BORDERLESS)

bool ms_regClass(const wchar * restrict className, WNDPROC winProc);
bool ms_regClassBg(const wchar * restrict className, WNDPROC winProc, COLORREF rgbColor);
bool ms_isCompositionEnabled(void);
bool ms_isMaximized(HWND hwnd);
bool ms_isActive(HWND hwnd);

int ms_dpi(HWND hwnd);
int ms_hdpi(HWND hwnd, int size);
int ms_cdpi(int dpi, int size);
int ms_fdpi(int dpi, int size);
int ms_defcdpi(int size);
int ms_deffdpi(int size);


typedef UINT (WINAPI * pfnGetDpiForSystem_t)(void);
typedef UINT (WINAPI * pfnGetDpiForWindow_t)(HWND hwnd);
typedef int  (WINAPI * pfnDrawShadowText_t)(
	HDC hdc,
	LPCWSTR pszText,
	UINT cch,
	RECT * prc,
	DWORD dwFlags,
	COLORREF crText,
	COLORREF crShadow,
	int ixOffset,
	int iyOffset
);

bool ms_initSymbols(void);

UINT ms_GetDpiForWindow(HWND hwnd);
UINT ms_GetDpiForSystem(void);
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
);


#endif
