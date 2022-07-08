#ifndef GUI_H
#define GUI_H

#include "winapi.h"

#define MAX_TITLETEXT MAX_PATH

LRESULT CALLBACK mgui_winProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

LRESULT mgui_hitTest(HWND hwnd, POINT cursor, const RECT * restrict titleRect, bool borderless_resize, bool rclick);
bool mgui_captionHit(const RECT * restrict window, POINT cursor, const RECT * restrict titleRect);
void mgui_adjustMaximizedClientRect(HWND window, RECT * restrict rect);
void mgui_calcTitleRect(HWND hwnd, RECT * restrict rect, int dpi);

void mgui_handleContextMenu(HWND hwnd, HMENU hmenu, LPARAM lp, bool sysmenu);
void mgui_handleCommand(HWND hwnd, WPARAM wp);
bool mgui_handleSystemMenu(HWND hwnd, WPARAM wp);

typedef struct mgui_btnBmps
{
	HFONT font;
	HBITMAP hbmNormal, hbmPressed, hbmHighlight;
	bool tracking, hover, press;

} mgui_btnBmps_t;

HWND mgui_btnCreate(
	DWORD dwExStyle,
	LPCWSTR name,
	DWORD dwStyle,
	int x, int y,
	int width, int height,
	HWND parent,
	HMENU hmenu,
	HINSTANCE hInstance,
	COLORREF color,
	HFONT font
);
LRESULT CALLBACK mgui_btnOwnerDrawProc(
	HWND hwnd, UINT umsg, WPARAM wp, LPARAM lp,
	UINT_PTR uidsubclass, DWORD_PTR dwRefData
);


#endif
