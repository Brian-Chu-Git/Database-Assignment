#ifndef TABLE_ITER
#define TABLE_ITER

#include "MyDB_TableRecIterator.h"
#include "MyDB_PageReaderWriter.h"

#include "MyDB_TableRecIterator.h"
#include "MyDB_TableReaderWriter.h"
#include "MyDB_PageReaderWriter.h"

MyDB_TableRecIterator::MyDB_TableRecIterator(MyDB_TableReaderWriter &table, MyDB_RecordPtr record) :
    tableRw(table), record(record), currentPage(0), pageIterator(nullptr) {

    if (tableRw.getTable()->lastPage() != (size_t)-1) {
        MyDB_PageReaderWriter firstPage = tableRw[0];
        pageIterator = firstPage.getIterator(record);
    }
}

bool MyDB_TableRecIterator::hasNext() {
    while (pageIterator) {
        if (pageIterator->hasNext()) {
            return true;
        }
        // move to next page
        currentPage++;
        if (currentPage > tableRw.getTable()->lastPage()) {
            pageIterator = nullptr;
            return false;
        }
        MyDB_PageReaderWriter nextPage = tableRw[currentPage];
        pageIterator = nextPage.getIterator(record);
    }
    return false;
}

void MyDB_TableRecIterator::getNext() {
    if (!pageIterator) return;

    if (!pageIterator->hasNext()) {
        // move to next page until we find a non-empty one
        currentPage++;
        if (currentPage > tableRw.getTable()->lastPage()) {
            pageIterator = nullptr;
            return;
        }
        MyDB_PageReaderWriter nextPage = tableRw[currentPage];
        pageIterator = nextPage.getIterator(record);
    }

    if (pageIterator && pageIterator->hasNext()) {
        pageIterator->getNext();
    }
}



#endif