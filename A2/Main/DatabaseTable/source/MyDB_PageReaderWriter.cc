
#ifndef PAGE_RW_C
#define PAGE_RW_C

#include "MyDB_PageReaderWriter.h"

MyDB_PageReaderWriter :: MyDB_PageReaderWriter (MyDB_PageHandle pageIn, MyDB_TablePtr tableIn) : 
page(pageIn), table(tableIn) {
	char* raw = (char*) page->getBytes();
	header = (PageHeader*) raw;
}

void MyDB_PageReaderWriter :: clear () {
	char* raw = (char*) page->getBytes();

	header->numRecords = 0;
	header->recordSpace = 0;
	header->pageType = MyDB_PageType :: RegularPage;
	page->wroteBytes();
}

MyDB_PageType MyDB_PageReaderWriter :: getType () {
	return header->pageType;
}

MyDB_RecordIteratorPtr MyDB_PageReaderWriter :: getIterator (MyDB_RecordPtr iterateIntoMe) {
	return make_shared<MyDB_PageRecIterator>(*this, iterateIntoMe);
}

void MyDB_PageReaderWriter :: setType (MyDB_PageType newType) {
	header->pageType = newType;
	page->wroteBytes();
}

bool MyDB_PageReaderWriter :: append (MyDB_RecordPtr appendMe) {
	char* raw = (char*) page->getBytes();
	int* slots = (int*) header->slotDirectory;

	int recordSize = appendMe->getBinarySize();
	int requiredSpace = recordSize + sizeof(int);

	int freeSpaceStart = header->numRecords * sizeof(int) + sizeof(PageHeader);
	
	int pageSize = page->getParent().getPageSize();

	if (requiredSpace > pageSize - header->recordSpace - freeSpaceStart) {
		return false;
	}

	int recordStart = pageSize - header->recordSpace - recordSize;
	appendMe->toBinary(raw + recordStart);

	slots[header->numRecords] = recordStart;
	header->numRecords++;

	header->recordSpace += recordSize;

	page->wroteBytes();
	return true;
}

#endif
