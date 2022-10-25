#include "asyncCmdEngine.h"

static inline enum acr s_acmd_test(void * restrict cArg);

static ace_cmd_t s_ace_commands[acmd_num_of_commands] = {
	&s_acmd_test
};

static inline struct ace_info * s_getq(struct ace_info * restrict * restrict pq)
{
	assert(pq != NULL);
	
	// Remove from queue front
	if ((*pq) == NULL)
	{
		return NULL;
	}
	
	struct ace_info * q = (*pq);
	
	(*pq)->prev->next = (*pq)->next;
	(*pq)->next->prev = (*pq)->prev;
	(*pq) = ((*pq) == (*pq)->next) ? NULL : (*pq)->next;
	
	return q;
}
static inline bool s_putq(
	struct ace_info * restrict * restrict pq,
	enum acmd cmdType,
	void * restrict cArg,
	ace_callback_t cbFinish
)
{
	assert(pq != NULL);
	
	// Allocate memory
	struct ace_info * q = (struct ace_info *)malloc(sizeof(struct ace_info));
	if (q == NULL)
	{
		return false;
	}
	
	q->cmdType  = cmdType;
	q->cArg     = cArg;
	q->cbFinish = cbFinish;
	
	q->prev = q;
	q->next = q;
	
	// Add to pq
	if ((*pq) == NULL)
	{
		(*pq) = q;
	}
	else
	{
		// Add task to queue end
		q->next = (*pq);
		q->prev = (*pq)->prev;
		
		q->prev->next = q;
		(*pq)->prev   = q;
	}
	
	return true;
}

DWORD WINAPI ace_runnerThread(LPVOID lpArgs)
{
	ace_data_t * This = (ace_data_t *)lpArgs;
	assert(This != NULL);
	
	while (!This->kill)
	{
		while (!This->kill && (This->cmds == NULL))
		{
			SleepConditionVariableCS(&This->cvReady, &This->critSect, ACE_THREAD_DEFAULT_DELAY);
		}
		
		EnterCriticalSection(&This->critSect);
		
		struct ace_info * restrict q = s_getq(&This->cmds);
		
		LeaveCriticalSection(&This->critSect);
		
		if (q != NULL)
		{
			// Execute task
			
			enum acr result = ace_cmdSync(This, q->cmdType, q->cArg);
			if (q->cbFinish != NULL)
			{
				q->cbFinish(q->cArg, result);
			}
			
			free(q);
		}
	}
	
	return 0;
}

bool ace_init(ace_data_t * restrict This)
{
	assert(This != NULL);
	InitializeCriticalSection(&This->critSect);
	InitializeConditionVariable(&This->cvReady);
	
	This->kill = false;
	This->cmds = NULL;
	
	This->thread = CreateThread(
		NULL,
		ACE_THREAD_STACK_SIZE * sizeof(size_t),
		&ace_runnerThread,
		This,
		0,
		NULL
	);
	if (This->thread == NULL)
	{
		return false;
	}
	
	return true;
}
void ace_destroy(ace_data_t * restrict This)
{
	assert(This != NULL);
	if (This->thread != NULL)
	{
		This->kill = true;
		WakeConditionVariable(&This->cvReady);
		LeaveCriticalSection(&This->critSect);
		
		if (!WaitForSingleObject(This->thread, ACE_THREAD_DEFAULT_DELAY))
		{
			TerminateThread(This->thread, 0);
		}
		
		This->thread = NULL;
		DeleteCriticalSection(&This->critSect);
	}
}

enum acr ace_cmdSync(ace_data_t * restrict This, enum acmd cmd, void * restrict cArg)
{
	assert(This != NULL);
	
	if (cmd >= acmd_num_of_commands)
	{
		return acr_unknownCommand;
	}
	
	return s_ace_commands[cmd](cArg);
}
bool ace_cmdAsync(ace_data_t * This, enum acmd cmd, void * restrict cArg, ace_callback_t cbFinish)
{
	assert(This != NULL);
	
	if (This->thread == NULL)
	{
		return false;
	}
	
	EnterCriticalSection(&This->critSect);
	bool res = s_putq(&This->cmds, cmd, cArg, cbFinish);
	LeaveCriticalSection(&This->critSect);
	
	WakeConditionVariable(&This->cvReady);
	
	return res;
}

static inline enum acr s_acmd_test(void * restrict cArg)
{
	return acr_ok;
}
