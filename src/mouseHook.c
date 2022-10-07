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


static struct mh_rectimer s_rectimer;

#define S_RECTIMERTHREAD_STACK_SIZE 1000
static DWORD WINAPI s_recTimerThread(LPVOID args);

static inline bool s_createRecTimer(void)
{
	if (s_rectimer.init)
	{
		return true;
	}
	
	InitializeCriticalSection(&s_rectimer.critSect);
	InitializeConditionVariable(&s_rectimer.cv);

	s_rectimer.killThread = false;
	s_rectimer.writeAll   = false;
	s_rectimer.recs       = NULL;
	s_rectimer.path       = NULL;
	
	// Create Thread
	s_rectimer.hthread = CreateThread(
		NULL,
		S_RECTIMERTHREAD_STACK_SIZE * sizeof(size_t),
		&s_recTimerThread,
		NULL,
		0,
		NULL
	);
	
	if (s_rectimer.hthread == NULL)
	{
		DeleteCriticalSection(&s_rectimer.critSect);
	}
	
	s_rectimer.init = true;
	
	return true;
}
static inline HANDLE s_recOpenWritable(void)
{
	assert(s_rectimer.path != NULL);
	
	// Open file
	HANDLE hfile = CreateFileW(
		s_rectimer.path,
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

static inline const mh_data_t * s_getLastRecord(void)
{
	assert(s_rectimer.recs != NULL);
	
	const mh_records_t * recs = s_rectimer.recs;
	
	if (recs->numRecords == 0)
	{
		return NULL;
	}
	else
	{
		const mh_record_t * record = &recs->records[recs->numRecords - 1];
		return &record->entries[record->numEntries - 1];
	}
}
static inline DWORD s_getTimeStamp(void)
{
	return GetTickCount();
}

static DWORD WINAPI s_recTimerThread(LPVOID args)
{
	(void)args;
	
	while (!s_rectimer.killThread)
	{
		if (s_rectimer.recs == NULL)
		{
			Sleep(1);
			continue;
		}
		
		EnterCriticalSection(&s_rectimer.critSect);
		
		const size_t prevRecords = s_rectimer.recs->numRecords;
		const size_t prevEntries = (s_rectimer.recs->records != NULL) ? s_rectimer.recs->records[prevRecords - 1].numEntries : 0;
		
		LeaveCriticalSection(&s_rectimer.critSect);
		
		do
		{
			SleepConditionVariableCS(&s_rectimer.cv, &s_rectimer.critSect, (DWORD)(1000.f * RECORDS_TIME_THRESHOLD));
		
		} while ((s_rectimer.recs->numRecords == prevRecords) &&
		       (  ( (s_rectimer.recs->records != NULL) && (s_rectimer.recs->records[prevRecords - 1].numEntries == prevEntries) ) ||
			        (s_rectimer.recs->records == NULL)
			   )
		);
		
		// Get current time stamp
		const mh_data_t * restrict lastrecord = s_getLastRecord();
		if ((lastrecord != NULL) && ((lastrecord->timeStamp - GetTickCount()) > (DWORD)(1000.0f * RECORDS_TIME_THRESHOLD)))
		{
			s_rectimer.writeAll = true;
		}

		// Save data to disk
		
		// Copy to local memory
		EnterCriticalSection(&s_rectimer.critSect);
		
		s_rectimer.recs->isCopying = true;
		s_rectimer.recs->numWasInCopy = s_rectimer.recs->numRecsInCopy;
		
		LeaveCriticalSection(&s_rectimer.critSect);
		
		const size_t numRecstowrite = s_rectimer.recs->numRecsInCopy - (s_rectimer.recs->copy[s_rectimer.recs->numRecsInCopy - 1].numEntries != MAX_ENTRIES_PER_RECORD);
		// Write to disk
		uint64_t writableNum = (uint64_t)(s_rectimer.writeAll ? s_rectimer.recs->numRecsInCopy : numRecstowrite);
		
		HANDLE hfile = s_recOpenWritable();
		if (hfile == INVALID_HANDLE_VALUE)
		{
			EnterCriticalSection(&s_rectimer.critSect);
			s_rectimer.recs->isCopying = false;
			LeaveCriticalSection(&s_rectimer.critSect);
			
			continue;
		}
		
		s_recWrite(hfile, &writableNum, sizeof(uint64_t));
		s_recWrite(hfile, s_rectimer.recs->copy, numRecstowrite * sizeof(mh_record_t));
		
		// Shift data to front
		if (s_rectimer.recs->numRecsInCopy != numRecstowrite)
		{
			if (s_rectimer.writeAll)
			{
				s_rectimer.writeAll = false;
				// Write last record to disk
				const mh_record_t * restrict prec = &s_rectimer.recs->copy[s_rectimer.recs->numWasInCopy - 1];
				s_recWrite(hfile, prec, sizeof(ENTRY_SIZE_T) + prec->numEntries * sizeof(mh_data_t));
				
				s_rectimer.recs->numRecsInCopy = 0;
			}
			else
			{
				s_rectimer.recs->numRecsInCopy = 1;
			}
		}
		else
		{
			s_rectimer.recs->numRecsInCopy = 0;
		}
		
		CloseHandle(hfile);
		
		EnterCriticalSection(&s_rectimer.critSect);
		
		// Give control back
		s_rectimer.recs->isCopying = false;
		WakeAllConditionVariable(&s_rectimer.readycv);
		
		LeaveCriticalSection(&s_rectimer.critSect);
	}
	
	return 0;
}

bool mh_timer_set(struct mh_records * restrict recs, const wchar * restrict path)
{
	assert(recs != NULL);
	assert(path != NULL);
	
	if (!s_createRecTimer())
	{
		return false;
	}
	else if (s_rectimer.recs != NULL)
	{
		return false;
	}
	
	s_rectimer.path = wcsdup(path);
	if (s_rectimer.path == NULL)
	{
		return false;
	}
	s_rectimer.recs = recs;
	
	return true;
}
void mh_timer_destroy(void)
{
	if (s_rectimer.init)
	{
		s_rectimer.killThread = true;
		WakeAllConditionVariable(&s_rectimer.cv);
		WaitForSingleObject(s_rectimer.hthread, INFINITE);
		
		// Delete critical section
		DeleteCriticalSection(&s_rectimer.critSect);
		
		s_rectimer.init = false;
		free(s_rectimer.path);
		s_rectimer.path = NULL;
	}
}
bool mh_timer_todisk(bool writeAll)
{
	if (s_rectimer.init && (s_rectimer.recs != NULL))
	{
		s_rectimer.writeAll = writeAll;
		WakeAllConditionVariable(&s_rectimer.cv);
		
		if (s_rectimer.recs->isCopying)
		{
			SleepConditionVariableCS(&s_rectimer.readycv, &s_rectimer.critSect, 1000);
		}
		if (s_rectimer.recs->isCopying)
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
	
	return mh_timer_set(recs, path);
}
bool mh_recs_destroy(mh_records_t * restrict recs)
{
	assert(recs != NULL);
	
	bool suc = true;
	if (s_rectimer.recs == recs)
	{
		suc = mh_timer_todisk(true);
		mh_timer_destroy();
	}
	
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
	
	// Resize
	if ((recs->numRecords + 1) >= recs->maxRecords)
	{
		const size_t tempmax = (recs->numRecords + 1) * 2;
		EnterCriticalSection(&s_rectimer.critSect);
		mh_record_t * temp = (mh_record_t *)realloc(recs->records, tempmax * sizeof(mh_record_t));
		
		if (temp == NULL)
		{
			LeaveCriticalSection(&s_rectimer.critSect);
			return NULL;
		}
		
		recs->records    = temp;
		recs->maxRecords = tempmax;
		
		LeaveCriticalSection(&s_rectimer.critSect);
		
		EnterCriticalSection(&s_rectimer.critSect);
		if (!recs->isCopying)
		{
			temp = (mh_record_t *)realloc(recs->copy, tempmax * sizeof(mh_record_t));
			if (temp == NULL)
			{
				LeaveCriticalSection(&s_rectimer.critSect);
				return NULL;
			}
			
			recs->copy = temp;
		}
		LeaveCriticalSection(&s_rectimer.critSect);
	}
	
	// Write to records
	
	EnterCriticalSection(&s_rectimer.critSect);
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
	
	LeaveCriticalSection(&s_rectimer.critSect);
	
	return true;
}
