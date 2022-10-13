#include "ucos_ii.h"
#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>
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
    OS_TCB *ptcb = OSTCBPrioTbl[0];
    OS_STK *FunctionPointer = ptcb->OSTCBStkPtr;
    //int opt = OSTCBHighRdy->OSTCBStkPtr + 1;
    //void *p_arg = OSTCBHighRdy->OSTCBStkPtr + 2;
    //pthread_create(OSTHREAD2,NULL,&fun_ptr,NULL);
}

void OSTimeTickHook(){}

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
