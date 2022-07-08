#ifndef APP_H
#define APP_H

#include "winapi.h"
#include "error.h"

typedef struct msdata
{
	bool init;
	int dpi;

	HHOOK mHook;
	HWND hwnd;

	RECT titleRect;
	HBRUSH titleBrush, titleBrushInactive;
	HFONT titleTextFont, titleBtnFont;
	bool resizeEnable;
	bool isActive;
	bool rclick;

	HMENU contextMenu, sysMenu;
	HWND closeBtn, minBtn;

} msdata_t;

bool ms_init(msdata_t * restrict This, int argc, char ** argv);

void ms_loop(msdata_t * restrict This);

void ms_free(msdata_t * restrict This);


#endif
