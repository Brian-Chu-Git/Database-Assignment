#ifndef PAGE_H
#define PAGE_H

#include "MyDB_Table.h"
#include <memory>
#include <string>

class MyDB_BufferManager;

class MyDB_Page;
typedef shared_ptr <MyDB_Page> MyDB_PagePtr;

class MyDB_Page : public std::enable_shared_from_this<MyDB_Page> {
public:
    MyDB_Page (MyDB_TablePtr tableIn, long pageIDIn, bool isTemp, MyDB_BufferManager* myMgr, void* slot);

    void* getBytes ();
    void* getRawBytes() {return bytes;}
    void wroteBytes ();

    void setPinned (bool val) {pinned = val;}

    void incRefCount ();
    void decRefCount ();

    bool isPinned () const {return pinned;}
    bool isDirty () const {return dirty;}
    bool isTempPage () const {return tempPage;}
    long getPageID () const {return pageID;}
    MyDB_TablePtr getTable () const {return table;}
    int getRefCount () const {return refCount;}
    bool getRefBit () const {return refBit;}


    void setRefBit (bool val) {refBit = val;}
    void setTable (MyDB_TablePtr tableIn) {table = tableIn;}
    void setPageID (long pageIDIn) {pageID = pageIDIn;}
    void setBytes (void* slot) {bytes = slot;}
    void setTempSlot(size_t slot) {tempSlot = slot;}
    void setDirty(bool val) {dirty = val;}
    size_t getTempSlot() const {return tempSlot;}

    void resetPage ();

private:
    void* bytes;
    MyDB_BufferManager* myMgr;

    MyDB_TablePtr table;
    long pageID;
    bool tempPage;
    bool dirty;
    bool pinned;
    int refCount;
    bool refBit;

    size_t tempSlot;
};

#endif
