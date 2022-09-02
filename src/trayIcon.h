#ifndef TRAYICON_H
#define TRAYICON_H

#include "winapi.h"

#define TRAYICON_ID 1000
#define WM_TRAYICON_MESSAGE (WM_USER + TRAYICON_ID)

typedef enum ti_msgType
{
	ti_none,
	ti_default,
	ti_error,
	ti_info,
	ti_warn,
	ti_excl = ti_warn,
	ti_question,
	ti_ask   = ti_question,
	ti_quest = ti_question
	
} ti_msgType_e;

typedef struct ti_data
{
	NOTIFYICONDATAW nid;
	HMENU hPopupMenu;

} ti_data_t;

bool ti_create(ti_data_t * restrict This, const wchar * tipInfo, const wchar * defaultInfoTitle, HWND parent, HICON hIcon);
bool ti_destroy(ti_data_t * restrict This);
bool ti_sendMessage(ti_data_t * restrict This, const wchar * msg, const wchar * title, ti_msgType_e msgType);
bool ti_tipInfo(ti_data_t * restrict This, const wchar * restrict tipInfo);
bool ti_context(ti_data_t * restrict This);

#endif
