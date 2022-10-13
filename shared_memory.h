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
#include <stdio.h>
#include <stddef.h>
//#include "Yaffs2.h"

enum Condition {
    CLEAN,
    DIRTY,
    NEW,
    MOVED
};

struct YaffeyItem
{

    //YaffeyItem* mParentItem;
    int mHeaderPosition;
    int mYaffsObjectId;
    //YaffeyItem* mChildItems[255];
    struct yaffs_obj_hdr mYaffsObjectHeader;
    enum Condition mCondition;
    char* mExternalFilename;      //filename with path - only for new files
    bool mMarkedForDelete;
    bool mHasChildMarkedForDelete;

};
#pragma once
#ifndef SHARED_MEMORY_H
#define SHARED_MEMORY_H


//shared memory

extern struct YaffeyItem Root;
extern struct yaffs_obj_hdr mYaffsObjectHeader;
extern enum Condition mCondition;
#endif // SHARED_MEMORY_H
