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
