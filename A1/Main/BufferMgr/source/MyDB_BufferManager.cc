
#ifndef BUFFER_MGR_C
#define BUFFER_MGR_C

#include "MyDB_BufferManager.h"
#include "MyDB_Page.h"
#include <string>
#include <fstream>
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <unordered_map>
#include <tuple>

using namespace std;

MyDB_PageHandle MyDB_BufferManager :: getPage (MyDB_TablePtr whichTable, long i) {
	for (auto page : activePages) {
		if (!page->isTempPage() && page->getTable() == whichTable && page->getPageID() == i) {
			return make_shared<MyDB_PageHandleBase>(page);
		}
	}

	PageKey key{whichTable, i};
	auto it = pageMap.find(key);
	if (it != pageMap.end()) {
		return make_shared<MyDB_PageHandleBase>(it->second);
	}

	MyDB_PagePtr page = make_shared<MyDB_Page>(whichTable, i, false, this, nullptr);
	pageMap[key] = page;
	return make_shared<MyDB_PageHandleBase>(page);
}

MyDB_PageHandle MyDB_BufferManager :: getPage () {
	MyDB_PagePtr page = make_shared<MyDB_Page>(nullptr, -1, true, this, nullptr);
	return make_shared<MyDB_PageHandleBase>(page);
}

MyDB_PageHandle MyDB_BufferManager :: getPinnedPage (MyDB_TablePtr whichTable, long i) {
	for (auto page : activePages) {
		if (!page->isTempPage() && page->getTable() == whichTable && page->getPageID() == i) {
			page->setPinned(true);
			return make_shared<MyDB_PageHandleBase>(page);
		}
	}

	PageKey key{whichTable, i};
	auto it = pageMap.find(key);
	if (it != pageMap.end()) {
		it->second->setPinned(true);
		return make_shared<MyDB_PageHandleBase>(it->second);
	}

	MyDB_PagePtr page = make_shared<MyDB_Page>(whichTable, i, false, this, nullptr);
	page->setPinned(true);
	pageMap[key] = page;
	return make_shared<MyDB_PageHandleBase>(page);
}

MyDB_PageHandle MyDB_BufferManager :: getPinnedPage () {
	MyDB_PagePtr page = make_shared<MyDB_Page>(nullptr, -1, true, this, nullptr);
	page->setPinned(true);
	return make_shared<MyDB_PageHandleBase>(page);
}

void MyDB_BufferManager :: unpin (MyDB_PageHandle unpinMe) {
	if (!unpinMe) return;
	MyDB_PagePtr page = unpinMe->getPage();
	if (page) {
		page->setPinned(false);
	}
}

MyDB_BufferManager :: MyDB_BufferManager (size_t pageSize, size_t numPages, string tempFile) {
	this->pageSize = pageSize;
	this->numPages = numPages;

	bufferPool = malloc(pageSize * numPages);
	// Assume it correctly allocates memory

	freePages.reserve(numPages);
	for (size_t i = 0; i < numPages; i++) {
		freePages.push_back(static_cast<char*>(bufferPool) + i * pageSize);
	}

	clockHand = 0;

	this->tempFile = tempFile;

	if (!tempFile.empty()) {
		int fd = open(tempFile.c_str(), O_RDWR | O_CREAT, 0666);
		close(fd);
	}
}

MyDB_BufferManager :: ~MyDB_BufferManager () {
	for (auto page : activePages) {
		if (page->isDirty() && !page->isTempPage()) {
			writeBack(page);
		}
	}

	if (bufferPool) {
		free(bufferPool);
		bufferPool = nullptr;
	}

	if (!tempFile.empty()) {
		remove(tempFile.c_str());
	}

	activePages.clear();
	freePages.clear();
}


void* MyDB_BufferManager :: getFreeSlot (MyDB_PagePtr requestingPage) {
	if (!freePages.empty()) {
		void* slot = freePages.back();
		freePages.pop_back();
		activePages.push_back(requestingPage);
		return slot;
	}

	size_t n = activePages.size();
	size_t start = clockHand;
	size_t rotations = 0;

	while (rotations < 2) {
		clockHand %= n;
		MyDB_PagePtr candidate = activePages[clockHand];

		cout << "Clock Hand at " << clockHand << "\n";
		if (candidate->isPinned()) {
			clockHand = (clockHand + 1) % n;
			cout << clockHand << " Is pinned\n";
			if (clockHand == start) {
				rotations++;
			}
			continue;
		}

		if (candidate->getRefBit()) {
			candidate->setRefBit(false);
			clockHand = (clockHand + 1) % n;
			if (clockHand == start) {
				rotations++;
			}
			continue;
		}

		if (candidate->isDirty()) {
			writeBack(candidate);
		}
		void* slot = candidate->getRawBytes();
		candidate->resetPage();
		activePages[clockHand] = requestingPage;
		clockHand = (clockHand + 1) % n;

		return slot;
	}

	cerr << "Was not able to find a slot\n";
	return nullptr;
}

void MyDB_BufferManager :: writeBack (MyDB_PagePtr page) {
	if (!page || !page->isDirty()) return;

	if (!page->isTempPage()) {
		int fd = open(page->getTable()->getStorageLoc().c_str(), O_RDWR | O_CREAT, 0666);
		lseek(fd, page->getPageID() * pageSize, SEEK_SET);
		write(fd, page->getRawBytes(), pageSize);
		close(fd);
	}
	else {
		cout << "Writing back temporary page " << page->getTempSlot() << "\n";
		int fd = open(tempFile.c_str(), O_RDWR | O_CREAT, 0666);
		lseek(fd, page->getTempSlot() * pageSize, SEEK_SET);
		write(fd, page->getRawBytes(), pageSize);
		close(fd);
	}

	page->setDirty(false);
}
	
#endif


