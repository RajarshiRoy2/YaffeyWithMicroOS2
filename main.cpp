/*
 * yaffey: Utility for reading, editing and writing YAFFS2 images
 * Copyright (C) 2012 David Place <david.t.place@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 */

#include <QApplication>

#include "MainWindow.h"
#include "ucos_ii.h"

#define  TASK0_STK_SIZE                         250u
static  CPU_STK  Task0Stk[TASK0_STK_SIZE];
static  CPU_STK  Task1Stk[TASK0_STK_SIZE];


void Task0 (void *p_arg)
{
    qDebug()<<"Started MicrOS system with timer...";
    while(1)
    {
        int milli_seconds = 1000;

        // Storing start time
        clock_t start_time = clock();

        // looping till required time is not achieved
        while (clock() < start_time + milli_seconds);
        qDebug()<< OSTimeGet();
        //TimeOS = OSTimeGet();

    }


}

void Task1 (void *p_arg)
{
    qDebug()<<"Task 1...";
}

int main(int argc, char* argv[]) {
    QString arg;
    if (argc > 0) {
        arg = argv[1];
    }




    OSInit();
    OS_STK stack = OSTaskCreate(Task0, (void *)0, &Task1Stk[TASK0_STK_SIZE - 1], 0);

    //OSTaskCreate(Task0, (void *)0, &Task0Stk[TASK0_STK_SIZE - 1], 0);


    OSStart();

    QApplication a(argc, argv);
    MainWindow w(NULL, arg);
    w.show();
    return a.exec();
}

