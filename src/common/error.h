#ifndef ERROR_H
#define ERROR_H

#include "winapi.h"
#include "../resources/resource.h"

#include <stdarg.h>

#define MAX_EMSG 2048

#ifdef _DEBUG
	#define ePrint(...) eCon(__VA_ARGS__)
#else
	#define ePrint(...) eMsg(__VA_ARGS__)
#endif

#define ePrinti(x) ePrint(E(x))

enum emsg
{
	eInit,
	eCreate,
	eMemErr,
	eAsyncCmd,
	eLogFileOpen,
	eLogFileWrite,
	eDataRecord,
	
	exWindow,
	exWindowClass,
	exTrayIcon,
	exCmdEngine,
	exMouseLog,
	exMouseHook,
	exPathVar,
	
	emsg_num_of_messages
};

extern const char * restrict e_errorMessages[emsg_num_of_messages];

#define E(x) e_errorMessages[x]

// Define error messages
#define EINIT_STR          "Error initializing %s!"
#define ECREATE_STR        "Error creating %s!"
#define EMEM_ERR_STR       "Error allocating memory for %s!"
#define EASYNC_CMD_STR     "Failed to execute asynchronous command!"
#define ELOGFILE_OPEN_STR  "Error opening mouse data logging file!"
#define ELOGFILE_WRITE_STR "Error writing mouse log data to disk!"
#define EDATA_RECORD_STR   "Error adding data record to statistics!"

#define EXWINDOW_STR       "window"
#define EXWINDOW_CLASS_STR "window class"
#define EXTRAY_ICON_STR    "tray icon"
#define EXCMD_ENGINE_STR   "asynchronous command engine"
#define EXMOUSELOG_STR     "mouse logging"
#define EXMOUSEHOOK_STR    "low-level mouse hook"
#define EXPATH_VAR_STR     "path variable"

int eCon(const char * restrict format, ...);
int evCon(const char * restrict format, va_list ap);

int eMsg(const char * restrict format, ...);
int evMsg(const char * restrict format, va_list ap);


#endif
