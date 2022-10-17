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
#include "shared_memory.h"
#include "MainWindow.h"
#include "ucos_ii.h"
//#define  TASK0_STK_SIZE                         250
//CPU_STK  Task0Stk[TASK0_STK_SIZE];

void Task0 (void *p_arg)
{
    for(;;)
    {
        if(OSRunning)
        {
            qDebug()<<"Yaffs started so pausing idle task..."<<OSTimeGet()<<Qt::endl;
            qDebug()<<"Idle counter..."<<OSIdleCtr<<Qt::endl;
            Sleep(100);
        }
        else
        {
            qDebug()<<"Yaffs didnt start so not pausing idle task..."<<OSTimeGet()<<Qt::endl;
            qDebug()<<"Idle counter..."<<OSIdleCtr<<Qt::endl;
            Sleep(100);
        }
        //signal(SIGINT,&sig_handler2);
    }
}

void Task1 (void *p_arg)
{
    for(;;)
    {
        if(OSRunning)
        {
            qDebug()<<"NewFile true task 1..."<<OSTimeGet();
            Sleep(1000);
        }
        else
        {
            qDebug()<<"NewFile false task 1..."<<OSTimeGet();
            Sleep(1000);
        }
    }
}
//void Task1 (void *p_arg)
//{

//    while(1)
//    {
//        qDebug()<<"Starting Yaffey Gui...";
//    }
//    //char* argv= " ";
//    //int argc = 0;
//    //QApplication *app = new QApplication(argc,&argv);

//    //QString arg;
//    //if (argc > 0) {
//    //    arg = argv[1];
//    //}


//   // MainWindow w(NULL, arg);
//   // w.show();
//    //app->exec();
//}

int main(int argc, char* argv[]) {
    QString arg;
    if (argc > 0) {
        arg = argv[1];
    }

    ///OSInit();
    //OSTaskCreate(Task0, (void *)0, &Task0Stk[TASK0_STK_SIZE - 1], 0);
    //OSStart();

    QApplication a(argc, argv);
    MainWindow w(NULL, arg);
    w.show();


    return a.exec();
}

