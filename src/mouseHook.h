#ifndef MOUSE_HOOK_H
#define MOUSE_HOOK_H

#include "winapi.h"

#define WM_MOUSEHOOK_MSG WM_USER

LRESULT CALLBACK mh_mouseHookProc(int nCode, WPARAM wp, LPARAM lp);
HHOOK mh_setHook(HINSTANCE hInst, HWND hwnd);
void mh_removeHook(void);

typedef enum mh_type
{
	mh_lbdown,
	mh_lbup,
	mh_rbdown,
	mh_rbup,
	mh_move,
	mh_wheel,
	mh_hwheel,

	mh_num_types

} mh_type_e;

typedef struct mh_data
{
	mh_type_e eventType;
	POINT cursorPos;
	int16_t wheelDelta, hwheelDelta;
	DWORD timeStamp;

} mh_data_t;

bool mh_decode(mh_data_t * restrict ptr, WPARAM wp, LPARAM lp);
const char * mh_eventName(mh_type_e type);


struct mh_records;

typedef struct mh_rectimer
{
	bool init, killThread, writeAll;
	// condition variable
	CRITICAL_SECTION critSect;
	CONDITION_VARIABLE cv, readycv;
	
	// thread
	HANDLE hthread;
	
	wchar * path;

} mh_rectimer_t;

bool mh_timer_create(struct mh_records * restrict recs, const wchar * restrict path);
void mh_timer_destroy(struct mh_records * restrict recs);

#define MAX_ENTRIES_PER_RECORD 16384


#if MAX_ENTRIES_PER_RECORD <= UINT8_MAX
	#define ENTRY_SIZE_T uint8_t
#elif MAX_ENTRIES_PER_RECORD <= UINT16_MAX
	#define ENTRY_SIZE_T uint16_t
#elif MAX_ENTRIES_PER_RECORD <= UINT32_MAX
	#define ENTRY_SIZE_T uint32_t
#else
	#define ENTRY_SIZE_T size_t
#endif

#pragma pack(push, 1)

typedef struct mh_record
{
	ENTRY_SIZE_T numEntries;
	mh_data_t entries[MAX_ENTRIES_PER_RECORD];

} mh_record_t;

#pragma pack(pop)

bool mh_rec_add(mh_record_t * restrict record, const mh_data_t * restrict entry);

#define RECORDS_TIME_THRESHOLD 60.0f

typedef struct mh_records
{
	size_t numRecords, maxRecords;
	mh_record_t * records;
	
	size_t numRecsInCopy, numWasInCopy;
	mh_record_t * copy;
	bool isCopying;

	mh_rectimer_t rectimer;
	
} mh_records_t;

bool mh_recs_create(mh_records_t * restrict recs, const wchar * restrict path);
bool mh_recs_destroy(mh_records_t * restrict recs);

bool mh_recs_add(mh_records_t * restrict recs, const mh_data_t * restrict entry);
bool mh_recs_todisk(mh_records_t * restrict recs, bool writeAll);

typedef struct mh_statistics
{
	size_t numRecords;
	mh_record_t * records;

} mh_statistics_t;

bool mh_statistics_create(mh_statistics_t * restrict stats);


#endif
