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

pthread_t OSTHREAD1;
pthread_t OSTHREAD2;

//struct YaffeyItem Root;
//struct YaffeyItem Node;
//struct yaffs_obj_hdr mYaffsObjectHeader;

//void makeDirty(struct YaffeyItem SelfNode) {
//    if (SelfNode.mCondition == CLEAN) {
//        SelfNode.mCondition = DIRTY;
//    }
//}


//void setName(char *name[],struct YaffeyItem SelfNode) {
//    if (sizeof(*name) > 0) {
//        char *newName = *name;
//        if (sizeof(*newName) > YAFFS_MAX_NAME_LENGTH) {
//            unsigned long long int Size = sizeof(*newName)-YAFFS_MAX_NAME_LENGTH;
//            newName[sizeof(*newName)-Size] = 0;
//        }
//        size_t len = (size_t)(sizeof(*newName));
//        char currentName[YAFFS_MAX_NAME_LENGTH + 1];
//        strncpy(currentName,SelfNode.mYaffsObjectHeader.name,len);//issue here

//        if (newName != currentName) {
//            memset(SelfNode.mYaffsObjectHeader.name, 0, YAFFS_MAX_NAME_LENGTH);
//            memcpy(SelfNode.mYaffsObjectHeader.name, newName, len);
//            makeDirty(SelfNode);
//        }
//    } else {
//        memset(SelfNode.mYaffsObjectHeader.name, 0, YAFFS_MAX_NAME_LENGTH);
//    }
//}


//struct YaffeyItem YaffsItem(struct YaffeyItem* parent, const char *name[], enum yaffs_obj_type type,struct YaffeyItem SelfNode) {
//    //parent->mParentItem = parent;

//    memset(&SelfNode.mYaffsObjectHeader, 0xff, sizeof(struct yaffs_obj_hdr));
//    setName(name, SelfNode);
//    SelfNode.mYaffsObjectHeader.type = type;
//    SelfNode.mYaffsObjectHeader.yst_ctime = OSTimeGet();
//    SelfNode.mYaffsObjectHeader.yst_atime = mYaffsObjectHeader.yst_ctime;
//    SelfNode.mYaffsObjectHeader.yst_mtime = mYaffsObjectHeader.yst_ctime;

//    SelfNode.mHeaderPosition = -1;
//    SelfNode.mYaffsObjectId = -1;

//    SelfNode.mCondition = NEW;
//}

//struct YaffeyItem* MakeRoot()
//{
//    Root = YaffsItem(NULL, "", YAFFS_OBJECT_TYPE_DIRECTORY,Root);


//    Root.mYaffsObjectId = YAFFS_OBJECTID_ROOT;
//    Root.mYaffsObjectHeader.parent_obj_id = Root.mYaffsObjectId;
//    Root.mYaffsObjectHeader.yst_mode = 0771 | 0x4000;
//    Root.mYaffsObjectHeader.yst_uid = 0;
//    Root.mYaffsObjectHeader.yst_gid = 0;

//    Root.mHeaderPosition = -1;
//    Root.mYaffsObjectId = -1;

//    Root.mCondition = NEW;
//    return &Root;

//}
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

    //OS_STK  *p_stk;
    //p_stk = ptos++;
    ptos = &task;

    task(p_arg);

    //pthread_create(&OSTHREAD2,NULL,task,NULL);
    //pthread_join(OSTHREAD2,NULL);
    //switch(opt){

   //     case 0:
   //         MakeRoot();
   //         break;
   //     case 1:
   //         pthread_create(&OSTHREAD2,NULL,task,NULL);
   //         pthread_join(OSTHREAD2,NULL);
   //         break;
   //     default : /* Optional */
   //        OSInitHookEnd();
   //         p_stk = 0;
   //         break;
    //}
    return ptos;
}

void CPU_CRITICAL_ENTER()
{

}

void CPU_CRITICAL_EXIT()
{

}

void OSInitHookBegin()
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

}




void OSTimeTickHook()
{


}
//need to break for loop in Idletask
void OSTaskIdleHook()
{

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
