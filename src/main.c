#include "app.h"


int main(int argc, char * argv[])
{
	msdata_t data = { 0 };
	if (!ms_init(&data, argc, argv))
	{
		return -1;
	}
	
	// Run main application loop
	ms_loop(&data);
	
	// Free everything
	ms_free(&data);
	
	return 0;
}
