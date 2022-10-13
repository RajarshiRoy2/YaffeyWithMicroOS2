#ifndef YAFFS_H
#define YAFFS_H

extern "C" {
    #include "yaffs2/yaffs_packedtags2.h"
}

#define CHUNK_SIZE  2048
#define SPARE_SIZE  64
#define PAGE_SIZE   (CHUNK_SIZE + SPARE_SIZE)

#endif  //YAFFS_H
