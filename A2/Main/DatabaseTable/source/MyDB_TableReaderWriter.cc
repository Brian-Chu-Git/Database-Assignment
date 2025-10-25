#ifndef TABLE_RW_C
#define TABLE_RW_C

#include <fstream>
#include "MyDB_PageReaderWriter.h"
#include "MyDB_TableReaderWriter.h"

using namespace std;

MyDB_TableReaderWriter::MyDB_TableReaderWriter(MyDB_TablePtr forMe, MyDB_BufferManagerPtr myBuffer) :
    table(forMe), bufferMgr(myBuffer) {}

MyDB_PageReaderWriter MyDB_TableReaderWriter::operator[](size_t i) {
    for (size_t p = table->lastPage() + 1; p <= i; p++) {
        MyDB_PageReaderWriter page(bufferMgr->getPage(table, p), table);
        page.clear();
    }
    return MyDB_PageReaderWriter(bufferMgr->getPage(table, i), table);
}

MyDB_RecordPtr MyDB_TableReaderWriter::getEmptyRecord() {
    return make_shared<MyDB_Record>(table->getSchema());
}

MyDB_PageReaderWriter MyDB_TableReaderWriter::last() {
    int lastPageIndex = table->lastPage();

    if (lastPageIndex == -1) {
        table->setLastPage(0);
        lastPageIndex = 0;
        MyDB_PageReaderWriter firstPage(bufferMgr->getPage(table, 0), table);
        firstPage.clear();
        return firstPage;
    }

    MyDB_PageReaderWriter page(bufferMgr->getPage(table, lastPageIndex), table);
    return page;
}

void MyDB_TableReaderWriter::append(MyDB_RecordPtr appendMe) {
    if (appendMe->getBinarySize() > bufferMgr->getPageSize() - sizeof(PageHeader) - sizeof(int)) {
        cerr << "Record too large to fit in empty page" << endl;
        exit(1);
    }

	int lastPageIndex = table->lastPage();
	MyDB_PageReaderWriter pageToUse(
		(lastPageIndex == -1) ? bufferMgr->getPage(table, 0) 
							: bufferMgr->getPage(table, lastPageIndex),
		table
	);

	if (lastPageIndex == -1) {
		pageToUse.clear();
		table->setLastPage(0);
	}

    if (lastPageIndex == -1) {
        pageToUse = MyDB_PageReaderWriter(bufferMgr->getPage(table, 0), table);
        pageToUse.clear();
        table->setLastPage(0);
    } else {
        pageToUse = MyDB_PageReaderWriter(bufferMgr->getPage(table, lastPageIndex), table);
    }

    if (!pageToUse.append(appendMe)) {
        size_t newPageNum = table->lastPage() + 1;
        MyDB_PageReaderWriter newPage(bufferMgr->getPage(table, newPageNum), table);
        newPage.clear(); 
        newPage.append(appendMe);
        table->setLastPage(newPageNum);
    }
}

void MyDB_TableReaderWriter::loadFromTextFile(string fromMe) {
    int lastPage = table->lastPage();
    for (int i = 0; i <= lastPage; i++) {
        MyDB_PageReaderWriter page(bufferMgr->getPage(table, i), table);
        page.clear();
    }
    table->setLastPage(-1);

    ifstream infile(fromMe);
    if (!infile.is_open()) {
        cerr << "Could not open file " << fromMe << endl;
        exit(1);
    }

    string line;
    while (getline(infile, line)) {
        MyDB_RecordPtr rec = getEmptyRecord();
        rec->fromString(line);
        append(rec);
    }

    infile.close();
}

MyDB_RecordIteratorPtr MyDB_TableReaderWriter::getIterator(MyDB_RecordPtr iterateIntoMe) {
    return make_shared<MyDB_TableRecIterator>(*this, iterateIntoMe);
}

void MyDB_TableReaderWriter::writeIntoTextFile(string toMe) {
    ofstream outfile(toMe);
    if (!outfile.is_open()) {
        cerr << "Could not open output file " << toMe << endl;
        exit(1);
    }

    MyDB_RecordPtr rec = getEmptyRecord();
    MyDB_RecordIteratorPtr iter = getIterator(rec);

    while (iter->hasNext()) {
        iter->getNext();
        outfile << *rec << "\n";
    }

    outfile.close();
}

#endif
