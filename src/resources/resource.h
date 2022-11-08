#ifndef RESOURCE_H
#define RESOURCE_H

#include "../common/winapi.h"

#define MOUSE_STATS_CLASS L"mouseStatsClass"
#define MOUSE_STATS_TITLE_ASCII "mouseStats"
#define MOUSE_STATS_TITLE L"" MOUSE_STATS_TITLE_ASCII
#define MOUSE_STATS_TIP   MOUSE_STATS_TITLE "\n*Tool to track your mouse*"

#define VERSION_SEQUENCE      0,1
#define VERSION_SEQUENCE_STR "0.1"


#define IDM_EXIT 101
#define IDM_MIN  102
#define IDM_TEST 103
#define IDM_SHOW 104

#define MS_DEFAULT_COLOR        RGB( 72,  79,  91)
#define MS_TITLE_COLOR          RGB( 50, 127, 255)
#define MS_TITLE_INACTIVE_COLOR RGB(192, 192, 192)
#define MS_MINBTN_COLOR         RGB(  0, 162, 232)
#define MS_CLOSEBTN_COLOR       RGB(215,  21,  38)

#define MS_CALCBTN_COLOR(col)           RGB(ms_cClip((4 * (GetRValue(col) + 5)) / 3), ms_cClip((5 * (GetGValue(col) + 5)) / 4), ms_cClip((4 * (GetBValue(col) + 5)) / 3))
#define MS_CALCBTN_PRESS_COLOR(r, g, b) RGB(r, g, (b * 4) / 5)
#define MS_CALCBTN_HIGH_COLOR(r, g, b)  RGB(ms_cClip(((r + 10) * 5) / 4), ms_cClip(((g + 10) * 4) / 3), ms_cClip(((b + 10) * 4) / 3))
#define MS_BTN_PRESSCOLOR               RGB(255, 255, 255)

// Define GUI element sizes
#define MS_MARGIN      10
#define MS_WND_X      400
#define MS_WND_Y      300
#define MS_FONT_SIZEPT      11
#define MS_TITLEFONT_SIZEPT 12

#define MS_TITLEBTN_SIZE 40
#define MS_TITLE_HEIGHT  30

#define MS_BTNW 90
#define MS_BTNH 32

#endif
