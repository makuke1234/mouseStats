#include "app.h"
#include "resource.h"
#include "gui.h"
#include "mouseHook.h"

bool ms_init(msdata_t * restrict This, int argc, char ** argv)
{
	assert(This != NULL);
	assert(argc > 0);
	assert(argv != NULL);
	assert(argv[0] != NULL);

	*This = (msdata_t){
		.init  = false,
		.mHook = NULL,
		.hwnd  = NULL
	};

	if (!ms_regClass(MOUSE_STATS_CLASS, &mgui_winProc))
	{
		ePrint("Error initializing windows class!");
		return false;
	}

	This->hwnd = CreateWindowExW(
		0,
		MOUSE_STATS_CLASS, MOUSE_STATS_TITLE,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, 400, 300,
		NULL, NULL, GetModuleHandleW(NULL), This
	);
	if (This->hwnd == NULL)
	{
		ePrint("Error creating window!");
		return false;
	}

	This->mHook = mmh_setHook(GetModuleHandleW(NULL), This->hwnd);
	if (This->mHook == NULL)
	{
		ePrint("Error creating low-level mouse hook!");
		DestroyWindow(This->hwnd);
		This->hwnd = NULL;
		return false;
	}

	ShowWindow(This->hwnd, SW_SHOW);
	UpdateWindow(This->hwnd);

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

		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}
}

void ms_free(msdata_t * restrict This)
{
	mmh_removeHook();
	This->init = false;
}
