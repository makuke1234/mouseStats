#include "mouseHook.h"
#include "serializer.h"
#include "error.h"

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


#define YEAR2000_EPOCH 12590294400ULL

static bool s_startupset = false;
static uint64_t s_startupseconds_since2000 = 0;

static inline void s_setStartup(void)
{
	SYSTEMTIME lt;
	const uint64_t currentTicks = GetTickCount64();
	GetLocalTime(&lt);
	
	FILETIME ft;
	if (!SystemTimeToFileTime(&lt, &ft))
	{
		return;
	}
	
	ULARGE_INTEGER uli;
	uli.LowPart  = ft.dwLowDateTime;
	uli.HighPart = ft.dwHighDateTime;
	
	uli.QuadPart /= 10000ULL;
	uli.QuadPart = (uli.QuadPart - currentTicks) / 1000ULL;
	
	s_startupseconds_since2000 = (uint64_t)(uli.QuadPart - YEAR2000_EPOCH);
	
	s_startupset = true;
}

mh_time_t mh_tm_fromTicks(DWORD ticks)
{
	return mh_tm_fromTicks64((ULONGLONG)ticks);
}
mh_time_t mh_tm_fromTicks64(ULONGLONG ticks)
{
	if (!s_startupset)
	{
		s_setStartup();
	}
	
	return (mh_time_t){ (uint32_t)((ticks / 1000ULL) - s_startupseconds_since2000) };
}


static inline bool s_isLeapYear(uint16_t year)
{
	return !(year % 4) && (!(year % 400) || (year % 100));
}
static inline uint16_t s_daysInYear(uint16_t year)
{
	return 365U + (uint16_t)s_isLeapYear(year);
}
static inline uint16_t s_daysInMonth(uint16_t month, uint16_t year)
{
	static const uint16_t daysInMonth[12] = {
		31,
		28,
		31,
		30,
		31,
		30,
		31,
		31,
		30,
		31,
		30,
		31
	};
	return daysInMonth[month - 1] + (uint16_t )((month == 1) * s_isLeapYear(year));
}
static inline bool s_checkDay(uint8_t day, uint8_t month, uint16_t year)
{
	return (day >= 1) && (day <= s_daysInMonth(month, year));
}

mh_time_t mh_tm_fromDate(
	uint8_t day, uint8_t month, uint16_t year,
	uint8_t hour, uint8_t minute, uint8_t second
)
{
	if (!(year >= 2000) ||
		!((month >= 1) && (month <= 12)) ||
		!s_checkDay(day, month, year) ||
		!(hour <= 23) ||
		!(minute <= 59) ||
		!(second <= 59)
	)
	{
		return (mh_time_t){ 0 };
	}
	else
	{
		uint16_t syear = 2000;
		uint8_t  sm = 1;
		
		uint32_t ans = 0;
		
		// Increase years
		while (syear < year)
		{
			ans += s_daysInYear(syear);
			++syear;
		}
		// Convert days to seconds
		ans *= 86400U;
		
		uint32_t ans2 = 0;
		while (sm < month)
		{
			ans2 += s_daysInMonth(sm, syear);
			++sm;
		}
		// Add months as seconds
		ans += ans2 * 86400;
		
		// Add days as seconds
		ans += (uint32_t)(day - 1) * 86400U;
		ans += hour   * 3600U;
		ans += minute * 60U;
		ans += second;
		
		return (mh_time_t){ ans };
	}
}
mh_time_t mh_tm_fromString(const char * restrict str)
{
	assert(str != NULL);
	
	uint16_t dd = 0, mm = 0, yyyy = 0, hr = 0, min = 0, sec = 0;
	if (sscanf(str, "%hu.%hu.%hu%hu:%hu:%hu", &dd, &mm, &yyyy, &hr, &min, &sec) < 6)
	{
		sscanf(str, "%hu:%hu:%hu%hu.%hu.%hu", &hr, &min, &sec, &dd, &mm, &yyyy);
	}
	
	return mh_tm_fromDate(
		(uint8_t)dd, (uint8_t) mm,          yyyy,
		(uint8_t)hr, (uint8_t)min, (uint8_t) sec
	);
}
mh_time_t mh_tm_fromSysTime(const SYSTEMTIME * restrict st)
{
	// Convert to FILETIME
	FILETIME ft;
	if (!SystemTimeToFileTime(st, &ft))
	{
		return (mh_time_t){ 0 };
	}
	
	ULARGE_INTEGER uli;
	uli.LowPart  = ft.dwLowDateTime;
	uli.HighPart = ft.dwHighDateTime;
	
	return (mh_time_t){ (uint32_t)((uli.QuadPart / 10000000ULL) - YEAR2000_EPOCH) };
}
mh_time_t mh_tm_fromCurrent(void)
{
	SYSTEMTIME lt;
	GetLocalTime(&lt);
	
	return mh_tm_fromSysTime(&lt);
}

DWORD mh_tm_toTicks(mh_time_t tm)
{
	return (DWORD)mh_tm_toTicks64(tm);
}
ULONGLONG mh_tm_toTicks64(mh_time_t tm)
{
	if (!s_startupset)
	{
		s_setStartup();
	}
	
	return ((uint64_t)tm.secs + s_startupseconds_since2000) * 1000ULL;
}
SYSTEMTIME mh_tm_toSysTime(mh_time_t tm)
{
	// Get file time
	ULARGE_INTEGER uli;
	uli.QuadPart = ((uint64_t)tm.secs + YEAR2000_EPOCH) * 10000000ULL;
	
	FILETIME ft;
	ft.dwLowDateTime  = uli.LowPart;
	ft.dwHighDateTime = uli.HighPart;

	SYSTEMTIME st;
	if (!FileTimeToSystemTime(&ft, &st))
	{
		return (SYSTEMTIME){ 0 };
	}
	
	return st;
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

	ptr->timeStamp = mh_tm_fromTicks(hstruct->time);
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

static inline HANDLE s_recOpenWritable(const wchar * restrict path)
{
	assert(path != NULL);
	
	// Open file
	HANDLE hfile = CreateFileW(
		path,
		GENERIC_WRITE,
		FILE_SHARE_READ,
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
static inline HANDLE s_recOpenReadable(const wchar * restrict path)
{
	assert(path != NULL);
	
	return CreateFileW(
		path,
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);
}

static inline bool s_dataWrite(HANDLE hfile, const void * restrict data, size_t objSize, size_t numObjects)
{
	assert(hfile != INVALID_HANDLE_VALUE);
	assert(data != NULL);
	assert(objSize > 0);
	
	if (numObjects == 0)
	{
		return true;
	}
	
	return ser_serialize(
		data,
		objSize,
		numObjects,
		0,
		hfile,
		&ser_winFileWriter,
		NULL
	);
}
static inline bool s_dataRead(HANDLE hfile, void * data, size_t objSize)
{
	assert(hfile != INVALID_HANDLE_VALUE);
	assert(data != NULL);
	assert(objSize > 0);
	
	return ser_deserialize(
		&data,
		objSize,
		0,
		hfile,
		&ser_winFileReader,
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
		if ((lastrecord != NULL) && ((mh_tm_toTicks(lastrecord->timeStamp) - GetTickCount()) > (DWORD)(1000.0f * RECORDS_TIME_THRESHOLD)))
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
		
		HANDLE hfile = s_recOpenWritable(rectimer->path);
		if (hfile == INVALID_HANDLE_VALUE)
		{
			EnterCriticalSection(&rectimer->critSect);
			recs->isCopying = false;
			LeaveCriticalSection(&rectimer->critSect);
			
			if (rectimer->errcounter == RECORDS_WRITE_ERROR_THRESHOLD)
			{
				ePrint("Error opening logging file to write mouse data!");
			}
			
			rectimer->errcounter = (rectimer->errcounter + ((rectimer->errcounter <= RECORDS_WRITE_ERROR_THRESHOLD) ? 1U : 0U)) & 0x1FFF;
			continue;
		}
		else
		{
			rectimer->errcounter = 0;
		}
		
		s_dataWrite(hfile, &writableNum, sizeof(uint64_t), 1);
		for (size_t i = 0; i < numRecstowrite; ++i)
		{
			const mh_record_t * restrict prec = &recs->copy[i];
			s_dataWrite(hfile, &prec->numEntries, sizeof(ENTRY_SIZE_T), 1);
			s_dataWrite(hfile, prec->entries, prec->numEntries * sizeof(mh_data_t), 1);		
		}
		
		// Shift data to front
		if (recs->numRecsInCopy != numRecstowrite)
		{
			if (rectimer->writeAll)
			{
				rectimer->writeAll = false;
				// Write last record to disk
				const mh_record_t * restrict prec = &recs->copy[recs->numWasInCopy - 1];
				s_dataWrite(hfile, &prec->numEntries, sizeof(ENTRY_SIZE_T), 1);
				s_dataWrite(hfile, prec->entries, prec->numEntries * sizeof(mh_data_t), 1);
				
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
	rectimer->errcounter = 0;
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

bool mh_statistics_create(mh_statistics_t * restrict stats, mh_records_t * restrict recs)
{
	assert(stats != NULL);
	assert(recs != NULL);
	
	if (!mh_recs_todisk(recs, true))
	{
		return false;
	}
	
	stats->numRecords = 0;
	stats->records    = NULL;
	
	// Load data from disk
	stats->hfile = s_recOpenWritable(recs->rectimer.path);
	if (stats->hfile == INVALID_HANDLE_VALUE)
	{
		return false;
	}
	
	return true;
}
void mh_statistics_destroy(mh_statistics_t * restrict stats)
{
	assert(stats != NULL);
	
	mh_statistics_unload(stats);
	
	if (stats->hfile != INVALID_HANDLE_VALUE)
	{
		CloseHandle(stats->hfile);
		stats->hfile = INVALID_HANDLE_VALUE;
	}
}

static inline bool s_stat_loadBlock(HANDLE hfile, uint64_t * restrict pnumBlocks, mh_record_t * restrict entry)
{
	assert(hfile      != INVALID_HANDLE_VALUE);
	assert(pnumBlocks != NULL);
	assert(entry      != NULL);
	
	if ((*pnumBlocks) == 0)
	{
		// Read new session data
		if (!s_dataRead(hfile, pnumBlocks, sizeof(uint64_t)))
		{
			return false;
		}
	}
	
	// Read block
	if (!s_dataRead(hfile, &entry->numEntries, sizeof(ENTRY_SIZE_T)))
	{
		return false;
	}
	if (!s_dataRead(hfile, entry->entries, entry->numEntries * sizeof(mh_data_t)))
	{
		return false;
	}
	
	--(*pnumBlocks);
	return true;
}

static inline bool s_statsAdd(
	mh_statistics_t * restrict stats,
	size_t * restrict maxRecords,
	const mh_data_t * restrict data
)
{
	assert(stats      != NULL);
	assert(maxRecords != NULL);
	assert(data       != NULL);
	
	if (((*maxRecords) + 1) >= stats->numRecords)
	{
		const size_t tempmax = (stats->numRecords + 1) * 2;
		mh_data_t * restrict temp = (mh_data_t *)realloc(stats->records, tempmax * sizeof(mh_data_t));
		
		if (temp == NULL)
		{
			return false;
		}
		
		stats->records = temp;
		(*maxRecords) = tempmax;
	}
	
	// Actually add the item
	stats->records[stats->numRecords] = *data;
	++stats->numRecords;
	
	return true;
}

bool mh_statistics_load(
	mh_statistics_t * restrict stats,
	mh_time_t stime,
	mh_time_t etime
)
{
	assert(stats != NULL);
	assert(stime.secs <= etime.secs);
	
	if (stats->hfile == INVALID_HANDLE_VALUE)
	{
		return false;
	}
	
	// Unload old data first
	mh_statistics_unload(stats);
	
	// Load data from file
	size_t maxRecs    = 0;
	stats->numRecords = 0;
	
	// Go to file begginning
	if (SetFilePointer(stats->hfile, 0L, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
	{
		return false;
	}
	
	uint64_t numBlocks = 0;
	mh_record_t entry = { .numEntries = 0 };
	const mh_data_t * restrict entries = entry.entries;
	
	while (s_stat_loadBlock(stats->hfile, &numBlocks, &entry))
	{
		// Scan through the read block
		for (ENTRY_SIZE_T i = 0; i < entry.numEntries; ++i)
		{
			// Check if timestamp matches the specified range
			if ((entries[i].timeStamp.secs >= stime.secs) && (entries[i].timeStamp.secs <= etime.secs))
			{
				// Add data to statistics
				if (!s_statsAdd(stats, &maxRecs, &entries[i]))
				{
					mh_statistics_unload(stats);
					return false;
				}
			}
		}
	}
	
	// Shrink statistics array to fit
	mh_data_t * restrict temp = (mh_data_t *)realloc(stats->records, sizeof(mh_data_t) * stats->numRecords);
	stats->records = (temp != NULL) ? temp : stats->records;
	
	return true;
}
bool mh_statistics_loadAll(mh_statistics_t * restrict stats)
{
	return mh_statistics_load(stats, (mh_time_t){ 0 }, (mh_time_t){ UINT32_MAX });
}
bool mh_statistics_unload(mh_statistics_t * restrict stats)
{
	assert(stats != NULL);
	
	if (stats->records != NULL)
	{
		free(stats->records);
		stats->records = NULL;
		return true;
	}
	else
	{
		return false;
	}
}
