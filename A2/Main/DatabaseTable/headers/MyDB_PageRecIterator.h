#ifndef MYDB_PAGERECITERATOR_H
#define MYDB_PAGERECITERATOR_H

#include "MyDB_PageReaderWriter.h"
#include "MyDB_Record.h"

class MyDB_PageRecIterator : public MyDB_RecordIterator {
public:
    MyDB_PageRecIterator(MyDB_PageReaderWriter &pageRW, MyDB_RecordPtr record);
    bool hasNext();
    void getNext();

private:
    MyDB_PageReaderWriter &pageRw;
    MyDB_RecordPtr record;
    int currentRecord;
    int totalRecords;

    char* rawBytes;
    int* slots;
};

#endif