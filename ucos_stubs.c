#include "ucos_ii.h"
#include  <windows.h>
#include  <mmsystem.h>
#include  <stdio.h>
#include <pthread.h>
#include <time.h>
#include <stdbool.h>
#include "yaffs2/yaffs_packedtags2.h"
#include "yaffs2/yaffs_guts.h"
#include "yaffs2/yaffs_ecc.h"
#include <stdint.h>
#include <stddef.h>
#include <signal.h>
#include "shared_memory.h"
pthread_t OSTHREAD1;
pthread_t OSTHREAD2;

//#define  TASK0_STK_SIZE                         250
//CPU_STK  Task0Stk[TASK0_STK_SIZE];

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
    int Index;
    *ptos = (int)(size_t)p_arg;//strong variables
    *(ptos - 1) = opt;
    *(ptos - 2) = &task;//storing pointer address of function

    pthread_t TASKthread;
    pthread_t TASKthread2;

    switch(opt)
    {
        case 0:
            pthread_create(&TASKthread,NULL,task,NULL);
            pthread_detach(TASKthread);
            break;
        default:
            pthread_create(&OSTHREAD1,NULL,task,NULL);
            pthread_detach(OSTHREAD1);
            break;
    }
    return ptos - 2;
}

void CPU_CRITICAL_ENTER(){}

void CPU_CRITICAL_EXIT(){}


void OSInitHookBegin()//OS initalization functions
{

}

void OSInitHookEnd()
{

}

void OSIntCtxSw()
{

}

void OSStartHighRdy()
{
    OSRunning = true;
    OS_STK *FunctionPointer = OSTCBHighRdy->OSTCBStkPtr;
    int opt = OSTCBHighRdy->OSTCBStkPtr + 1;
    void *p_arg = OSTCBHighRdy->OSTCBStkPtr + 2;
    //pthread_create(OSTHREAD1,NULL,&FunctionPointer,NULL);
}




void OSTimeTickHook()
{


}

void sig_handler(int signum)
{
  //if (signum == SIGINT)
  //{
    OSStartHighRdy();
    pthread_create(&OSTHREAD1,NULL,Task1,NULL);
 // }

}

//need to break for loop in Idletask
void OSTaskIdleHook()
{

    while(OSRunning)
    {
        // be here and pause idle task
    }

}

void OSTaskDelHook(OS_TCB *ptcb)
{

}

void OSTCBInitHook(OS_TCB *ptcb)
{

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
