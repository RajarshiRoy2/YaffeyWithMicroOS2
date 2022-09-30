/*
************************************************************************************************************************
*                                                      uC/OS-III
*                                                 The Real-Time Kernel
*
*                                        (c) Copyright 2005, Micrium, Weston, FL
*                                                  All Rights Reserved
*
*                                                   TIMER MANAGEMENT
*
* File    : OS_TMR.C
* By      : Jean J. Labrosse
* Version : V2.81
************************************************************************************************************************
*/
#ifndef OS_MASTER_FILE
#include <ucos_ii.h>
#endif

/*
************************************************************************************************************************
*                                                        NOTES
*
* 1) Your application MUST define the following #define constants:
*
*    OS_TASK_TMR_PRIO          The priority of the Timer management task
*    OS_TASK_TMR_STK_SIZE      The size     of the Timer management task's stack
*
* 2) You must call OSTmrSignal() to notify the Timer management task that it's time to update the timers.
************************************************************************************************************************
*/

/*
************************************************************************************************************************
*                                                  LOCAL PROTOTYPES
************************************************************************************************************************
*/

#if OS_TMR_EN > 0
static  OS_TMR  *OSTmr_Alloc     (void);
static  void     OSTmr_Free      (OS_TMR *ptmr);
static  void     OSTmr_InitTask  (void);
static  void     OSTmr_Link      (OS_TMR *ptmr);
static  void     OSTmr_Unlink    (OS_TMR *ptmr);
static  void     OSTmr_Lock      (void);
static  void     OSTmr_Unlock    (void);
static  void     OSTmr_Task      (void   *p_arg);
#endif

/*$PAGE*/
/*
************************************************************************************************************************
*                                    GET HOW MUCH TIME IS LEFT BEFORE A TIMER EXPIRES
*
* Description: This function is called to get the number of ticks before a timer times out.
*
* Arguments  : ptmr          Is a pointer to the timer to obtain the remaining time from.
*
*              perr          Is a pointer to an error code.  '*perr' will contain one of the following:
*                               OS_NO_ERR
*                               OS_ERR_TMR_ISR            Called from an ISR
*                               OS_ERR_TMR_INVALID_DEST  'pdest' is a NULL pointer
*                               OS_ERR_TMR_INVALID       'ptmr'  is a NULL pointer
*                               OS_ERR_TMR_INACTIVE      'ptmr'  points to a timer that is not active
*
* Returns    : A pointer to the ptmr->OSTmrName field.
************************************************************************************************************************
*/

#if OS_TMR_EN > 0 && OS_TMR_CFG_NAME_SIZE > 0
void  OSTmrGetName (OS_TMR  *ptmr,
                    INT8U   *pdest,
                    INT8U   *perr)
{
#if OS_ARG_CHK_EN > 0
    if (perr == (INT8U *)0) {
        return;
    }
    if (pdest == (INT8U *)0) {
        *perr = OS_ERR_TMR_INVALID_DEST;
        return;
    }
    if (ptmr == (OS_TMR *)0) {
        *perr = OS_ERR_TMR_INVALID;
        return;
    }
#endif
    if (OSIntNesting > 0) {                            /* See if trying to call from an ISR                           */
        *perr  = OS_ERR_TMR_ISR;
        return;
    }
    OSTmr_Lock();
    if (ptmr->OSTmrActive == FALSE) {
        OSTmr_Unlock();
        *perr = OS_ERR_TMR_INACTIVE;
        return;
    }
    OS_StrCopy(pdest, ptmr->OSTmrName);
    OSTmr_Unlock();
    *perr  = OS_NO_ERR;
}
#endif

/*$PAGE*/
/*
************************************************************************************************************************
*                                    GET HOW MUCH TIME IS LEFT BEFORE A TIMER EXPIRES
*
* Description: This function is called to get the number of ticks before a timer times out.
*
* Arguments  : ptmr          Is a pointer to the timer to obtain the remaining time from.
*
*              perr          Is a pointer to an error code.  '*perr' will contain one of the following:
*                               OS_NO_ERR
*                               OS_ERR_TMR_ISR          Called from an ISR
*                               OS_ERR_TMR_INVALID     'ptmr' is a NULL pointer
*                               OS_ERR_TMR_INACTIVE    'ptmr' points to a timer that is not active
*
* Returns    : The time remaining for the timer to expire.  The time represents 'timer' increments.  In other words, if
*              OSTmr_Task() is signaled every 1/10 of a second then the returned value represents the number of 1/10 of
*              a second remaining before the timer expires.
************************************************************************************************************************
*/

#if OS_TMR_EN > 0
INT32U  OSTmrGetRemain (OS_TMR  *ptmr,
                        INT8U   *perr)
{
    INT32U  remain;


    if (OSIntNesting > 0) {                            /* See if trying to call from an ISR                           */
        return (OS_ERR_TMR_ISR);
    }
#if OS_ARG_CHK_EN > 0
    if (ptmr == (OS_TMR *)0) {
        *perr = OS_ERR_TMR_INVALID;
        return (0);
    }
    if (perr == (INT8U *)0) {
        return (0);
    }
#endif
    OSTmr_Lock();
    if (ptmr->OSTmrActive == FALSE) {
        OSTmr_Unlock();
        *perr = OS_ERR_TMR_INACTIVE;
        return (0);
    }
    remain = ptmr->OSTmrMatch - OSTmrTime;             /* Determine how much time is left to timeout                  */
    OSTmr_Unlock();
    *perr  = OS_NO_ERR;
    return (remain);
}
#endif

/*$PAGE*/
/*
************************************************************************************************************************
*                                                   START A TIMER
*
* Description: This function is called by your application code to create and start a timer.
*
* Arguments  : period        The time (in OS ticks) before the timer expires.
*                               If you specified 'OS_TMR_OPT_PERIODIC' as an option, when the timer expires, it will
*                               automatically restart with the same period.
*
*              opt           Specifies either:
*                               OS_TMR_OPT_ONE_SHOT       The timer counts down only once and then is deleted
*                               OS_TMR_OPT_PERIODIC       The timer counts down and then reloads itself
*
*              callback      Is a pointer to a callback function that will be called when the timer expires.  The
*                               callback function must be declared as follows:
*
*                               void MyCallback (OS_TMR *ptmr, void *p_arg);
*
*              callback_arg  Is an argument (a pointer) that is passed to the callback function when it is called.
*
*              pname         Is a pointer to an ASCII string that is used to name the timer.  Names are useful for
*                               debugging.  The length of the ASCII string for the name is given by:
*
*                               OS_TMR_CFG_NAME_SIZE and should be found in OS_CFG.H
*
*              perr          Is a pointer to an error code.  '*perr' will contain one of the following:
*                               OS_NO_ERR
*                               OS_ERR_TMR_INVALID_PERIOD
*                               OS_ERR_TMR_INVALID_OPT
*                               OS_ERR_TMR_NON_AVAIL
*
* Returns    : A pointer to an OS_TMR data structure.  This is the 'handle' that you application will use to reference
*              the timer created/started.
************************************************************************************************************************
*/

#if OS_TMR_EN > 0
OS_TMR  *OSTmrStart (INT32U           period,
                     INT8U            opt,
                     OS_TMR_CALLBACK  callback,
                     void            *callback_arg,
                     INT8U           *pname,
                     INT8U           *perr)
{
#if OS_TMR_CFG_NAME_SIZE > 0
    INT8U         len;
#endif
    OS_TMR       *ptmr;


#if OS_ARG_CHK_EN > 0
    if (perr == (INT8U *)0) {                               /* Validate arguments                                     */
        return (OS_TMR *)0;
    }
    if (period == 0) {
        *perr = OS_ERR_TMR_INVALID_PERIOD;
        return (OS_TMR *)0;
    }
    if (opt != OS_TMR_OPT_ONE_SHOT && opt != OS_TMR_OPT_PERIODIC) {
        *perr = OS_ERR_TMR_INVALID_OPT;
        return (OS_TMR *)0;
    }
#endif
    OSTmr_Lock();
    ptmr = OSTmr_Alloc();                                   /* Obtain a timer from the free pool                      */
    if (ptmr == (OS_TMR *)0) {
        OSTmr_Unlock();
        *perr = OS_ERR_TMR_NON_AVAIL;
        return ((OS_TMR *)0);
    }
    ptmr->OSTmrActive      = TRUE;                          /* Indicate that timer is active                          */
    ptmr->OSTmrPeriod      = period;
    ptmr->OSTmrOpt         = opt;
    ptmr->OSTmrCallback    = callback;
    ptmr->OSTmrCallbackArg = callback_arg;
#if OS_TMR_CFG_NAME_SIZE > 0
    if (pname !=(INT8U *)0) {
        len                = OS_StrLen(pname);              /* Copy timer name                                        */
        if (len < OS_TMR_CFG_NAME_SIZE) {
            (void)OS_StrCopy(ptmr->OSTmrName, pname);
        }
    }
#endif

    OSTmr_Link(ptmr);                                       /* Link timer to timer wheel                              */

    OSTmr_Unlock();

    *perr = OS_NO_ERR;

    return (ptmr);
}
#endif

/*$PAGE*/
/*
************************************************************************************************************************
*                                                   STOP A TIMER
*
* Description: This function is called by your application code to stop and delete a timer.
*
* Arguments  : ptmr          Is a pointer to the timer to stop and delete.
*
*              opt           Allows you to specify an option to this functions which can be:
*
*                               OS_TMR_OPT_NONE          Do nothing special but stop the timer
*                               OS_TMR_OPT_CALLBACK      Execute the callback function, pass it the callback argument
*                                                        specified when the timer was created.
*                               OS_TMR_OPT_CALLBACK_ARG  Execute the callback function, pass it the callback argument
*                                                        specified in THIS function call
*
*              callback_arg  Is a pointer to a 'new' callback argument that can be passed to the callback function
*                               instead of the timer's callback argument.  In other words, use 'callback_arg' passed in
*                               THIS function INSTEAD of ptmr->OSTmrCallbackArg
*
*              perr          Is a pointer to an error code.  '*perr' will contain one of the following:
*                               OS_NO_ERR
*                               OS_ERR_TMR_INVALID
*
* Returns    : none
************************************************************************************************************************
*/

#if OS_TMR_EN > 0
void  OSTmrStop (OS_TMR  *ptmr,
                 INT8U    opt,
                 void    *callback_arg,
                 INT8U   *perr)
{
    OS_TMR_CALLBACK  pfnct;


#if OS_ARG_CHK_EN > 0
    if (perr == (INT8U *)0) {                               /* Validate arguments                                     */
        return;
    }
    if (ptmr == (OS_TMR *)0) {
        *perr = OS_ERR_TMR_INVALID;
        return;
    }
#endif
    OSTmr_Lock();
    OSTmr_Unlink(ptmr);                                     /* Remove from current wheel spoke                        */
    *perr = OS_NO_ERR;
    switch (opt) {
        case OS_TMR_OPT_CALLBACK:
             pfnct = ptmr->OSTmrCallback;                   /* Execute callback function if available ...             */
             if (pfnct != (OS_TMR_CALLBACK)0) {
                 (*pfnct)(ptmr, ptmr->OSTmrCallbackArg);    /* ... using the 'argument' specified @ timer start       */
             }
             break;

        case OS_TMR_OPT_CALLBACK_ARG:
             pfnct = ptmr->OSTmrCallback;                   /* Execute callback function if available ...             */
             if (pfnct != (OS_TMR_CALLBACK)0) {
                 (*pfnct)(ptmr, callback_arg);              /* ... using the 'callback_arg' provided in this function */
             }
             break;

        case OS_TMR_OPT_NONE:
             break;

        default:
            *perr = OS_ERR_TMR_INVALID_OPT;
            break;
    }
    OSTmr_Free(ptmr);                                       /* Return timer to free list of timers                    */
    OSTmr_Unlock();
    *perr = OS_NO_ERR;
}
#endif

/*$PAGE*/
/*
************************************************************************************************************************
*                                      SIGNAL THAT IT'S TIME TO UPDATE THE TIMERS
*
* Description: This function is typically called by the ISR that occurs at the timer tick rate and is used to signal to
*              OSTmr_Task() that it's time to update the timers.
*
* Arguments  : none
*
* Returns    : none
************************************************************************************************************************
*/

#if OS_TMR_EN > 0
void  OSTmrSignal (void)
{
    OSSemPost(OSTmrSemSignal);
}
#endif

/*$PAGE*/
/*
************************************************************************************************************************
*                                               ALLOCATE AND FREE A TIMER
*
* Description: This function is called to allocate a timer.
*
* Arguments  : none
*
* Returns    : a pointer to a timer if one is available
************************************************************************************************************************
*/

#if OS_TMR_EN > 0
static  OS_TMR  *OSTmr_Alloc (void)
{
    OS_TMR *ptmr;


    if (OSTmrFreeList == (OS_TMR *)0) {
        return ((OS_TMR *)0);
    }
    ptmr            = OSTmrFreeList;
    OSTmrFreeList   = ptmr->OSTmrNext;
    ptmr->OSTmrNext = (OS_TCB *)0;
    ptmr->OSTmrPrev = (OS_TCB *)0;
    OSTmrUsed++;
    OSTmrFree--;
    return (ptmr);
}



/*
************************************************************************************************************************
*                                             RETURN A TIMER TO THE FREE LIST
*
* Description: This function is called to return a timer object to the free list of timers.
*
* Arguments  : ptmr     is a pointer to the timer to free
*
* Returns    : none
************************************************************************************************************************
*/

static  void  OSTmr_Free (OS_TMR *ptmr)
{
    ptmr->OSTmrActive      = FALSE;                    /* Clear timer object fields                                   */
    ptmr->OSTmrOpt         = OS_TMR_OPT_NONE;
    ptmr->OSTmrPeriod      = 0;
    ptmr->OSTmrMatch       = 0;
    ptmr->OSTmrCallback    = (OS_TMR_CALLBACK)0;
    ptmr->OSTmrCallbackArg = (void *)0;
#if OS_TMR_CFG_NAME_SIZE > 1
    OS_StrCopy(ptmr->OSTmrName, "?");
#endif

    ptmr->OSTmrPrev        = (OS_TCB *)0;              /* Chain timer to free list                                    */
    ptmr->OSTmrNext        = OSTmrFreeList;
    OSTmrFreeList          = ptmr;

    OSTmrUsed--;                                       /* Update timer object statistics                              */
    OSTmrFree++;
}
#endif

/*$PAGE*/
/*
************************************************************************************************************************
*                                                    INITIALIZATION
*                                          INITIALIZE THE FREE LIST OF TIMERS
*
* Description: This function is called by OSInit() to initialize the free list of OS_TMRs.
*
* Arguments  : none
*
* Returns    : none
************************************************************************************************************************
*/

#if OS_TMR_EN > 0
void  OSTmr_Init (void)
{
#if OS_EVENT_NAME_SIZE > 10
    INT8U    err;
#endif
    INT16U   i;
    OS_TMR  *ptmr1;
    OS_TMR  *ptmr2;


    OS_MemClr((INT8U *)&OSTmrTbl[0],      sizeof(OSTmrTbl));            /* Clear all the TMRs                         */
    OS_MemClr((INT8U *)&OSTmrWheelTbl[0], sizeof(OSTmrWheelTbl));       /* Clear the timer wheel                      */

    ptmr1 = &OSTmrTbl[0];
    ptmr2 = &OSTmrTbl[1];
    for (i = 0; i < (OS_TMR_CFG_MAX - 1); i++) {                        /* Init. list of free TMRs                    */
        ptmr1->OSTmrActive  = FALSE;                                    /* Indicate that timer is inactive            */
        ptmr1->OSTmrNext    = (void *)ptmr2;                            /* Link to next timer                         */
#if OS_TMR_NAME_SIZE > 1
        ptmr1->OSTmrName[0] = '?';                                      /* Unknown name                               */
        ptmr1->OSTmrName[1] = OS_ASCII_NUL;
#endif
        ptmr1++;
        ptmr2++;
    }
    ptmr1->OSTmrActive  = FALSE;                                        /* Indicate that timer is inactive            */
    ptmr1->OSTmrNext    = (void *)0;                                    /* Last OS_TMR                                */
#if OS_TMR_CFG__SIZE > 1
    ptmr1->OSTmrName[0] = '?';                                          /* Unknown name                               */
    ptmr1->OSTmrName[1] = OS_ASCII_NUL;
#endif
    OSTmrTime           = 0;
    OSTmrUsed           = 0;
    OSTmrFree           = OS_TMR_CFG_MAX;
    OSTmrFreeList       = &OSTmrTbl[0];
    OSTmrSem            = OSSemCreate(1);
    OSTmrSemSignal      = OSSemCreate(0);

#if OS_EVENT_NAME_SIZE > 18
    OSEventNameSet(OSTmrSem,       "uC/OS-II TmrLock",   &err);         /* Assign names to semaphores                 */
#else
#if OS_EVENT_NAME_SIZE > 10
    OSEventNameSet(OSTmrSem,       "OS-TmrLock",         &err);
#endif
#endif

#if OS_EVENT_NAME_SIZE > 18
    OSEventNameSet(OSTmrSemSignal, "uC/OS-II TmrSignal", &err);
#else
#if OS_EVENT_NAME_SIZE > 10
    OSEventNameSet(OSTmrSemSignal, "OS-TmrSig",          &err);
#endif
#endif

    OSTmr_InitTask();
}
#endif

/*$PAGE*/
/*
************************************************************************************************************************
*                                          INITIALIZE THE TIMER MANAGEMENT TASK
*
* Description: This function is called by OSTmrInit() to create the timer management task.
*
* Arguments  : none
*
* Returns    : none
************************************************************************************************************************
*/

#if OS_TMR_EN > 0
static  void  OSTmr_InitTask (void)
{
#if OS_TASK_NAME_SIZE > 6
    INT8U  err;
#endif


#if OS_TASK_CREATE_EXT_EN > 0
    #if OS_STK_GROWTH == 1
    (void)OSTaskCreateExt(OSTmr_Task,
                          (void *)0,                                       /* No arguments passed to OSTmrTask()      */
                          &OSTmrTaskStk[OS_TASK_TMR_STK_SIZE - 1],         /* Set Top-Of-Stack                        */
                          OS_TASK_TMR_PRIO,
                          OS_TASK_TMR_ID,
                          &OSTmrTaskStk[0],                                /* Set Bottom-Of-Stack                     */
                          OS_TASK_TMR_STK_SIZE,
                          (void *)0,                                       /* No TCB extension                        */
                          OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR);      /* Enable stack checking + clear stack     */
    #else
    (void)OSTaskCreateExt(OSTmr_Task,
                          (void *)0,                                       /* No arguments passed to OSTmrTask()      */
                          &OSTmrTaskStk[0],                                /* Set Top-Of-Stack                        */
                          OS_TASK_TMR_PRIO,
                          OS_TASK_TMR_ID,
                          &OSTmrTaskStk[OS_TASK_TMR_STK_SIZE - 1],         /* Set Bottom-Of-Stack                     */
                          OS_TASK_TMR_STK_SIZE,
                          (void *)0,                                       /* No TCB extension                        */
                          OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR);      /* Enable stack checking + clear stack     */
    #endif
#else
    #if OS_STK_GROWTH == 1
    (void)OSTaskCreate(OSTmr_Task,
                       (void *)0,
                       &OSTmrTaskStk[OS_TASK_TMR_STK_SIZE - 1],
                       OS_TASK_TMR_PRIO);
    #else
    (void)OSTaskCreate(OSTmr_Task,
                       (void *)0,
                       &OSTmrTaskStk[0],
                       OS_TASK_TMR_PRIO);
    #endif
#endif

#if OS_TASK_NAME_SIZE > 12
    OSTaskNameSet(OS_TASK_TMR_PRIO, (INT8U *)"uC/OS-II Tmr", &err);
#else
#if OS_TASK_NAME_SIZE > 6
    OSTaskNameSet(OS_TASK_TMR_PRIO, (INT8U *)"OS-Tmr", &err);
#endif
#endif
}
#endif

/*$PAGE*/
/*
************************************************************************************************************************
*                                         INSERT A TIMER INTO THE TIMER WHEEL
*
* Description: This function is called to insert the timer into the timer wheel.  The timer is always inserted at the
*              beginning of the list.
*
* Arguments  : ptmr          Is a pointer to the timer to insert.
*
* Returns    : none
************************************************************************************************************************
*/

#if OS_TMR_EN > 0
static  void  OSTmr_Link (OS_TMR *ptmr)
{
    OS_TMR       *ptmr1;
    OS_TMR_WHEEL *pspoke;
    INT16U        spoke;


    ptmr->OSTmrMatch       = ptmr->OSTmrPeriod + OSTmrTime;        /* Determine when timer will expire                */
    spoke                  = ptmr->OSTmrMatch % OS_TMR_CFG_WHEEL_SIZE;
    pspoke                 = &OSTmrWheelTbl[spoke];

    if (pspoke->OSTmrFirst == (OS_TMR *)0) {                       /* Link into timer wheel                           */
        pspoke->OSTmrFirst   = ptmr;
        ptmr->OSTmrNext      = (OS_TMR *)0;
        pspoke->OSTmrEntries = 1;
    } else {
        ptmr1                = pspoke->OSTmrFirst;                 /* Point to first timer in the spoke               */
        pspoke->OSTmrFirst   = ptmr;
        ptmr->OSTmrNext      = (void *)ptmr1;
        ptmr1->OSTmrPrev     = (void *)ptmr;
        pspoke->OSTmrEntries++;
    }
    ptmr->OSTmrPrev = (void *)0;                                   /* Timer always inserted as first node in list     */
}
#endif

/*$PAGE*/
/*
************************************************************************************************************************
*                                         REMOVE A TIMER FROM THE TIMER WHEEL
*
* Description: This function is called to remove the timer from the timer wheel.
*
* Arguments  : ptmr          Is a pointer to the timer to remove.
*
* Returns    : none
************************************************************************************************************************
*/

#if OS_TMR_EN > 0
static  void  OSTmr_Unlink (OS_TMR *ptmr)
{
    OS_TMR        *ptmr1;
    OS_TMR        *ptmr2;
    OS_TMR_WHEEL  *pspoke;
    INT16U         spoke;


    spoke  = ptmr->OSTmrMatch % OS_TMR_CFG_WHEEL_SIZE;
    pspoke = &OSTmrWheelTbl[spoke];

    if (pspoke->OSTmrFirst == ptmr) {                       /* See if timer to remove is at the beginning of list     */
        ptmr1              = (OS_TMR *)ptmr->OSTmrNext;
        pspoke->OSTmrFirst = ptmr1;
        if (ptmr1 != (OS_TMR *)0) {
            ptmr1->OSTmrPrev = (void *)0;
        }
    } else {
        ptmr1            = ptmr->OSTmrPrev;                 /* Remove timer from somewhere in the list                */
        ptmr2            = ptmr->OSTmrNext;
        ptmr1->OSTmrNext = ptmr2;
        if (ptmr2 != (OS_TMR *)0) {
            ptmr2->OSTmrPrev = ptmr1;
        }
    }
    ptmr->OSTmrNext       = (void *)0;
    ptmr->OSTmrPrev       = (void *)0;
    pspoke->OSTmrEntries--;
}
#endif

/*$PAGE*/
/*
************************************************************************************************************************
*                                       TIMER MANAGER DATA STRUCTURE LOCKING MECHANISM
*
* Description: These functions are used to gain exclusive access to timer management data structures.
*
* Arguments  : none
*
* Returns    : none
************************************************************************************************************************
*/

#if OS_TMR_EN > 0
static  void  OSTmr_Lock (void)
{
    INT8U  err;


    OSSemPend(OSTmrSem, 0, &err);
}



static  void  OSTmr_Unlock (void)
{
    OSSemPost(OSTmrSem);
}
#endif

/*$PAGE*/
/*
************************************************************************************************************************
*                                                 TIMER MANAGEMENT TASK
*
* Description: This task is created by OSTmrInit().
*
* Arguments  : none
*
* Returns    : none
************************************************************************************************************************
*/

#if OS_TMR_EN > 0
static  void  OSTmr_Task (void *p_arg)
{
    INT8U            err;
    OS_TMR          *ptmr;
    OS_TMR          *ptmr_next;
    OS_TMR_CALLBACK  pfnct;
    OS_TMR_WHEEL    *pspoke;
    INT16U           spoke;


    (void)p_arg;                                                 /* Not using 'p_arg', prevent compiler warning       */
    while (1) {
        OSSemPend(OSTmrSemSignal, 0, &err);                      /* Wait for signal indicating time to update timers  */
        OSTmr_Lock();
        OSTmrTime++;                                             /* Increment the current time                        */
        spoke  = OSTmrTime % OS_TMR_CFG_WHEEL_SIZE;              /* Position on current timer wheel entry             */
        pspoke = &OSTmrWheelTbl[spoke];
        ptmr   = pspoke->OSTmrFirst;
        while (ptmr != (OS_TMR *)0) {
            ptmr_next = ptmr->OSTmrNext;                         /* Point to next timer to update because current ... */
                                                                 /* ... timer could get unlinked from the wheel.      */
            if (OSTmrTime == ptmr->OSTmrMatch) {                 /* Process each timer that expires                   */
                pfnct = ptmr->OSTmrCallback;                     /* Execute callback function if available            */
                if (pfnct != (OS_TMR_CALLBACK)0) {
                    (*pfnct)(ptmr, ptmr->OSTmrCallbackArg);
                }
                OSTmr_Unlink(ptmr);                              /* Remove from current wheel spoke                   */
                switch (ptmr->OSTmrOpt) {
                    case OS_TMR_OPT_PERIODIC:
                         OSTmr_Link(ptmr);                       /* Recalculate new position of timer in wheel        */
                         break;

                    case OS_TMR_OPT_ONE_SHOT:
                    default:
                         OSTmr_Free(ptmr);                       /* Return timer to free list                         */
                         break;
                }
            }
            ptmr = ptmr_next;
        }
        OSTmr_Unlock();
    }
}
#endif
