#ifndef GUI_H
#define GUI_H

#include "winapi.h"

#define MAX_TITLETEXT MAX_PATH

LRESULT CALLBACK mgui_winProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

LRESULT mgui_hitTest(HWND hwnd, POINT cursor, RECT * restrict titleRect, bool borderless_resize);
void mgui_adjustMaximizedClientRect(HWND window, RECT * restrict rect);
void mgui_calcTitleRect(HWND hwnd, RECT * restrict rect, int dpi);

void mgui_handleContextMenu(HWND hwnd, LPARAM lp);
void mgui_handleCommand(HWND hwnd, WPARAM wp);
bool mgui_handleSystemMenu(HWND hwnd, WPARAM wp);

#endif
