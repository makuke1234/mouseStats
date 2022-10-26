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

#define MS_DEFAULT_COLOR RGB(72, 79, 91)

#endif
