#ifndef APP_H
#define APP_H

#include "winapi.h"
#include "error.h"
#include "trayIcon.h"

typedef struct msdata
{
	bool init;
	int dpi;

	HHOOK mHook;
	HWND hwnd;

	RECT titleRect;
	int yBegin;
	HBRUSH titleBrush, titleBrushInactive;
	HFONT titleTextFont, titleBtnFont, btnFont, normFont;
	bool resizeEnable;
	bool isActive;
	bool rclick;

	HMENU contextMenu, sysMenu;
	HWND closeBtn, minBtn;

	HWND testBtn;
	
	ti_data_t tidata;

} msdata_t;

bool ms_init(msdata_t * restrict This, int argc, char ** argv);

void ms_loop(msdata_t * restrict This);

void ms_free(msdata_t * restrict This);


#endif
