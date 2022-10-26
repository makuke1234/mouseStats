#include "app.h"
#include "../resources/resource.h"
#include "../gui/gui.h"

bool ms_init(msdata_t * restrict This, int argc, char ** argv)
{
	assert(This != NULL);
	assert(argc > 0);
	assert(argv != NULL);
	assert(argv[0] != NULL);

	*This = (msdata_t){
		.init         = false,
		.dpi          = USER_DEFAULT_SCREEN_DPI,
		.mHook        = NULL,
		.hwnd         = NULL,
		.titleRect    = { 0, 0, 0, 0 },
		.titleBrush   = NULL,
		.titleBrushInactive = NULL,
		.titleTextFont = NULL,
		.titleBtnFont  = NULL,
		.resizeEnable  = true,
		.isActive      = true,
		.rclick        = false,

		.contextMenu = NULL,
		.sysMenu     = NULL,
		.closeBtn    = NULL,
		.minBtn      = NULL
	};

	SetProcessDPIAware();

	INITCOMMONCONTROLSEX icex = { 0 };
	icex.dwSize = sizeof icex;
	icex.dwICC  = ICC_WIN95_CLASSES;

	InitCommonControlsEx(&icex);

	if (!ms_regClassBg(MOUSE_STATS_CLASS, &mgui_winProc, MS_DEFAULT_COLOR))
	{
		ePrint(E(eInit), E(exWindowClass));
		return false;
	}

	This->hwnd = CreateWindowExW(
		0,
		MOUSE_STATS_CLASS, MOUSE_STATS_TITLE,
		msWS_BORDERLESS & (DWORD)(~WS_MAXIMIZEBOX),
		CW_USEDEFAULT, CW_USEDEFAULT,
		400, 300,
		NULL, NULL, GetModuleHandleW(NULL), This
	);
	if (This->hwnd == NULL)
	{
		ePrint(E(eCreate), E(exWindow));
		return false;
	}

	
	if (ms_isCompositionEnabled())
	{
		const MARGINS shadow_state = { 1, 1, 1, 1 };
    	DwmExtendFrameIntoClientArea(This->hwnd, &shadow_state);
	}
	
	SetWindowPos(This->hwnd, NULL, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE);
	
	This->logpath = wcsdup(DEFAULT_LOG_PATH);
	if (This->logpath == NULL)
	{
		ePrint(E(eMemErr), E(exPathVar));
		DestroyWindow(This->hwnd);
		ms_free(This);
		return false;
	}
	if (!mh_recs_create(&This->mouseData, This->logpath))
	{
		ePrint(E(eInit), E(exMouseLog));
		DestroyWindow(This->hwnd);
		ms_free(This);
		return false;
	}

	This->mHook = mh_setHook(GetModuleHandleW(NULL), This->hwnd);
	if (This->mHook == NULL)
	{
		ePrint(E(eCreate), E(exMouseHook));
		DestroyWindow(This->hwnd);
		ms_free(This);
		return false;
	}
	
	if (!ace_init(&This->asyncCmd))
	{
		ePrint(E(eInit), E(exCmdEngine));
		DestroyWindow(This->hwnd);
		ms_free(This);
		return false;
	}

	ShowWindow(This->hwnd, SW_SHOW);
	UpdateWindow(This->hwnd);

	if (!ti_create(
		&This->tidata,
		MOUSE_STATS_TIP,
		MOUSE_STATS_TITLE,
		This->hwnd,
		LoadIconW(GetModuleHandleW(NULL), MAKEINTRESOURCEW(IDI_APPLICATION))
	))
	{
		ePrint(E(eCreate), E(exTrayIcon));
		DestroyWindow(This->hwnd);
		ms_free(This);
		return false;
	}

	This->init = true;
	return true;
}

void ms_loop(msdata_t * restrict This)
{
	assert(This != NULL);
	assert(This->init);

	MSG msg = { 0 };
	BOOL bret;
	while ((bret = GetMessageW(&msg, NULL, 0, 0)) != 0)
	{
		if (bret == -1)
		{
			return;
		}
		else if (!IsDialogMessageW(This->hwnd, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}
	}
}

void ms_free(msdata_t * restrict This)
{
	mh_removeHook();
	ti_destroy(&This->tidata);
	
	if (!mh_recs_destroy(&This->mouseData))
	{
		ePrinti(eLogFileWrite);
	}
	free(This->logpath);
	This->logpath = NULL;
	
	ace_destroy(&This->asyncCmd);

	This->init = false;
}
