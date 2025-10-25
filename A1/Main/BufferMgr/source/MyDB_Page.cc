#include "MyDB_Page.h"
#include "MyDB_BufferManager.h"
#include "../../Catalog/headers/MyDB_Table.h"
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <iostream>

MyDB_Page::MyDB_Page (MyDB_TablePtr tableIn, long pageIDIn, bool isTemp, MyDB_BufferManager* myMgr, void* slot)
    : bytes(slot), myMgr(myMgr), table(tableIn), pageID(pageIDIn), tempPage(isTemp), 
    dirty(false), pinned(false), refCount(0), refBit(false), tempSlot(-1) {}

void* MyDB_Page::getBytes () {
    refBit = true;

    if (bytes != nullptr) return bytes;

    if (!myMgr) return nullptr;

    void* slot = myMgr->getFreeSlot(shared_from_this());
    if (!slot) {
        cerr << "Null Slot\n";
        return nullptr;
    }
    bytes = slot;

    if (!tempPage && table) {
        int fd = open(table->getStorageLoc().c_str(), O_RDONLY);
        if (fd < 0) {
            memset(bytes, 0, myMgr->getPageSize());
        }
        else {
            off_t offset = pageID * myMgr->getPageSize();
            if (lseek(fd, offset, SEEK_SET) < 0) {
                memset(bytes, 0, myMgr->getPageSize());
            }
            else {
                ssize_t n = read(fd, bytes, myMgr->getPageSize());
                if (n < 0) n = 0;
                if (n < (size_t)myMgr->getPageSize()) {
                    memset(((char*)bytes) + n, 0, myMgr->getPageSize() - n);
                }
            }
        }
        close(fd);
    }
    
    if (tempPage) {
        if (tempSlot == -1) {
            tempSlot = myMgr->getNextTempSlot();
            memset(bytes, 0, myMgr->getPageSize());
        }
        else {
            int fd = open(myMgr->getTempFile().c_str(), O_RDONLY);
            if (fd < 0) {
                memset(bytes, 0, myMgr->getPageSize());
            }
            else {
                off_t offset = tempSlot * myMgr->getPageSize();
                if (lseek(fd, offset, SEEK_SET) < 0) {
                    memset(bytes, 0, myMgr->getPageSize());
                }
                else {
                    ssize_t n = read(fd, bytes, myMgr->getPageSize());
                    if (n < 0) n = 0;
                    if (n < (size_t)myMgr->getPageSize()) {
                        memset(((char*)bytes) + n, 0, myMgr->getPageSize() - n);
                    }
                }
            }
            close(fd);
        }
    }

    dirty = false;
    return bytes;
}

void MyDB_Page::wroteBytes () {
    dirty = true;
}

void MyDB_Page::incRefCount () {
    refCount++;
}

void MyDB_Page::decRefCount () {
    if (refCount > 0) refCount--;
}

void MyDB_Page::resetPage () {
    table = nullptr;
    pageID = -1;
    tempPage = false;
    dirty = false;
    pinned = false;
    refCount = 0;
    refBit = false;
    tempSlot = -1;
    bytes = nullptr;
}