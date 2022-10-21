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

#include <QDebug>

#include <QFile>
#include <QMessageBox>
#include <QFileDialog>
#include <QListView>

#include "MainWindow.h"
#include "qtimer.h"
#include "ucos_ii.h"
#include "ui_MainWindow.h"
#include "DialogEditProperties.h"
#include "DialogFastboot.h"
#include "DialogImport.h"
#include "YaffsManager.h"
#include "YaffsTreeView.h"
#include "shared_memory.h"
#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif
#include <iostream>
#include <cstdlib>
#include <QAbstractItemModel>

using namespace std;

struct Command{
    int CommandNumber;
    QString imageFilename;
};

static const QString APPNAME = "Yaffey";
static const QString VERSION = "0.2";

#define  TASK0_STK_SIZE                         250
CPU_STK  Task0Stk[TASK0_STK_SIZE];
//shared memory
std::vector<Command>YaffsCommandsMicroOS2;
YaffsModel* CYaffsModel;            //not owned
YaffsManager* CYaffsManager;        //not owned - singleton
YaffsReadInfo readInfo;
YaffsSaveInfo saveInfo;
YaffsItem* parentItem;
QModelIndexList selectedRowsDelete;
QModelIndexList selectedRowsExport;
int itemsDeleted;
YaffsExportInfo* exportInfo;

void YaffeyCommandMicroOS2(void *p_arg)
{
    for(;;)
    {
        if(YaffsCommandsMicroOS2.size()>0)
            {

               switch(YaffsCommandsMicroOS2.at(0).CommandNumber){
                   case 0:
                       CYaffsManager = YaffsManager::getInstance();
                       YaffsCommandsMicroOS2.pop_back();
                   break;
                   case 1:
                       CYaffsModel = CYaffsManager->newModel();
                       YaffsCommandsMicroOS2.pop_back();
                   break;
                   case 2:
                       CYaffsModel->mYaffsRoot = YaffsItem::createRoot();
                       CYaffsModel->mItemsNew++;
                       CYaffsModel->mImageFilename = "new-yaffs2.img";
                       emit CYaffsModel->layoutChanged();

                       YaffsCommandsMicroOS2.pop_back();
                   break;
                   case 3:
                       CYaffsModel->mImageFilename = YaffsCommandsMicroOS2.at(0).imageFilename;

                       YaffsReadInfo readinfo;
                       memset(&readinfo, 0, sizeof(YaffsReadInfo));

                       if (CYaffsModel->mYaffsRoot == NULL)
                       {
                           YaffsControl yaffsControl(CYaffsModel->mImageFilename.toStdString().c_str(), CYaffsModel);
                           if (yaffsControl.open(YaffsControl::OPEN_READ)) {
                               if (yaffsControl.readImage()) {
                                   readinfo = yaffsControl.getReadInfo();

                                   CYaffsModel->mItemsNew = 0;
                                   CYaffsModel->mItemsDirty = 0;
                                   CYaffsModel->mItemsDeleted = 0;
                               }
                           }
                       }
                       readInfo = readinfo;
                       YaffsCommandsMicroOS2.pop_back();
                        break;
                   case 4:
                        saveInfo = CYaffsModel->saveAs(YaffsCommandsMicroOS2.at(0).imageFilename);
                        YaffsCommandsMicroOS2.pop_back();
                        break;
                   case 5:
                        CYaffsModel->importFile(parentItem, YaffsCommandsMicroOS2.at(0).imageFilename);
                        YaffsCommandsMicroOS2.pop_back();
                        break;
                   case 6:
                        CYaffsModel->importDirectory(parentItem, YaffsCommandsMicroOS2.at(0).imageFilename);
                        YaffsCommandsMicroOS2.pop_back();
                        break;
                  case 7:
                       foreach (QModelIndex index, selectedRowsDelete) {
                           YaffsItem* item = static_cast<YaffsItem*>(index.internalPointer());
                           if (item) {
                               item->markForDelete();
                           }
                       }

                       itemsDeleted = 0;
                       if (CYaffsModel->mYaffsRoot->hasChildMarkedForDelete()) {
                           QList<int> rowsToDelete;

                           //iterate through child items to build up list of items to delete
                           for (int i = 0; i < CYaffsModel->mYaffsRoot->childCount(); ++i) {
                               YaffsItem* childItem = CYaffsModel->mYaffsRoot->child(i);
                               if (childItem->isMarkedForDelete()) {
                                   rowsToDelete.append(childItem->row());
                               } else if (childItem->hasChildMarkedForDelete()) {
                                   itemsDeleted += CYaffsModel->processChildItemsForDelete(childItem);
                                   CYaffsModel->mYaffsRoot->setHasChildMarkedForDelete(false);
                               }
                           }

                           itemsDeleted += CYaffsModel->calculateAndDeleteContiguousRows(rowsToDelete, CYaffsModel->mYaffsRoot);
                       }
                        YaffsCommandsMicroOS2.pop_back();
                        break;
                   case 8:
                       CYaffsManager->mYaffsExportInfo = new YaffsExportInfo();
                       CYaffsManager->mYaffsExportInfo->numDirsExported = 0;
                       CYaffsManager->mYaffsExportInfo->numFilesExported = 0;

                       foreach (QModelIndex index, selectedRowsExport) {
                           YaffsItem* item = static_cast<YaffsItem*>(index.internalPointer());
                           CYaffsManager->exportItem(item, YaffsCommandsMicroOS2.at(0).imageFilename);
                       }
                        exportInfo = CYaffsManager->mYaffsExportInfo;
                        YaffsCommandsMicroOS2.pop_back();
                        break;
                   default:
                       break;
               }
            }
    }

}

void PushCommandOntoCommandVector(int Command, const QString& imageFilename)
{
    struct Command NewModelCommand;
    NewModelCommand.CommandNumber = Command;
    NewModelCommand.imageFilename = imageFilename;
    YaffsCommandsMicroOS2.push_back(NewModelCommand);
    while(YaffsCommandsMicroOS2.size()>0)
    {
        qDebug()<<"Pausing Main thread... Time:"<<OSTimeGet();
        Sleep(1);// just to slow down the print statements
    }
}

MainWindow::MainWindow(QWidget* parent, QString imageFilename) : QMainWindow(parent),
                                                                 mUi(new Ui::MainWindow),
                                                                 mContextMenu(this) {

    OSInit();
    OSTaskCreate(YaffeyCommandMicroOS2, (void *)0, &Task0Stk[TASK0_STK_SIZE - 1], 0);
    OSStart();

    mUi->setupUi(this);

    //external timer
    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this,SLOT(TimeUpdate()));
    timer->start(100);//100Hz timer

    //MainThreadtimer = new QTimer(this);
    //connect(MainThreadtimer, SIGNAL(timeout()), this,SLOT(TimerMainThead()));
    //MainThreadtimer->start();//100Hz timer

    //setup context menu for the treeview
    mContextMenu.addAction(mUi->actionImport);
    mContextMenu.addAction(mUi->actionExport);
    mContextMenu.addSeparator();
    mContextMenu.addAction(mUi->actionRename);
    mContextMenu.addAction(mUi->actionDelete);
    mContextMenu.addSeparator();
    mContextMenu.addAction(mUi->actionEditProperties);

    //setup context menu for the header
    mHeaderContextMenu.addAction(mUi->actionColumnName);
    mHeaderContextMenu.addAction(mUi->actionColumnSize);
    mHeaderContextMenu.addAction(mUi->actionColumnPermissions);
    mHeaderContextMenu.addAction(mUi->actionColumnAlias);
    mHeaderContextMenu.addAction(mUi->actionColumnDateAccessed);
    mHeaderContextMenu.addAction(mUi->actionColumnDateCreated);
    mHeaderContextMenu.addAction(mUi->actionColumnDateModified);
    mHeaderContextMenu.addAction(mUi->actionColumnUser);
    mHeaderContextMenu.addAction(mUi->actionColumnGroup);

    //get YaffsManager instance and create model
    QString PlaceHolder;
    PushCommandOntoCommandVector(0,PlaceHolder);

    //CYaffsManager = YaffsManager::getInstance();
    newModel();
    updateWindowTitle();

    QHeaderView* headerView = mUi->treeView->header();
    headerView->setContextMenuPolicy(Qt::CustomContextMenu);
    headerView->setSectionResizeMode(YaffsItem::NAME, QHeaderView::Stretch);
    headerView->setSectionResizeMode(YaffsItem::SIZE, QHeaderView::ResizeToContents);
    headerView->setSectionResizeMode(YaffsItem::PERMISSIONS, QHeaderView::ResizeToContents);
    headerView->setSectionResizeMode(YaffsItem::ALIAS, QHeaderView::ResizeToContents);
    headerView->setSectionResizeMode(YaffsItem::DATE_ACCESSED, QHeaderView::ResizeToContents);
    headerView->setSectionResizeMode(YaffsItem::DATE_CREATED, QHeaderView::ResizeToContents);
    headerView->setSectionResizeMode(YaffsItem::DATE_MODIFIED, QHeaderView::ResizeToContents);
    headerView->setSectionResizeMode(YaffsItem::USER, QHeaderView::ResizeToContents);
    headerView->setSectionResizeMode(YaffsItem::GROUP, QHeaderView::ResizeToContents);
#ifdef QT_DEBUG
    headerView->setSectionResizeMode(YaffsItem::OBJECTID, QHeaderView::ResizeToContents);
    headerView->setSectionResizeMode(YaffsItem::PARENTID, QHeaderView::ResizeToContents);
    headerView->setSectionResizeMode(YaffsItem::HEADERPOS, QHeaderView::ResizeToContents);
#endif  //QT_DEBUG

    mUi->treeView->hideColumn(YaffsItem::DATE_CREATED);
    mUi->treeView->hideColumn(YaffsItem::DATE_ACCESSED);

    mUi->actionColumnName->setEnabled(false);
    mUi->actionColumnName->setChecked(!mUi->treeView->isColumnHidden(YaffsItem::NAME));
    mUi->actionColumnSize->setChecked(!mUi->treeView->isColumnHidden(YaffsItem::SIZE));
    mUi->actionColumnPermissions->setChecked(!mUi->treeView->isColumnHidden(YaffsItem::PERMISSIONS));
    mUi->actionColumnAlias->setChecked(!mUi->treeView->isColumnHidden(YaffsItem::ALIAS));
    mUi->actionColumnDateAccessed->setChecked(!mUi->treeView->isColumnHidden(YaffsItem::DATE_ACCESSED));
    mUi->actionColumnDateCreated->setChecked(!mUi->treeView->isColumnHidden(YaffsItem::DATE_CREATED));
    mUi->actionColumnDateModified->setChecked(!mUi->treeView->isColumnHidden(YaffsItem::DATE_MODIFIED));
    mUi->actionColumnUser->setChecked(!mUi->treeView->isColumnHidden(YaffsItem::USER));
    mUi->actionColumnGroup->setChecked(!mUi->treeView->isColumnHidden(YaffsItem::GROUP));

    connect(headerView, SIGNAL(customContextMenuRequested(QPoint)), SLOT(on_treeViewHeader_customContextMenuRequested(QPoint)));
    connect(mUi->actionExpandAll, SIGNAL(triggered()), mUi->treeView, SLOT(expandAll()));
    connect(mUi->actionCollapseAll, SIGNAL(triggered()), mUi->treeView, SLOT(collapseAll()));
    connect(mUi->treeView, SIGNAL(selectionChanged()), SLOT(on_treeView_selectionChanged()));

    if (imageFilename.length() > 0) {
        show();
        openImage(imageFilename);
    } else {
        mUi->statusBar->showMessage(windowTitle() + " v" + VERSION);
    }

    mFastbootDialog = NULL;

    setupActions();


}

void MainWindow::TimerMainThead()//check the vector if any yaffs function is left to do issue with saving the file onto desktop
{
    qDebug()<<"Time:"<<OSTimeGet();
    if(YaffsCommandsMicroOS2.size()>0)
    {
        qDebug()<<YaffsCommandsMicroOS2.at(0).CommandNumber;
        qDebug()<<"Pausing Main thread... Time:"<<OSTimeGet();
    }
    else
        return;

}

void MainWindow::TimeUpdate()//OSTickISR equivalent
{
    OSIntEnter();
    if (OSIntNesting == 1) {
         //OSTCBCur->OSTCBStkPtr = Stack Pointer;
      }
    OSTimeTick();
    OSIntExit();
}

MainWindow::~MainWindow() {
    delete mUi;
    delete mFastbootDialog;
}



void MainWindow::newModel() {//cannot put this function onto MicroOS due to Qt connection to shared memory

    //PushCommandOntoCommandVector(1);
    CYaffsModel = CYaffsManager->newModel();
    mUi->treeView->setModel(CYaffsModel);
    connect(CYaffsManager, SIGNAL(modelChanged()), SLOT(on_modelChanged()));
}

void MainWindow::on_treeView_doubleClicked(const QModelIndex& itemIndex) {
    YaffsItem* item = static_cast<YaffsItem*>(itemIndex.internalPointer());
    if (item) {
        if (item->isFile() || item->isSymLink()) {
            mUi->actionEditProperties->trigger();
        }
    }
}

void MainWindow::on_actionNew_triggered() {
    newModel();
    QString PlaceHolder;
    PushCommandOntoCommandVector(2,PlaceHolder);

    //CYaffsModel->newImage("new-yaffs2.img");
    mUi->statusBar->showMessage("Created new YAFFS2 image");
    OSRunning = true;
}

void MainWindow::on_actionOpen_triggered() {
    QString imageFilename = QFileDialog::getOpenFileName(this, "Open File", ".");

    if (imageFilename.length() > 0) {
        newModel();
        openImage(imageFilename);
        OSRunning = true;
    }
}

void MainWindow::updateWindowTitle() {//cannot add to Gui since its Qt realted
    QString modelFilename = CYaffsModel->getImageFilename();
    if (modelFilename.length() > 0) {
        if (CYaffsModel->isDirty()) {
            setWindowTitle(APPNAME + " - " + modelFilename + "*");
        } else {
            setWindowTitle(APPNAME + " - " + modelFilename);
        }
    } else {
        setWindowTitle(APPNAME);
    }
}

void MainWindow::openImage(const QString& imageFilename) {//done
    if (imageFilename.length() > 0) {
        PushCommandOntoCommandVector(3,imageFilename);
        //readInfo = CYaffsModel->openImage(imageFilename);
        CYaffsModel->openImage(imageFilename);//only does emite layout which is Qt realted
        if (readInfo.result) {
            QModelIndex rootIndex = CYaffsModel->index(0, 0);
            mUi->treeView->expand(rootIndex);
            mUi->statusBar->showMessage("Opened image: " + imageFilename);

            updateWindowTitle();
            QString summary("<table>" \
                            "<tr><td width=120>Files:</td><td>" + QString::number(readInfo.numFiles) + "</td></tr>" +
                            "<tr><td width=120>Directories:</td><td>" + QString::number(readInfo.numDirs) + "</td></tr>" +
                            "<tr><td width=120>SymLinks:</td><td>" + QString::number(readInfo.numSymLinks) + "</td></tr>" +
                            "<tr><td colspan=2><hr/></td></tr>" +
                            "<tr><td width=120>HardLinks:</td><td>" + QString::number(readInfo.numHardLinks) + "</td></tr>" +
                            "<tr><td width=120>Specials:</td><td>" + QString::number(readInfo.numSpecials) + "</td></tr>" +
                            "<tr><td width=120>Unknowns:</td><td>" + QString::number(readInfo.numUnknowns) + "</td></tr>" +
                            "<tr><td colspan=2><hr/></td></tr>" +
                            "<tr><td width=120>Errors:</td><td>" + QString::number(readInfo.numErrorousObjects) + "</td></tr></table>");

            if (readInfo.eofHasIncompletePage) {
                summary += "<br/><br/>Warning:<br/>Incomplete page found at end of file";
            }
            QMessageBox::information(this, "Summary", summary);
        } else {
            QString msg = "Error opening image: " + imageFilename;
            mUi->statusBar->showMessage(msg);
            QMessageBox::critical(this, "Error", msg);
        }
        setupActions();//qt realted Gui changed from shared memory
        OSRunning = true;
    }
}

void MainWindow::on_actionClose_triggered() {//Qt related mostly but other functons done in yaffs command center
    QString imageFile = CYaffsModel->getImageFilename();
    if (imageFile.length() > 0) {
        newModel();
        mUi->statusBar->showMessage("Closed image file: " + imageFile);
    }
    mUi->linePath->clear();
    updateWindowTitle();
    setupActions();
    OSRunning = false;
}

void MainWindow::on_actionSaveAs_triggered() {//done and the rest are Qt Gui related
    if (CYaffsModel->isImageOpen()) {
        QString imgName = CYaffsModel->getImageFilename();
        QString saveAsFilename = QFileDialog::getSaveFileName(this, "Save Image As", "./" + imgName);
        if (saveAsFilename.length() > 0) {
            PushCommandOntoCommandVector(4,saveAsFilename);

            //saveInfo = CYaffsModel->saveAs(saveAsFilename);
            updateWindowTitle();
            if (saveInfo.result) {
                mUi->statusBar->showMessage("Image saved: " + saveAsFilename);
                QString summary("<table>" \
                                "<tr><td width=120>Files:</td><td>" + QString::number(saveInfo.numFilesSaved) + "</td></tr>" +
                                "<tr><td width=120>Directories:</td><td>" + QString::number(saveInfo.numDirsSaved) + "</td></tr>" +
                                "<tr><td width=120>SymLinks:</td><td>" + QString::number(saveInfo.numSymLinksSaved) + "</td></tr>" +
                                "<tr><td colspan=2><hr/></td></tr>" +
                                "<tr><td width=120>Files Failed:</td><td>" + QString::number(saveInfo.numFilesFailed) + "</td></tr>" +
                                "<tr><td width=120>Directories Failed:</td><td>" + QString::number(saveInfo.numDirsFailed) + "</td></tr>" +
                                "<tr><td width=120>SymLinks Failed:</td><td>" + QString::number(saveInfo.numSymLinksFailed) + "</td></tr></td></tr></table>");
                QMessageBox::information(this, "Save summary", summary);
            } else {
                QString msg = "Error saving image: " + saveAsFilename;
                QMessageBox::critical(this, "Error", msg);
                mUi->statusBar->showMessage(msg);
            }
        }
    }
}
//importing file or folder to yaffs
void MainWindow::on_actionImport_triggered() {//done
    DialogImport import(this);
    int result = import.exec();

    if (result == DialogImport::RESULT_FILE) {
        QModelIndex parentIndex = mUi->treeView->selectionModel()->currentIndex();
        parentItem = static_cast<YaffsItem*>(parentIndex.internalPointer());
        if (parentItem && parentItem->isDir()) {
            QStringList fileNames = QFileDialog::getOpenFileNames(this, "Select file(s) to import...");
            foreach (QString importFilename, fileNames) {
                importFilename.replace('\\', '/');
                PushCommandOntoCommandVector(5,importFilename);
                //CYaffsModel->importFile(parentItem, importFilename);
            }
        }
    } else if (result == DialogImport::RESULT_DIRECTORY) {
        QModelIndex parentIndex = mUi->treeView->selectionModel()->currentIndex();
        parentItem = static_cast<YaffsItem*>(parentIndex.internalPointer());
        if (parentItem && parentItem->isDir()) {
            QString directoryName = QFileDialog::getExistingDirectory(this, "Select directory to import...");
            if (directoryName.length() > 0) {
                directoryName.replace('\\', '/');
                PushCommandOntoCommandVector(6,directoryName);
                //CYaffsModel->importDirectory(parentItem, directoryName);
            }
        }
    }
}
//button to save to desktop
void MainWindow::on_actionExport_triggered() {//not used by yaffs at all
    QModelIndexList selectedRows = mUi->treeView->selectionModel()->selectedRows();
    if (selectedRows.size() > 0) {
        QString path = QFileDialog::getExistingDirectory(this);
        if (path.length() > 0) {
            exportSelectedItems(path);
        } else {
            mUi->statusBar->showMessage("Export cancelled");
        }
    } else {
        mUi->statusBar->showMessage("Nothing selected to export");
    }
}
//closing qt gui
void MainWindow::on_actionExit_triggered() {
    close();
}
//reanming using Qt libraries and not yaffs
void MainWindow::on_actionRename_triggered() {
    QModelIndex index = mUi->treeView->selectionModel()->currentIndex();
    YaffsItem* item = static_cast<YaffsItem*>(index.internalPointer());
    if (item && !item->isRoot()) {
        if (index.column() == YaffsItem::NAME) {
            mUi->treeView->edit(index);
        }
    }
}

void MainWindow::on_actionDelete_triggered() {// ldelete on yaffs loop and removed from qt
    selectedRowsDelete = mUi->treeView->selectionModel()->selectedRows();
    QString PlaceHolder;
    PushCommandOntoCommandVector(7,PlaceHolder);

    //int numRowsDeleted = CYaffsModel->removeRows(selectedRowsDelete);
    mUi->statusBar->showMessage("Deleted " + QString::number(itemsDeleted) + " items");
}

void MainWindow::on_actionEditProperties_triggered() {//Gui realted
    QModelIndexList selectedRows = mUi->treeView->selectionModel()->selectedRows();
    if (selectedRows.size() > 0) {
        QDialog* dialog = new DialogEditProperties(*CYaffsModel, selectedRows, this);
        dialog->exec();
    }
    setupActions();
}

void MainWindow::on_actionAndroidFastboot_triggered() {//Gui realted
    if (mFastbootDialog) {
        mFastbootDialog->show();
    } else {
        mFastbootDialog = new DialogFastboot(this);
        mFastbootDialog->exec();
    }
}

void MainWindow::on_actionAbout_triggered() {//Gui realted
    static const QString about("<b>" + APPNAME + " v" + VERSION + "</b><br/>" \
                               "Yet Another Flash File (System) Editor YEAH!<br/><br/>" \
                               "Built on " + QString(__DATE__) + " at " + QString(__TIME__) + "<br/><br/>" );
    QMessageBox::information(this, "About " + APPNAME, about);
}

void MainWindow::on_actionTime_triggered()//timer and must be external to Microos 2
{
    QString Time = QString::number(OSTimeGet());
    QString about("<b>Time: " + Time + "</b><br/>");
    QMessageBox::information(this, "Time ", about);
    mUi->statusBar->showMessage("Time:", OSTimeGet());

}

void MainWindow::on_actionColumnName_triggered() {//Gui realted
    if (mUi->actionColumnName->isChecked()) {
        mUi->treeView->showColumn(YaffsItem::NAME);
    } else {
        mUi->treeView->hideColumn(YaffsItem::NAME);
    }
}

void MainWindow::on_actionColumnSize_triggered() {//Gui realted
    if (mUi->actionColumnSize->isChecked()) {
        mUi->treeView->showColumn(YaffsItem::SIZE);
    } else {
        mUi->treeView->hideColumn(YaffsItem::SIZE);
    }
}

void MainWindow::on_actionColumnPermissions_triggered() {//Gui realted
    if (mUi->actionColumnPermissions->isChecked()) {
        mUi->treeView->showColumn(YaffsItem::PERMISSIONS);
    } else {
        mUi->treeView->hideColumn(YaffsItem::PERMISSIONS);
    }
}

void MainWindow::on_actionColumnAlias_triggered() {//Gui realted
    if (mUi->actionColumnAlias->isChecked()) {
        mUi->treeView->showColumn(YaffsItem::ALIAS);
    } else {
        mUi->treeView->hideColumn(YaffsItem::ALIAS);
    }
}

void MainWindow::on_actionColumnDateAccessed_triggered() {//Gui realted
    if (mUi->actionColumnDateAccessed->isChecked()) {
        mUi->treeView->showColumn(YaffsItem::DATE_ACCESSED);
    } else {
        mUi->treeView->hideColumn(YaffsItem::DATE_ACCESSED);
    }
}

void MainWindow::on_actionColumnDateCreated_triggered() {//Gui realted
    if (mUi->actionColumnDateCreated->isChecked()) {
        mUi->treeView->showColumn(YaffsItem::DATE_CREATED);
    } else {
        mUi->treeView->hideColumn(YaffsItem::DATE_CREATED);
    }
}

void MainWindow::on_actionColumnDateModified_triggered() {//Gui realted
    if (mUi->actionColumnDateModified->isChecked()) {
        mUi->treeView->showColumn(YaffsItem::DATE_MODIFIED);
    } else {
        mUi->treeView->hideColumn(YaffsItem::DATE_MODIFIED);
    }
}

void MainWindow::on_actionColumnUser_triggered() {//Gui realted
    if (mUi->actionColumnUser->isChecked()) {
        mUi->treeView->showColumn(YaffsItem::USER);
    } else {
        mUi->treeView->hideColumn(YaffsItem::USER);
    }
}

void MainWindow::on_actionColumnGroup_triggered() {//Gui realted
    if (mUi->actionColumnGroup->isChecked()) {
        mUi->treeView->showColumn(YaffsItem::GROUP);
    } else {
        mUi->treeView->hideColumn(YaffsItem::GROUP);
    }
}

void MainWindow::on_treeViewHeader_customContextMenuRequested(const QPoint& pos) {//Gui realted
    QPoint p(mUi->treeView->mapToGlobal(pos));
    mHeaderContextMenu.exec(p);
}

void MainWindow::on_treeView_customContextMenuRequested(const QPoint& pos) {//Gui realted
    setupActions();

    QPoint p(mUi->treeView->mapToGlobal(pos));
    p.setY(p.y() + mUi->treeView->header()->height());
    mContextMenu.exec(p);
}

void MainWindow::on_modelChanged() {//Gui realted
    setupActions();
}

void MainWindow::on_treeView_selectionChanged() {//Gui realted
    setupActions();
}

//goes to yaffsmanafer to write to iso file on dekstop
void MainWindow::exportSelectedItems(const QString& path) { //last one
    selectedRowsExport = mUi->treeView->selectionModel()->selectedRows();
    if (selectedRowsExport.size() > 0) {
        PushCommandOntoCommandVector(8,path);
        //exportInfo = CYaffsManager->exportItems(selectedRowsExport, path);

        QString status = "Exported " + QString::number(exportInfo->numDirsExported) + " dir(s) and " +
                                       QString::number(exportInfo->numFilesExported) + " file(s).";
        mUi->statusBar->showMessage(status);
        //are messages to display in case file and directory fails to save onto desktop
        int dirFails = exportInfo->listDirExportFailures.size();
        int fileFails = exportInfo->listFileExportFailures.size();
        if (dirFails + fileFails > 0) {
            QString msg;

            if (dirFails > 0) {
                static const int MAXDIRS = 10;
                QString items;
                int max = (dirFails > MAXDIRS ? MAXDIRS : dirFails);
                for (int i = 0; i < max; ++i) {
                    const YaffsItem* item = exportInfo->listDirExportFailures.at(i);
                    items += item->getFullPath() + "\n";
                }
                msg += "Failed to export directories:\n" + items;

                if (dirFails > MAXDIRS) {
                    msg += "... plus " + QString::number(dirFails - MAXDIRS) + " more";
                }
            }

            if (fileFails > 0) {
                if (dirFails > 0) {
                    msg += "\n";
                }

                static const int MAXFILES = 10;
                QString items;
                int max = (fileFails > MAXFILES ? MAXFILES : fileFails);
                for (int i = 0; i < max; ++i) {
                    const YaffsItem* item = exportInfo->listFileExportFailures.at(i);
                    items += item->getFullPath() + "\n";
                }
                msg += "Failed to export files:\n" + items;

                if (fileFails > MAXFILES) {
                    msg += "... plus " + QString::number(fileFails - MAXFILES) + " more";
                }
            }

            QMessageBox::critical(this, "Export", msg);
        }

        delete exportInfo;
    }
}

int MainWindow::identifySelection(const QModelIndexList& selectedRows) {
    int selectionFlags = (selectedRows.size() == 1 ? SELECTED_SINGLE : 0);

    //iterate through the list of items to see what we have selected
    YaffsItem* item = NULL;
    foreach (QModelIndex index, selectedRows) {
        item = static_cast<YaffsItem*>(index.internalPointer());
        if (item) {
            selectionFlags |= (item->isRoot() ? SELECTED_ROOT : 0);
            selectionFlags |= (item->isDir() ? SELECTED_DIR : 0);
            selectionFlags |= (item->isFile() ? SELECTED_FILE : 0);
            selectionFlags |= (item->isSymLink() ? SELECTED_SYMLINK : 0);
        }
    }

    return selectionFlags;
}
//setting up Gui
void MainWindow::setupActions() {//Gui related to update it and cannot be put into MicroOS2
    updateWindowTitle();

    QModelIndexList selectedRows = mUi->treeView->selectionModel()->selectedRows();
    int selectionFlags = identifySelection(selectedRows);
    int selectionSize = selectedRows.size();

    if (CYaffsModel->index(0, 0).isValid()) {
        mUi->actionExpandAll->setEnabled(true);
        mUi->actionCollapseAll->setEnabled(true);
        mUi->actionSaveAs->setEnabled(true);
    } else {
        mUi->actionExpandAll->setEnabled(false);
        mUi->actionCollapseAll->setEnabled(false);
        mUi->actionSaveAs->setEnabled(false);
    }

    mUi->actionEditProperties->setEnabled(false);
    mUi->actionImport->setEnabled(false);
    mUi->actionExport->setEnabled(false);
    mUi->actionRename->setEnabled(false);
    mUi->actionDelete->setEnabled(false);

    //if only a single item is selected
    if (selectionSize == 1) {
        mUi->actionRename->setEnabled(!(selectionFlags & SELECTED_ROOT));
        mUi->actionImport->setEnabled(  selectionFlags & SELECTED_DIR);

        QModelIndex itemIndex = selectedRows.at(0);
        YaffsItem* item = static_cast<YaffsItem*>(itemIndex.internalPointer());
        if (item) {
            mUi->linePath->setText(item->getFullPath());
        }
    }

    if (selectionSize >= 1) {
        mUi->actionDelete->setEnabled(!(selectionFlags & SELECTED_ROOT));
        mUi->actionEditProperties->setEnabled(!(selectionFlags & SELECTED_ROOT));
        mUi->actionExport->setEnabled((selectionFlags & (SELECTED_DIR | SELECTED_FILE) && !(selectionFlags & SELECTED_SYMLINK)));

        mUi->statusBar->showMessage("Selected " + QString::number(selectedRows.size()) + " items");
    } else if (selectionSize == 0) {
        mUi->statusBar->showMessage("");
    }
}
