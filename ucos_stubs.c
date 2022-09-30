#include "ucos_ii.h"

#include  <windows.h>
#include  <mmsystem.h>
#include  <stdio.h>
#include <pthread.h>
#include <time.h>

/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/

#define  WIN32_SLEEP                                        1u
#define  WIN32_MM_TMR                                       2u          /* Use the high resolution Multimedia timer.                */

#define  TIMER_METHOD                       WIN32_MM_TMR

#define  WIN_MM_MIN_RES                                     1u          /* Minimum timer resolution.                                */

#define  OS_MSG_TRACE                                       1u          /* Allow print trace messages.                              */

#ifdef  _MSC_VER
#define  MS_VC_EXCEPTION                           0x406D1388
#endif

#define MAXTHREADS 5
pthread_t OSTHREAD1;
pthread_t OSTHREAD2;
int Thread = 0;
void CPU_CRITICAL_ENTER()
{

}

void CPU_CRITICAL_EXIT()
{

}

void OSTimeISR()
{
    while(1)
    {
        int milli_seconds = 100;

        //Storing start time
        clock_t start_time = clock();

        // looping till required time is not achieved
        while (clock() < start_time + milli_seconds)
                ;
        OSTimeTick();
    }


}

void OSInitHookBegin()
{
    //int i;
    //char temp[256];
    //static BOOLEAN isInitialized = FALSE;

    //for(i=0;i<MAXTHREADS;i++)
    //{

    //}
    //OSTmrStart(1,OS_TMR_OPT_PERIODIC,OSTimeTick,NULL,"Timer",NULL);

    pthread_t Timer_Thread;
    pthread_create(Timer_Thread,NULL, OSTimeISR,NULL);
    pthread_detach(Timer_Thread);

    //pthread_t Interrupter_Thread;
   // pthread_create(&OSTHREAD[0], NULL, OSInterrupter, NULL);
   // pthread_t Scheduler_Thread;
   // pthread_create(&OSTHREAD[1], NULL, OScheduler, NULL);

}

void OSInitHookEnd()
{
    // pthread_exit(&OSTHREAD1);
    // pthread_exit(&OSTHREAD2);
    // pthread_exit(&OSTHREAD3);
    // pthread_exit(&OSTHREAD4);
    // pthread_exit(&OSTHREAD5);
}

void OSIntCtxSw()
{

}

void OSStartHighRdy()
{
    //OSTimeTickInit();

}




void OSTimeTickHook()
{

}

void OSTaskIdleHook()
{

}

void OSTaskDelHook(OS_TCB *ptcb)
{

}

void OSTCBInitHook(OS_TCB *ptcb)
{

}
/*$PAGE*/
/*
*********************************************************************************************************
*                                            CREATE A TASK
*
* Description: This function is used to have uC/OS-II manage the execution of a task.  Tasks can either
*              be created prior to the start of multitasking or by a running task.  A task cannot be
*              created by an ISR.
*
* Arguments  : task     is a pointer to the task's code
*
*              p_arg    is a pointer to an optional data area which can be used to pass parameters to
*                       the task when the task first executes.  Where the task is concerned it thinks
*                       it was invoked and passed the argument 'p_arg' as follows:
*
*                           void Task (void *p_arg)
*                           {
*                               for (;;) {
*                                   Task code;
*                               }
*                           }
*
*              ptos     is a pointer to the task's top of stack.  If the configuration constant
*                       OS_STK_GROWTH is set to 1, the stack is assumed to grow downward (i.e. from high
*                       memory to low memory).  'pstk' will thus point to the highest (valid) memory
*                       location of the stack.  If OS_STK_GROWTH is set to 0, 'pstk' will point to the
*                       lowest memory location of the stack and the stack will grow with increasing
*                       memory locations.
*
*              prio     is the task's priority.  A unique priority MUST be assigned to each task and the
*                       lower the number, the higher the priority.
*
* Returns    : OS_NO_ERR               if the function was successful.
*              OS_PRIO_EXIT            if the task priority already exist
*                                      (each task MUST have a unique priority).
*              OS_PRIO_INVALID         if the priority you specify is higher that the maximum allowed
*                                      (i.e. >= OS_LOWEST_PRIO)
*              OS_ERR_TASK_CREATE_ISR  if you tried to create a task from an ISR.
*********************************************************************************************************
*/
OS_STK *OSTaskStkInit(void (*task)(void *p_arg), void *p_arg, OS_STK *ptos, INT16U opt)
{
    OS_STK  *p_stk;
    p_stk = ptos++;
    switch(opt){

        case 0:
            //pthread_t OSTHREAD1;
            pthread_create(&OSTHREAD1,NULL,task,NULL);
            pthread_detach(OSTHREAD1);
            break;
        case 1:
            //pthread_t OSTHREAD2;
            pthread_create(&OSTHREAD2,NULL,task,NULL);
            pthread_detach(OSTHREAD2);
            break;
        default : /* Optional */
           OSInitHookEnd();
            p_stk = 0;
            break;
    }
    return ((OS_STK *)p_stk);
}

void OSTaskStatHook()
{

}

void OSTaskCreateHook(OS_TCB *ptcb)
{

}

void OSCtxSw()
{

}
