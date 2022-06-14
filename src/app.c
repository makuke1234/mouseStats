#include "app.h"

bool ms_init(msdata_t * restrict This, int argc, char ** argv)
{
	assert(This != NULL);
	assert(argc > 0);
	assert(argv != NULL);
	assert(argv[0] != NULL);
	
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
	
}