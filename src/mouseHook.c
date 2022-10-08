#include "mouseHook.h"
#include "serializer.h"

static HHOOK s_mh_mouseHook = NULL;
static HWND s_mh_hwnd = NULL;

LRESULT CALLBACK mh_mouseHookProc(int nCode, WPARAM wp, LPARAM lp)
{
	if ((nCode >= 0) && SendMessageW(s_mh_hwnd, WM_MOUSEHOOK_MSG, wp, lp))
	{
		return 1;
	}
	return CallNextHookEx(s_mh_mouseHook, nCode, wp, lp);
}
HHOOK mh_setHook(HINSTANCE hInst, HWND hwnd)
{
	assert(s_mh_mouseHook == NULL);

	s_mh_mouseHook = SetWindowsHookExW(
		WH_MOUSE_LL,
		&mh_mouseHookProc,
		hInst,
		0
	);

	if (s_mh_mouseHook != NULL)
	{
		s_mh_hwnd = hwnd;
	}

	return s_mh_mouseHook;
}
void mh_removeHook(void)
{
	assert(s_mh_mouseHook != NULL);

	UnhookWindowsHookEx(s_mh_mouseHook);
	s_mh_mouseHook = NULL;
	s_mh_hwnd      = NULL;
}

bool mh_decode(mh_data_t * restrict ptr, WPARAM wp, LPARAM lp)
{
	assert(ptr != NULL);

	// Determine message type
	switch (wp)
	{
	case WM_LBUTTONDOWN:
		ptr->eventType = mh_lbdown;
		break;
	case WM_LBUTTONUP:
		ptr->eventType = mh_lbup;
		break;
	case WM_RBUTTONDOWN:
		ptr->eventType = mh_rbdown;
		break;
	case WM_RBUTTONUP:
		ptr->eventType = mh_rbup;
		break;
	case WM_MOUSEMOVE:
		ptr->eventType = mh_move;
		break;
	case WM_MOUSEWHEEL:
		ptr->eventType = mh_wheel;
		break;
	case WM_MOUSEHWHEEL:
		ptr->eventType = mh_hwheel;
		break;
	}
	
	const MSLLHOOKSTRUCT * restrict hstruct = (MSLLHOOKSTRUCT *)lp;
	if (hstruct == NULL)
	{
		return false;
	}
	ptr->cursorPos = hstruct->pt;
	
	if (ptr->eventType == mh_wheel)
	{
		ptr->wheelDelta  = (int16_t)HIWORD(hstruct->mouseData);
		ptr->hwheelDelta = 0;
	}
	else if (ptr->eventType == mh_hwheel)
	{
		ptr->wheelDelta  = 0;
		ptr->hwheelDelta = (int16_t)HIWORD(hstruct->mouseData);
	}
	else
	{
		ptr->wheelDelta  = 0;
		ptr->hwheelDelta = 0;
	}

	ptr->timeStamp = hstruct->time;
	return true;
}
const char * mh_eventName(mh_type_e type)
{
	static const char * eventNames[mh_num_types] = {
		"lbdown",
		"lbup",
		"rbdown",
		"rbup",
		"move",
		"wheel",
		"hwheel"
	};

	assert(type < mh_num_types);

	return eventNames[type];
}


#define S_RECTIMERTHREAD_STACK_SIZE 150
static DWORD WINAPI s_recTimerThread(LPVOID args);

static inline HANDLE s_recOpenWritable(mh_rectimer_t * restrict rectimer)
{
	assert(rectimer->path != NULL);
	
	// Open file
	HANDLE hfile = CreateFileW(
		rectimer->path,
		GENERIC_WRITE,
		0,
		NULL,
		OPEN_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);
	
	if (hfile == INVALID_HANDLE_VALUE)
	{
		return INVALID_HANDLE_VALUE;
	}
	
	// Skip to file end
	if (SetFilePointer(hfile, 0L, NULL, FILE_END) == INVALID_SET_FILE_POINTER)
	{
		CloseHandle(hfile);
		return INVALID_HANDLE_VALUE;
	}
	
	return hfile;
}
static inline bool s_recWrite(HANDLE hfile, const void * restrict data, size_t dataSize)
{
	assert(data != NULL);
	
	if (dataSize == 0)
	{
		return true;
	}
	
	return ser_serialize(
		data,
		dataSize,
		1,
		0,
		hfile,
		&ser_winFileWriter,
		NULL
	);
}
static inline const mh_data_t * s_getLastRecord(const mh_records_t * restrict recs)
{
	assert(recs != NULL);
	
	if (recs->numRecords == 0)
	{
		return NULL;
	}
	else
	{
		const mh_record_t * restrict record = &recs->records[recs->numRecords - 1];
		return &record->entries[record->numEntries - 1];
	}
}


static DWORD WINAPI s_recTimerThread(LPVOID args)
{
	mh_records_t * restrict recs = (mh_records_t *)args;
	assert(recs != NULL);
	mh_rectimer_t * restrict rectimer = &recs->rectimer;
	
	while (!rectimer->killThread)
	{
		EnterCriticalSection(&rectimer->critSect);
		
		const size_t prevRecords = recs->numRecords;
		const size_t prevEntries = (recs->records != NULL) ? recs->records[prevRecords - 1].numEntries : 0;
		
		LeaveCriticalSection(&rectimer->critSect);
		
		do
		{
			SleepConditionVariableCS(&rectimer->cv, &rectimer->critSect, (DWORD)(1000.f * RECORDS_TIME_THRESHOLD));
		
		} while ((recs->numRecords == prevRecords) &&
		       (  ( (recs->records != NULL) && (recs->records[prevRecords - 1].numEntries == prevEntries) ) ||
			        (recs->records == NULL)
			   )
		);
		
		// Get current time stamp
		const mh_data_t * restrict lastrecord = s_getLastRecord(recs);
		if ((lastrecord != NULL) && ((lastrecord->timeStamp - GetTickCount()) > (DWORD)(1000.0f * RECORDS_TIME_THRESHOLD)))
		{
			rectimer->writeAll = true;
		}

		// Save data to disk
		
		// Copy to local memory
		EnterCriticalSection(&rectimer->critSect);
		
		recs->isCopying = true;
		recs->numWasInCopy = recs->numRecsInCopy;
		
		LeaveCriticalSection(&rectimer->critSect);
		
		const size_t numRecstowrite = recs->numRecsInCopy - (recs->copy[recs->numRecsInCopy - 1].numEntries != MAX_ENTRIES_PER_RECORD);
		// Write to disk
		uint64_t writableNum = (uint64_t)(rectimer->writeAll ? recs->numRecsInCopy : numRecstowrite);
		
		HANDLE hfile = s_recOpenWritable(rectimer);
		if (hfile == INVALID_HANDLE_VALUE)
		{
			EnterCriticalSection(&rectimer->critSect);
			recs->isCopying = false;
			LeaveCriticalSection(&rectimer->critSect);
			
			continue;
		}
		
		s_recWrite(hfile, &writableNum, sizeof(uint64_t));
		s_recWrite(hfile, recs->copy, numRecstowrite * sizeof(mh_record_t));
		
		// Shift data to front
		if (recs->numRecsInCopy != numRecstowrite)
		{
			if (rectimer->writeAll)
			{
				rectimer->writeAll = false;
				// Write last record to disk
				const mh_record_t * restrict prec = &recs->copy[recs->numWasInCopy - 1];
				s_recWrite(hfile, prec, sizeof(ENTRY_SIZE_T) + prec->numEntries * sizeof(mh_data_t));
				
				recs->numRecsInCopy = 0;
			}
			else
			{
				recs->numRecsInCopy = 1;
			}
		}
		else
		{
			recs->numRecsInCopy = 0;
		}
		
		CloseHandle(hfile);
		
		EnterCriticalSection(&rectimer->critSect);
		
		// Give control back
		recs->isCopying = false;
		WakeAllConditionVariable(&rectimer->readycv);
		
		LeaveCriticalSection(&rectimer->critSect);
	}
	
	return 0;
}

bool mh_timer_create(mh_records_t * restrict recs, const wchar * restrict path)
{
	assert(recs != NULL);
	assert(path != NULL);
	
	mh_rectimer_t * restrict rectimer = &recs->rectimer;
	
	if (rectimer->init)
	{
		return true;
	}
	
	rectimer->killThread = false;
	rectimer->writeAll   = false;
	rectimer->path       = wcsdup(path);
	
	if (rectimer->path == NULL)
	{
		return false;
	}
	
	InitializeCriticalSection(&rectimer->critSect);
	InitializeConditionVariable(&rectimer->cv);
	InitializeConditionVariable(&rectimer->readycv);
	
	// Create Thread
	rectimer->hthread = CreateThread(
		NULL,
		S_RECTIMERTHREAD_STACK_SIZE * sizeof(size_t),
		&s_recTimerThread,
		recs,
		0,
		NULL
	);
	
	if (rectimer->hthread == NULL)
	{
		DeleteCriticalSection(&rectimer->critSect);
	}
	
	rectimer->init = true;
	
	return true;
}
void mh_timer_destroy(mh_records_t * restrict recs)
{
	assert(recs != NULL);
	
	mh_rectimer_t * restrict rectimer = &recs->rectimer;
	
	if (rectimer->init)
	{
		rectimer->killThread = true;
		WakeAllConditionVariable(&rectimer->cv);
		WaitForSingleObject(rectimer->hthread, INFINITE);
		
		// Delete critical section
		DeleteCriticalSection(&rectimer->critSect);
		
		rectimer->init = false;
		free(rectimer->path);
		rectimer->path = NULL;
	}
}



bool mh_rec_add(mh_record_t * restrict record, const mh_data_t * restrict entry)
{
	assert(record != NULL);
	assert(entry != NULL);
	
	if (record->numEntries < MAX_ENTRIES_PER_RECORD)
	{
		record->entries[record->numEntries] = *entry;
		++record->numEntries;
		
		return true;
	}
	else
	{
		return false;
	}
}



bool mh_recs_create(mh_records_t * restrict recs, const wchar * restrict path)
{
	assert(recs != NULL);
	
	*recs = (mh_records_t){
		.numRecords = 0,
		.maxRecords = 0,
		.records    = NULL,
		
		.numRecsInCopy = 0,
		.copy          = NULL,
		.isCopying     = false
	};
	
	return mh_timer_create(recs, path);
}
bool mh_recs_destroy(mh_records_t * restrict recs)
{
	assert(recs != NULL);
	
	bool suc = mh_recs_todisk(recs, true);
	mh_timer_destroy(recs);
	
	free(recs->records);
	free(recs->copy);
	
	recs->numRecords    = 0;
	recs->records       = NULL;
	recs->numRecsInCopy = 0;
	recs->copy          = NULL;
	
	return suc;
}

static inline bool s_addToRecs(mh_record_t * restrict prec, size_t * restrict pnumRecs, const mh_data_t * restrict entry)
{
	assert(prec != NULL);
	assert(pnumRecs != NULL);
	assert(entry != NULL);
	
	if ((*pnumRecs) == 0)
	{
		++(*pnumRecs);
	}
	
	if (!mh_rec_add(&prec[(*pnumRecs) - 1], entry))
	{
		++(*pnumRecs);
		return mh_rec_add(&prec[(*pnumRecs) - 1], entry);
	}
	else
	{
		return true;
	}
}
bool mh_recs_add(mh_records_t * restrict recs, const mh_data_t * restrict entry)
{
	assert(recs != NULL);
	assert(entry != NULL);
	
	mh_rectimer_t * restrict rectimer = &recs->rectimer;
	
	// Resize
	if ((recs->numRecords + 1) >= recs->maxRecords)
	{
		const size_t tempmax = (recs->numRecords + 1) * 2;
		EnterCriticalSection(&rectimer->critSect);
		mh_record_t * restrict temp = (mh_record_t *)realloc(recs->records, tempmax * sizeof(mh_record_t));
		
		if (temp == NULL)
		{
			LeaveCriticalSection(&rectimer->critSect);
			return NULL;
		}
		
		recs->records    = temp;
		recs->maxRecords = tempmax;
		
		LeaveCriticalSection(&rectimer->critSect);
		
		EnterCriticalSection(&rectimer->critSect);
		if (!recs->isCopying)
		{
			temp = (mh_record_t *)realloc(recs->copy, tempmax * sizeof(mh_record_t));
			if (temp == NULL)
			{
				LeaveCriticalSection(&rectimer->critSect);
				return NULL;
			}
			
			recs->copy = temp;
		}
		LeaveCriticalSection(&rectimer->critSect);
	}
	
	// Write to records
	
	EnterCriticalSection(&rectimer->critSect);
	if (!recs->isCopying)
	{
		if (recs->numRecsInCopy != recs->numRecords)
		{
			// Move records structure
			if (recs->numRecords > recs->numWasInCopy)
			{
				memmove(recs->records, &recs->records[recs->numWasInCopy], sizeof(mh_record_t) * (recs->numRecords - recs->numWasInCopy));
				recs->numRecords -= recs->numWasInCopy;
			}
			else if (recs->numRecords > 0)
			{
				recs->records[0].numEntries = 0;
				recs->numRecords = 0;
			}
			
			if (recs->numRecsInCopy)
			{
				const mh_record_t * restrict prec   = &recs->copy[recs->numWasInCopy - 1];
				const mh_data_t * restrict pentries = prec->entries;
				for (ENTRY_SIZE_T i = 0; i < prec->numEntries; ++i)
				{
					s_addToRecs(recs->records, &recs->numRecords, &pentries[i]);
				}
			}
			
			// Make a copy of records structure to "copy"
			memcpy(recs->copy, recs->records, sizeof(mh_record_t) * recs->numRecords);
			recs->numRecsInCopy = recs->numRecords;
		}
		
		s_addToRecs(recs->copy, &recs->numRecsInCopy, entry);
	}
	
	s_addToRecs(recs->records, &recs->numRecords, entry);
	
	LeaveCriticalSection(&rectimer->critSect);
	
	return true;
}
bool mh_recs_todisk(mh_records_t * restrict recs, bool writeAll)
{
	assert(recs != NULL);
	
	mh_rectimer_t * restrict rectimer = &recs->rectimer;
	
	if (rectimer->init && (recs != NULL))
	{
		rectimer->writeAll = writeAll;
		WakeAllConditionVariable(&rectimer->cv);
		
		if (recs->isCopying)
		{
			SleepConditionVariableCS(&rectimer->readycv, &rectimer->critSect, 1000);
		}
		if (recs->isCopying)
		{
			return false;
		}
		
		return true;
	}
	else
	{
		return false;
	}
}
