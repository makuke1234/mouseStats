#include "error.h"

const char * restrict e_errorMessages[emsg_num_of_messages] = {
	[eInit]         = EINIT_STR,
	[eCreate]       = ECREATE_STR,
	[eMemErr]       = EMEM_ERR_STR,
	[eAsyncCmd]     = EASYNC_CMD_STR,
	[eLogFileOpen]  = ELOGFILE_OPEN_STR,
	[eLogFileWrite] = ELOGFILE_WRITE_STR,
	[eDataRecord]   = EDATA_RECORD_STR,
	
	[exWindow]      = EXWINDOW_STR,
	[exWindowClass] = EXWINDOW_CLASS_STR,
	[exTrayIcon]    = EXTRAY_ICON_STR,
	[exCmdEngine]   = EXCMD_ENGINE_STR,
	[exMouseLog]    = EXMOUSELOG_STR,
	[exMouseHook]   = EXMOUSEHOOK_STR,
	[exPathVar]     = EXPATH_VAR_STR
};

int eCon(const char * restrict format, ...)
{
	assert(format != NULL);

	va_list ap;
	va_start(ap, format);

	int ret = evCon(format, ap);

	va_end(ap);

	return ret;
}
int evCon(const char * restrict format, va_list ap)
{
	return vfprintf(stderr, format, ap);
}

int eMsg(const char * restrict format, ...)
{
	assert(format != NULL);

	va_list ap;
	va_start(ap, format);

	int ret = evMsg(format, ap);

	va_end(ap);

	return ret;
}
int evMsg(const char * restrict format, va_list ap)
{
	char temparr[MAX_EMSG];
	int ch = vsprintf_s(temparr, MAX_EMSG, format, ap);
	MessageBoxA(NULL, temparr, MOUSE_STATS_TITLE_ASCII, MB_ICONERROR | MB_OK);
	return ch;
}
