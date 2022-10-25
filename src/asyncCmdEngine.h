#ifndef ASYNC_CMD_ENGINE_H
#define ASYNC_CMD_ENGINE_H

#include "winapi.h"
#include <stdint.h>
#include <stdbool.h>

enum acmd
{
	acmd_test,
	
	
	acmd_num_of_commands
};

enum acr
{
	acr_ok,
	acr_unknownCommand,
	acr_memoryError
};

typedef void (* ace_callback_t)(void * restrict cArg, enum acr result);
typedef enum acr (* ace_cmd_t)(void * restrict cArg);

struct ace_info
{
	enum acmd cmdType;
	void * restrict cArg;
	ace_callback_t cbFinish;

	// Mandatory variables
	struct ace_info * prev;
	struct ace_info * next;
};

typedef struct ace_data
{
	CRITICAL_SECTION critSect;
	CONDITION_VARIABLE cvReady;

	volatile HANDLE thread;
	volatile bool kill;
	
	struct ace_info * cmds;
	
} ace_data_t;


#define ACE_THREAD_STACK_SIZE    4096
#define ACE_THREAD_DEFAULT_DELAY  100

DWORD WINAPI ace_runnerThread(LPVOID lpArgs);

bool ace_init(ace_data_t * restrict This);
void ace_destroy(ace_data_t * This);

enum acr ace_cmdSync (ace_data_t * restrict This, enum acmd cmd, void * restrict cArg);
bool     ace_cmdAsync(ace_data_t *          This, enum acmd cmd, void * restrict cArg, ace_callback_t cbFinish);

#endif
