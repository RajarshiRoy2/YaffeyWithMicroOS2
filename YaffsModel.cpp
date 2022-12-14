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

#include <QtGui>

#include "YaffsModel.h"
#include "os_cpu.h"
#include "shared_memory.h"

YaffsModel::YaffsModel(QObject* parent) : QAbstractItemModel(parent) {
    mYaffsRoot = NULL;
    mYaffsSaveControl = NULL;
    LogFile = NULL;

    mItemsNew = 0;
    mItemsDirty = 0;
    mItemsDeleted = 0;
    LogFileFound = false;
    LogFileReadOnce = false;
    Once=0;
    OnceNewLog = 0;
}

YaffsModel::~YaffsModel() {
    delete mYaffsRoot;
}

void YaffsModel::newImage(const QString& newImageName) {

    mYaffsRoot = YaffsItem::createRoot();
    mItemsNew++;
    mImageFilename = newImageName;
    LogFileFound = false;

    emit layoutChanged();
}
//re reading old yaffeys images files
YaffsReadInfo YaffsModel::openImage(const QString& imageFilename) {

    mImageFilename = imageFilename;

    YaffsReadInfo readInfo;
    memset(&readInfo, 0, sizeof(YaffsReadInfo));

    if (mYaffsRoot == NULL) {
        YaffsControl yaffsControl(mImageFilename.toStdString().c_str(), this);
        if (yaffsControl.open(YaffsControl::OPEN_READ)) {
            if (yaffsControl.readImage()) {
                readInfo = yaffsControl.getReadInfo();

                mItemsNew = 0;
                mItemsDirty = 0;
                mItemsDeleted = 0;

                emit layoutChanged();
            }
        }
    }
    return readInfo;
}
//saves to yaffs from deskptop
void YaffsModel::importFile(YaffsItem* parentItem, const QString& filenameWithPath) {
    if (parentItem && filenameWithPath.length() > 0) {
        QFileInfo fileInfo(filenameWithPath);
        int filesize = fileInfo.size();

        YaffsItem* importedFile = YaffsItem::createFile(parentItem, filenameWithPath, filesize);
        if(importedFile->getExternalFilename() == "Log.txt")
        {
            LogFile = importedFile;
            LogFileFound = true;
        }
        else if(importedFile->getExternalFilename() == "New.txt")
        {
            NewLogFile = importedFile;
            LogFileFound = true;
        }
        parentItem->appendChild(importedFile);

        //can add log fuile from using this code you added
        //std::string str = "log.txt";
        //YaffsItem* RajimportedFile = YaffsItem::createFile(parentItem, QString::fromStdString(str), 3);
        //parentItem->appendChild(RajimportedFile);

        //mItemsNew++;
        mItemsNew++;
        emit layoutChanged();
    }
}

void YaffsModel::importDirectory(YaffsItem* parentItem, const QString& directoryName) {
    if (parentItem && directoryName.length() > 0) {
        YaffsItem* newDir = YaffsItem::createDirectory(parentItem, directoryName);
        parentItem->appendChild(newDir);
        mItemsNew++;

        QDirIterator dirs(directoryName, QDirIterator::NoIteratorFlags);
        while (dirs.hasNext()) {
            QFileInfo fileInfo(dirs.next());
            QString fileName = fileInfo.fileName();
            QString fileNameWithPath = fileInfo.absoluteFilePath();

            if (fileInfo.isDir()) {
                if (fileName != "." && fileName != "..") {
                    importDirectory(newDir, fileNameWithPath);
                }
            } else if (fileInfo.isFile()) {
                importFile(newDir, fileNameWithPath);
            }
        }

        emit layoutChanged();
    }
}

//Add new folder
void YaffsModel::CreateNewDirectory() {
    YaffsItem* parentItem;
    const QString& directoryName = "Untitled";

    if (parentItem && directoryName.length() > 0) {
        YaffsItem* newDir = YaffsItem::createDirectory(parentItem, directoryName);
        parentItem->appendChild(newDir);
        mItemsNew++;

        emit layoutChanged();
    }
}
//clicking save as on the yaffs to save iso file onto desktop
YaffsSaveInfo YaffsModel::saveAs(const QString& filename) {
    YaffsSaveInfo saveInfo;
    memset(&saveInfo, 0, sizeof(YaffsSaveInfo));
    //qDebug()<<"saving"<<Qt::endl;
    //qDebug()<<filename<<Qt::endl;//name of the iso file "C:/Users/royra/OneDrive/Desktop/new-yaffs2.img"
    //qDebug()<<mImageFilename<<Qt::endl;// "new-yaffs2.img"

    if (filename != mImageFilename) {
        mYaffsSaveControl = new YaffsControl(filename.toStdString().c_str(), NULL);
        if (mYaffsSaveControl->open(YaffsControl::OPEN_NEW)) {//WRITE_LOG OPEN_NEW
            saveDirectory(mYaffsRoot);
            saveInfo = mYaffsSaveControl->getSaveInfo();
            saveInfo.result = (saveInfo.numDirsFailed + saveInfo.numFilesFailed + saveInfo.numSymLinksFailed == 0);
        }
        delete mYaffsSaveControl;
        mYaffsSaveControl = NULL;

        if (saveInfo.result) {
            mItemsNew = 0;
            mItemsDirty = 0;
            mItemsDeleted = 0;
            mImageFilename = filename;
        }
    }

    return saveInfo;
}

void YaffsModel::saveDirectory(YaffsItem* dirItem) {
    if (dirItem) {
        YaffsItem* parentItem = dirItem->parent();
        if (parentItem) {
            qDebug() << "d: " << dirItem->getFullPath() << ", Parent: " << parentItem->getFullPath();
        } else {
            qDebug() << "d: " << dirItem->getFullPath() << ", Parent: NULL";
        }

        int newObjectId = -1;
        int newHeaderPos = -1;

        if (parentItem) {
            newObjectId = mYaffsSaveControl->addDirectory(dirItem->getHeader(), newHeaderPos);
            qDebug()<<"Saving directory"<<newHeaderPos;
        } else {
            newObjectId = mYaffsSaveControl->addRoot(dirItem->getHeader(), newHeaderPos);
            qDebug()<<"Saving directory root"<<newHeaderPos;
        }
        dirItem->setHeaderPosition(newHeaderPos);
        dirItem->setObjectId(newObjectId);

        int childCount = dirItem->childCount();
        for (int i = 0; i < childCount; ++i) {
            YaffsItem* childItem = dirItem->child(i);
            childItem->setParentObjectId(newObjectId);

            if (childItem->isDir()) {
                saveDirectory(childItem);
            } else if (childItem->isFile()) {
                saveFile(childItem);
            } else if (childItem->isSymLink()) {
                saveSymLink(childItem);
            }
        }

        dirItem->setCondition(YaffsItem::CLEAN);
    }
}

void YaffsModel::CreateRootForLogs()
{
    mYaffsSaveControl = new YaffsControl(YaffsLogMemory.toStdString().c_str(), NULL);
    mYaffsSaveControl->open(YaffsControl::OPEN_NEW);
    YaffsItem* parentItem = mYaffsRoot->parent();


    if (parentItem) {
        LognewObjectId = mYaffsSaveControl->addDirectory(mYaffsRoot->getHeader(), LognewHeaderPos);
    } else {
        LognewObjectId = mYaffsSaveControl->addRoot(mYaffsRoot->getHeader(), LognewHeaderPos);
    }
    mYaffsRoot->setHeaderPosition(LognewHeaderPos);
    mYaffsRoot->setObjectId(LognewObjectId);
}

void YaffsModel::PrintContentofLogFile(YaffsItem *LogItem, QString Location )
{
    //Displaying all the contents in the log file earlier ones
    int headerPosition = LogItem->getHeaderPosition();
    qDebug()<<headerPosition;

    YaffsControl yaffsControl(Location.toStdString().c_str(), NULL);
    if (yaffsControl.open(YaffsControl::OPEN_READ)) {
        char* data = yaffsControl.extractFile(headerPosition);//reading old iso files
        std::string OldData = data;
        qDebug()<<QString::fromStdString(OldData);
    }
}

void YaffsModel::writeToFile(YaffsItem *parentItem, YaffsItem *LogItem, std::string Data)
{
    if(LogItem != NULL)
    {
        int sizeOfChar = Data.length();
        char* data = new char[sizeOfChar];//filesize
        strcpy(data, Data.c_str());

        int filesize = LogItem->getFileSize();
        filesize = filesize + sizeOfChar;
        LogItem->setFileSize(filesize);

        if(LogItem->getFullPath() == "/Log.txt")// when its new log file
        {
            if(Once==0)
            {
                LognewObjectId = mYaffsSaveControl->addFile(LogItem->getHeader(), LognewHeaderPos, data, sizeOfChar);
            }
            else
            {
                LognewObjectId = mYaffsSaveControl->addTextFile(LogItem->getHeader(), LognewHeaderPos, data, sizeOfChar);
                PrintContentofLogFile(LogItem, YaffsLogMemory.toStdString().c_str());
            }
            Once++;
        }
        else if(LogItem->getFullPath() == "/New.txt")// when its new log file
        {

            if(OnceNewLog==0)
            {
                LognewObjectId = mYaffsSaveControl->addFile(LogItem->getHeader(), LognewHeaderPos, data, sizeOfChar);
            }
            else
            {
                LognewObjectId = mYaffsSaveControl->addTextFile(LogItem->getHeader(), LognewHeaderPos, data, sizeOfChar);
                PrintContentofLogFile(LogItem, YaffsLogMemory.toStdString().c_str());
            }
            OnceNewLog++;

        }
        delete[] data;
        LogItem->setHeaderPosition(LognewHeaderPos);
        LogItem->setObjectId(LognewObjectId);
        LogItem->setCondition(YaffsItem::CLEAN);
    }
    else //when reading old log file
    {
        //qDebug()<<"Null";

    }
}

//saves files to desktop from save as to iso file
void YaffsModel::saveFile(YaffsItem* fileItem) {

    if (fileItem) {
        YaffsItem* parentItem = fileItem->parent();
        qDebug() << "f: " << fileItem->getFullPath() << ", Parent: " << parentItem->getFullPath();

       if (fileItem->isFile()) {
            YaffsItem::Condition condition = fileItem->getCondition();
            bool saved = false;
            int filesize = fileItem->getFileSize();
            int newObjectId = -1;
            int newHeaderPos = -1;

            if (condition == YaffsItem::NEW) {//reading the file and passing to break into chunks
                char* data = new char[filesize];
                QString filename = fileItem->getExternalFilename();
                FILE* file = fopen(filename.toStdString().c_str(), "rb");
                if (file) {
                    int bytesRead = fread(data, 1, filesize, file);
                    if (bytesRead == filesize) {
                        newObjectId = mYaffsSaveControl->addFile(fileItem->getHeader(), newHeaderPos, data, filesize);
                        saved = true;
                    }
                }
                delete[] data;
            }
            else
            {
                int headerPosition = fileItem->getHeaderPosition();
                YaffsControl yaffsControl(mImageFilename.toStdString().c_str(), NULL);
                if (yaffsControl.open(YaffsControl::OPEN_READ)) {
                    char* data = yaffsControl.extractFile(headerPosition);//reading old iso files
                    if (data != NULL) {
                        newObjectId = mYaffsSaveControl->addFile(fileItem->getHeader(), newHeaderPos, data, filesize);
                        saved = true;
                    }
                }
            }

            if (saved) {
                fileItem->setHeaderPosition(newHeaderPos);
                fileItem->setObjectId(newObjectId);
                fileItem->setCondition(YaffsItem::CLEAN);
            }
        }
    }
}

void YaffsModel::saveSymLink(YaffsItem* symLinkItem) {
    if (symLinkItem) {
        YaffsItem* parentItem = symLinkItem->parent();
        if (parentItem) {
            qDebug() << "s: " << symLinkItem->getFullPath() << ", Parent: " << parentItem->getFullPath();
            int newHeaderPos = -1;
            int newObjectId = mYaffsSaveControl->addSymLink(symLinkItem->getHeader(), newHeaderPos);
            symLinkItem->setHeaderPosition(newHeaderPos);
            symLinkItem->setObjectId(newObjectId);
            symLinkItem->setCondition(YaffsItem::CLEAN);
        }
    }
}

QVariant YaffsModel::data(const QModelIndex& itemIndex, int role) const {
    QVariant result = QVariant();
    YaffsItem* item = static_cast<YaffsItem*>(itemIndex.internalPointer());
    if (itemIndex.isValid() && item) {
        if (role == Qt::DisplayRole) {
            if (item == mYaffsRoot && itemIndex.column() == YaffsItem::NAME) {
                result = "/";
            } else {
                result = item->data(itemIndex.column());
            }
        } else if (role == Qt::ForegroundRole) {
            if (itemIndex.column() == YaffsItem::NAME) {
                if (item->isDir()) {
                    result = QVariant(QColor(Qt::blue));
                } else if (item->isFile()) {
                    result = QVariant(QColor(Qt::black));
                } else if (item->isSymLink()) {
                    result = QVariant(QColor(Qt::darkGreen));
                }
            }
        } else if (role == Qt::BackgroundRole) {
//            static const QColor orange(255, 165, 0);

            switch (item->getCondition()) {
            case YaffsItem::CLEAN:
                break;
            case YaffsItem::DIRTY:
//                result = orange;
                break;
            case YaffsItem::NEW:
//                result = Qt::green;
                break;
            case YaffsItem::MOVED:
                break;
            }
        } else if (role == Qt::FontRole) {
            if (itemIndex.column() == YaffsItem::PERMISSIONS) {
                result = QFont("Courier");
            }
        } else if (role == Qt::EditRole) {
            switch (itemIndex.column()) {
            case YaffsItem::NAME:
                result = item->getName();
                break;
            case YaffsItem::PERMISSIONS:
                result = item->getPermissions();
                break;
            case YaffsItem::ALIAS:
                result = item->getAlias();
                break;
            case YaffsItem::USER:
                result = item->getUserId();
                break;
            case YaffsItem::GROUP:
                result = item->getGroupId();
                break;
            }
        }
    }
    return result;
}

bool YaffsModel::setData(const QModelIndex& itemIndex, const QVariant& value, int role) {
    bool result = false;
    if (role == Qt::EditRole) {
        YaffsItem* item = static_cast<YaffsItem*>(itemIndex.internalPointer());
        if (item) {
            switch (itemIndex.column()) {
            case YaffsItem::NAME:
                item->setName(value.toString());
                result = true;
                break;
            case YaffsItem::PERMISSIONS:
                item->setPermissions(value.toUInt());
                result = true;
                break;
            case YaffsItem::ALIAS:
                item->setAlias(value.toString());
                result = true;
                break;
            case YaffsItem::USER:
                item->setUserId(value.toUInt());
                result = true;
                break;
            case YaffsItem::GROUP:
                item->setGroupId(value.toUInt());
                result = true;
                break;
            }
        }
        mItemsDirty += (item->getCondition() == YaffsItem::DIRTY ? 1 : 0);
    }

    if (result) {
        emit dataChanged(itemIndex, itemIndex);
    }

    return result;
}

Qt::ItemFlags YaffsModel::flags(const QModelIndex& itemIndex) const {
    Qt::ItemFlags flags = Qt::NoItemFlags;
    if (itemIndex.isValid()) {
        flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
        if (itemIndex.column() == YaffsItem::NAME) {
            flags |= Qt::ItemIsEditable;
        }
    }
    return flags;
}

QVariant YaffsModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (role == Qt::DisplayRole) {
        if (orientation == Qt::Horizontal) {
            if (section == YaffsItem::NAME) {
                return "Name";
            } else if (section == YaffsItem::SIZE) {
                return "Size";
            } else if (section == YaffsItem::PERMISSIONS) {
                return "Permissions";
            } else if (section == YaffsItem::ALIAS) {
                return "Alias";
            } else if (section == YaffsItem::DATE_ACCESSED) {
                return "Date Accessed";
            } else if (section == YaffsItem::DATE_CREATED) {
                return "Date Created";
            } else if (section == YaffsItem::DATE_MODIFIED) {
                return "Date Modified";
            } else if (section == YaffsItem::USER) {
                return "User";
            } else if (section == YaffsItem::GROUP) {
                return "Group";
            }
#ifdef QT_DEBUG
            else if (section == YaffsItem::OBJECTID) {
                return "ObjectId";
            } else if (section == YaffsItem::PARENTID) {
                return "ParentId";
            } else if (section == YaffsItem::HEADERPOS) {
                return "HeaderPos";
            }
#endif  //QT_DEBUG
        }
    }
    return QVariant();
}

QModelIndex YaffsModel::index(int row, int column, const QModelIndex& parentIndex) const {
    YaffsItem* parent = NULL;

    if (mYaffsRoot && (!parentIndex.isValid() || parentIndex == QModelIndex())) {
//        parent = mYaffsRoot;
        if (row == 0) {
            return createIndex(row, column, mYaffsRoot);
        }
    } else {
        parent = static_cast<YaffsItem*>(parentIndex.internalPointer());
    }

    if (parent) {
        YaffsItem* item = parent->child(row);
        if (item) {
            return createIndex(row, column, item);
        }
    }

    return QModelIndex();
}

QModelIndex YaffsModel::parent(const QModelIndex& itemIndex) const {
    YaffsItem* item = static_cast<YaffsItem*>(itemIndex.internalPointer());
    if (item && item != mYaffsRoot) {
        YaffsItem* parent = item->parent();
        if (parent) {
            return createIndex(parent->row(), 0, parent);
        }
    }
    return QModelIndex();
}

int YaffsModel::rowCount(const QModelIndex& parentIndex) const {
    YaffsItem* parent = NULL;
    int count = 0;

    if (!parentIndex.isValid() || parentIndex == QModelIndex()) {
        if (mYaffsRoot) {
            count = 1;
        }
    } else {
        parent = static_cast<YaffsItem*>(parentIndex.internalPointer());
    }

    if (parent) {
        count = parent->childCount();
    }

    return count;
}

int YaffsModel::columnCount(const QModelIndex& parentIndex) const {
    return YaffsItem::COLUMN_COUNT;
}

int YaffsModel::removeRows(const QModelIndexList& selectedRows) {
    //mark all selected items for delete
    foreach (QModelIndex index, selectedRows) {
        YaffsItem* item = static_cast<YaffsItem*>(index.internalPointer());
        if (item) {
            item->markForDelete();
        }
    }

    int itemsDeleted = 0;
    if (mYaffsRoot->hasChildMarkedForDelete()) {
        QList<int> rowsToDelete;

        //iterate through child items to build up list of items to delete
        for (int i = 0; i < mYaffsRoot->childCount(); ++i) {
            YaffsItem* childItem = mYaffsRoot->child(i);
            if (childItem->isMarkedForDelete()) {
                rowsToDelete.append(childItem->row());
            } else if (childItem->hasChildMarkedForDelete()) {
                itemsDeleted += processChildItemsForDelete(childItem);
                mYaffsRoot->setHasChildMarkedForDelete(false);
            }
        }

        itemsDeleted += calculateAndDeleteContiguousRows(rowsToDelete, mYaffsRoot);
    }
    return itemsDeleted;

    //iterate through ALL items and process the marked ones
    //return processChildItemsForDelete(mYaffsRoot);
}

int YaffsModel::processChildItemsForDelete(YaffsItem* item) {
    int itemsDeleted = 0;
    if (item->hasChildMarkedForDelete()) {
        QList<int> rowsToDelete;

        //iterate through child items to build up list of items to delete
        for (int i = 0; i < item->childCount(); ++i) {
            YaffsItem* childItem = item->child(i);
            if (childItem->isMarkedForDelete()) {
                rowsToDelete.append(childItem->row());
            } else if (childItem->hasChildMarkedForDelete()) {
                itemsDeleted += processChildItemsForDelete(childItem);
                item->setHasChildMarkedForDelete(false);
            }
        }

        itemsDeleted += calculateAndDeleteContiguousRows(rowsToDelete, item);
    }
    return itemsDeleted;
}

bool variantLessThan(const QVariant &v1, const QVariant &v2)
 {
     return v1.toInt() < v2.toInt();
 }

int YaffsModel::calculateAndDeleteContiguousRows(QList<int>& rows, YaffsItem* parentItem) {
    int itemsDeleted = 0;

    std::sort(rows.begin(),rows.end(),variantLessThan);

    int size = rows.size();
    int thisRow, nextRow;
    int count = 1;

    for (int i = size - 1; i >= 0; --i) {
        thisRow = rows.at(i);
        bool lastRow = (i == 0);

        if (!lastRow) {
            nextRow = rows.at(i - 1);
        }

        if (lastRow || nextRow != thisRow - 1) {
            qDebug() << "Removing rows (start, count): (" << thisRow << ", " << count << ")";
            QModelIndex parentIndex = createIndex(parentItem->row(), 0, parentItem);
            itemsDeleted += deleteRows(thisRow, count, parentIndex);
            count = 0;

            if (lastRow) {
                break;
            }
        }

        ++count;
    }

    return itemsDeleted;
}

int YaffsModel::deleteRows(int row, int count, const QModelIndex& parentIndex) {
    int itemsDeleted = 0;

    if (parentIndex.isValid()) {
        beginRemoveRows(parentIndex, row, row + (count - 1));
        for (int i = row + (count - 1); i >= row; --i) {
            YaffsItem* parentItem = static_cast<YaffsItem*>(parentIndex.internalPointer());
            parentItem->removeChild(row);
            itemsDeleted++;
        }
        endRemoveRows();
        emit layoutChanged();
    }

    mItemsDeleted += itemsDeleted;
    return itemsDeleted;
}

//from YaffsReaderObserver
void YaffsModel::newItem(int yaffsObjectId, const yaffs_obj_hdr* yaffsObjectHeader, int fileOffset) {
    if (yaffsObjectId == YAFFS_OBJECTID_ROOT) {
        mYaffsRoot = new YaffsItem(NULL, yaffsObjectHeader, fileOffset, yaffsObjectId);
        mYaffsObjectsItemMap.insert(YAFFS_OBJECTID_ROOT, mYaffsRoot);
        return;
    }

    //get childs parent
    YaffsItem* parent = mYaffsObjectsItemMap.value(yaffsObjectHeader->parent_obj_id);

    //create item and map it
    YaffsItem* child = new YaffsItem(parent, yaffsObjectHeader, fileOffset, yaffsObjectId);
    mYaffsObjectsItemMap.insert(yaffsObjectId, child);

    if (parent) {
        //add child to parent
        parent->appendChild(child);
    } else {
        qDebug() << "error, parent not found, id: " << yaffsObjectHeader->parent_obj_id;
        mYaffsObjectsWithoutParent.append(child);
    }
}

void YaffsModel::readComplete() {
    //if image didn't contain a root but did contain other stuff, give model a root
    if (mYaffsRoot == NULL && mYaffsObjectsItemMap.size() > 0) {
        mYaffsRoot = YaffsItem::createRoot();
        mYaffsObjectsItemMap.insert(YAFFS_OBJECTID_ROOT, mYaffsRoot);
    }

    if (mYaffsObjectsWithoutParent.size() > 0) {
        //child objects might have been before parent
        foreach (YaffsItem* child, mYaffsObjectsWithoutParent) {
            YaffsItem* parent = mYaffsObjectsItemMap.value(child->getHeader().parent_obj_id);
            if (parent) {
                parent->appendChild(child);
                qDebug() << "child came before parent in file, parent id: " << child->getHeader().parent_obj_id;
            } else {
                qDebug() << "parent still not found, item name: " << child->getName();
            }
        }
    }
    mYaffsObjectsWithoutParent.clear();
}
