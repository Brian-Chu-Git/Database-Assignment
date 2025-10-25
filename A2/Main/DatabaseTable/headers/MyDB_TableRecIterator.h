#ifndef MYDB_TABLERECITERATOR_H
#define MYDB_TABLERECITERATOR_H

#include "MyDB_RecordIterator.h"
#include "MyDB_Record.h"

// Forward declaration to avoid circular include
class MyDB_TableReaderWriter;

class MyDB_TableRecIterator : public MyDB_RecordIterator {
public:
    MyDB_TableRecIterator(MyDB_TableReaderWriter &table, MyDB_RecordPtr record);
    void getNext();
    bool hasNext();

private:
    MyDB_TableReaderWriter &tableRw;
    MyDB_RecordPtr record;
    size_t currentPage;
    MyDB_RecordIteratorPtr pageIterator;
};

#endif