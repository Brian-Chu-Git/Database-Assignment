#ifndef PAGE_ITER
#define PAGE_ITER

#include "MyDB_PageRecIterator.h"

MyDB_PageRecIterator::MyDB_PageRecIterator(MyDB_PageReaderWriter &pageRWIn, MyDB_RecordPtr recordIn) :
    pageRw(pageRWIn), record(recordIn), currentRecord(0){
    char* raw = (char*)pageRw.page->getBytes();
    PageHeader* header = (PageHeader*)raw;

    totalRecords = header->numRecords;
    slots = (int*)header->slotDirectory;
    rawBytes = raw;
}

bool MyDB_PageRecIterator::hasNext() {
    return currentRecord < totalRecords;
}

void MyDB_PageRecIterator::getNext() {
    if (!hasNext()) return;

    int recordStart = slots[currentRecord];
    record->fromBinary(rawBytes + recordStart);
    currentRecord++;
}

#endif