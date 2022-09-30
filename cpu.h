/*
*********************************************************************************************************
*                                              uC/OS-II
*                                        The Real-Time Kernel
*
*                    Copyright 1992-2021 Silicon Laboratories Inc. www.silabs.com
*
*                                 SPDX-License-Identifier: APACHE-2.0
*
*               This software is subject to an open source license and is distributed by
*                Silicon Laboratories Inc. pursuant to the terms of the Apache License,
*                    Version 2.0 available at www.apache.org/licenses/LICENSE-2.0.
*
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*
*                                    Microsoft Win32 Specific code
*
* Filename : cpu.h
* Version  : V2.93.01
*********************************************************************************************************
*/

#ifndef  CPU_H
#define  CPU_H

typedef void CPU_VOID;
typedef unsigned char CPU_CHAR;
typedef unsigned char CPU_BOOLEAN;
typedef unsigned char CPU_INTO8U;
typedef signed char CPU_INT08S;
typedef unsigned char CPU_INT08U;
typedef unsigned short CPU_INT16U;
typedef signed short CPU_INT16S;
typedef unsigned int CPU_INT32U;
typedef signed int CPU_INT32S;
typedef unsigned long long CPU_INT64U;
typedef signed long long CPU_INT64S;
typedef float CPU_FP32;
typedef double CPU_FP64;
typedef volatile CPU_INT08U CPU_REG08;
//typedef volatile CPU_INT16U CPU_REG16;
//typedef volatile CPU_INT320 CPU_REG32;
//typedef volatile CPU_INT640 CPU_REG64;
typedef void (*CPU_FNCT_VOID) (void);
typedef void (*CPU_FNCT_PTR) (void *);
typedef CPU_INT32U CPU_ADDR;
typedef CPU_INT32U CPU_DATA;
typedef CPU_DATA CPU_ALIGN;
typedef CPU_ADDR CPU_SIZE_T;
typedef CPU_INT32U CPU_STK;
typedef CPU_ADDR CPU_STK_SIZE;
typedef CPU_INT16U CPU_ERR;
typedef CPU_INT32U CPU_SR;
typedef CPU_INT32U CPU_TS;


#endif                                                          /* End of cpu module include.                        */
