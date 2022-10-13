#-------------------------------------------------
#
# Project created by QtCreator 2012-04-27T16:17:40
#
#-------------------------------------------------

QT        += core gui
QT += widgets
TARGET     = yaffey
TEMPLATE   = app
RC_FILE    = yaffey.rc

SOURCES   += main.cpp\
    MainWindow.cpp \
    YaffeyCommand.c \
    YaffsModel.cpp \
    YaffsItem.cpp \
    YaffsTreeView.cpp \
    DialogEditProperties.cpp \
    YaffsControl.cpp \
    os_core.c \
    os_dbg_r.c \
    os_flag.c \
    os_mbox.c \
    os_mem.c \
    os_mutex.c \
    os_q.c \
    os_sem.c \
    os_task.c \
    os_time.c \
    os_tmr.c \
    ucos_ii.c \
    ucos_stubs.c \
    yaffs2/yaffs_packedtags2.c \
    yaffs2/yaffs_hweight.c \
    yaffs2/yaffs_ecc.c \
    DialogFastboot.cpp \
    DialogImport.cpp \
    YaffsManager.cpp

HEADERS   += \
    MainWindow.h \
    YaffeyCommand.h \
    YaffsModel.h \
    YaffsItem.h \
    YaffsTreeView.h \
    DialogEditProperties.h \
    YaffsControl.h \
    app_cfg.h \
    cpu.h \
    os_cfg.h \
    os_cpu.h \
    shared_memory.h \
    ucos_ii.h \
    yaffs2/yaffs_trace.h \
    yaffs2/yaffs_packedtags2.h \
    yaffs2/yaffs_hweight.h \
    yaffs2/yaffs_guts.h \
    yaffs2/yaffs_ecc.h \
    AndroidIDs.h \
    Yaffs2.h \
    DialogFastboot.h \
    DialogImport.h \
    YaffsManager.h

FORMS     += \
    MainWindow.ui \
    DialogEditProperties.ui \
    DialogFastboot.ui \
    DialogImport.ui

RESOURCES += \
    icons.qrc \
    icons.qrc

DISTFILES += \
    .gitattributes \
    .hgignore \
    README.md \
    yaffey.pro.user \
    yaffey.rc \
    yaffeyNASA.cbp \
    yaffeyNASA.depend \
    yaffeyNASA.layout
