
#ifndef PAGE_HANDLE_C
#define PAGE_HANDLE_C

#include <memory>
#include "MyDB_PageHandle.h"
#include "MyDB_Page.h"
#include <iostream>

void *MyDB_PageHandleBase :: getBytes () {
	if (!page){
		return nullptr;
	}
	return page->getBytes();
}

void MyDB_PageHandleBase :: wroteBytes () {
	if (!page) return;
	page->wroteBytes();
}

MyDB_PageHandleBase :: ~MyDB_PageHandleBase () {
	if (!page) return;
	page->decRefCount();
	if (page->getRefCount() == 0) {
		page->setPinned(false);
	}
}

#endif

