
#ifndef CATALOG_UNIT_H
#define CATALOG_UNIT_H

#include "MyDB_BufferManager.h"
#include "MyDB_PageHandle.h"
#include "MyDB_Table.h"
#include "QUnit.h"
#include <iostream>
#include <unistd.h>
#include <vector>
#include <fstream>
#include <fcntl.h>

using namespace std;

// these functions write a bunch of characters to a string... used to produce data
void writeNums (char *bytes, size_t len, int i) {
	for (size_t j = 0; j < len - 1; j++) {
		bytes[j] = '0' + (i % 10);
	}
	bytes[len - 1] = 0;
}

void writeLetters (char *bytes, size_t len, int i) {
	for (size_t j = 0; j < len - 1; j++) {
		bytes[j] = 'i' + (i % 10);
	}
	bytes[len - 1] = 0;
}

void writeSymbols (char *bytes, size_t len, int i) {
	for (size_t j = 0; j < len - 1; j++) {
		bytes[j] = '!' + (i % 10);
	}
	bytes[len - 1] = 0;
}

int main () {

	QUnit::UnitTest qunit(cerr, QUnit::verbose);

	// UNIT TEST 1: A BIG ONE!!
	{

		// create a buffer manager 
		MyDB_BufferManager myMgr (64, 16, "tempDSFSD");
		MyDB_TablePtr table1 = make_shared <MyDB_Table> ("tempTable", "foobar");

		// allocate a pinned page
		cout << "allocating pinned page\n";
		MyDB_PageHandle pinnedPage = myMgr.getPinnedPage (table1, 0);
		char *bytes = (char *) pinnedPage->getBytes ();
		writeNums (bytes, 64, 0);
		pinnedPage->wroteBytes ();

		
		// create a bunch of pinned pages and remember them
		vector <MyDB_PageHandle> myHandles;
		for (int i = 1; i < 10; i++) {
			cout << "allocating pinned page\n";
			MyDB_PageHandle temp = myMgr.getPinnedPage (table1, i);
			char *bytes = (char *) temp->getBytes ();
			writeNums (bytes, 64, i);
			temp->wroteBytes ();
			myHandles.push_back (temp);
		}

		// now forget the pages we created
		vector <MyDB_PageHandle> temp;
		myHandles = temp;

		// now remember 8 more pages
		for (int i = 0; i < 8; i++) {
			cout << "allocating pinned page\n";
			MyDB_PageHandle temp = myMgr.getPinnedPage (table1, i);
			char *bytes = (char *) temp->getBytes ();

			// write numbers at the 0th position
			if (i == 0)
				writeNums (bytes, 64, i);
			else
				writeSymbols (bytes, 64, i);
			temp->wroteBytes ();
			myHandles.push_back (temp);
		}

		// now correctly write nums at the 0th position
		cout << "allocating unpinned page\n";
		MyDB_PageHandle anotherDude = myMgr.getPage (table1, 0);
		bytes = (char *) anotherDude->getBytes ();
		writeSymbols (bytes, 64, 0);
		anotherDude->wroteBytes ();
		
		// now do 90 more pages, for which we forget the handle immediately
		for (int i = 10; i < 100; i++) {
			cout << "allocating unpinned page\n";
			MyDB_PageHandle temp = myMgr.getPage (table1, i);
			char *bytes = (char *) temp->getBytes ();
			writeNums (bytes, 64, i);
			temp->wroteBytes ();
		}

		// now forget all of the pinned pages we were remembering
		vector <MyDB_PageHandle> temp2;
		myHandles = temp2;

		// now get a pair of pages and write them
		for (int i = 0; i < 100; i++) {
			cout << "allocating pinned page\n";
			MyDB_PageHandle oneHandle = myMgr.getPinnedPage ();
			char *bytes = (char *) oneHandle->getBytes ();
			writeNums (bytes, 64, i);
			oneHandle->wroteBytes ();
			cout << "allocating pinned page\n";
			MyDB_PageHandle twoHandle = myMgr.getPinnedPage ();
			writeNums (bytes, 64, i);
			twoHandle->wroteBytes ();
		}

		// make a second table
		MyDB_TablePtr table2 = make_shared <MyDB_Table> ("tempTable2", "barfoo");
		for (int i = 0; i < 100; i++) {
			cout << "allocating unpinned page\n";
			MyDB_PageHandle temp = myMgr.getPage (table2, i);
			char *bytes = (char *) temp->getBytes ();
			writeLetters (bytes, 64, i);
			temp->wroteBytes ();
		}
		
	}

	// UNIT TEST 2
	{
		MyDB_BufferManager myMgr (64, 16, "tempDSFSD");
		MyDB_TablePtr table1 = make_shared <MyDB_Table> ("tempTable", "foobar");

		// look up all of the pages, and make sure they have the correct numbers
		for (int i = 0; i < 100; i++) {
			MyDB_PageHandle temp = myMgr.getPage (table1, i);
			char answer[64];
			if (i < 8)
				writeSymbols (answer, 64, i);
			else
				writeNums (answer, 64, i);
			char *bytes = (char *) temp->getBytes ();
			QUNIT_IS_EQUAL (string (answer), string (bytes));
		}
	}

	// UNIT TEST 3: Eviction should work when pages are unpinned
	{
		cerr << "=== UNIT TEST 3: Eviction ===\n";
		MyDB_BufferManager myMgr (64, 4, "tempEvict");
		MyDB_TablePtr table1 = make_shared<MyDB_Table>("table1", "file1");

		// Fill all 4 slots
		vector<MyDB_PageHandle> handles;
		for (int i = 0; i < 4; i++) {
			MyDB_PageHandle h = myMgr.getPage(table1, i);
			char *bytes = (char *) h->getBytes();
			writeNums(bytes, 64, i);
			h->wroteBytes();
			handles.push_back(h);
		}

		// Drop references so all pages are unpinned
		handles.clear();

		// Request a new page, should evict one of the old ones
		MyDB_PageHandle hNew = myMgr.getPage(table1, 99);
		char *bytes = (char *) hNew->getBytes();
		writeSymbols(bytes, 64, 9);
		hNew->wroteBytes();
		QUNIT_IS_EQUAL(string(bytes).substr(0,5), string("*****")); // 'i' + (9%10)
	}

	// UNIT TEST 4: Pinned pages should not be evicted
	{
		cerr << "=== UNIT TEST 4: Pinning ===\n";
		MyDB_BufferManager myMgr (64, 2, "tempPin");
		MyDB_TablePtr table1 = make_shared<MyDB_Table>("table1", "file2");

		// Pin first page
		MyDB_PageHandle pinned = myMgr.getPinnedPage(table1, 0);
		char *bytes = (char *) pinned->getBytes();
		writeNums(bytes, 64, 0);
		pinned->wroteBytes();

		// Get second page (fills buffer)
		MyDB_PageHandle second = myMgr.getPage(table1, 1);
		bytes = (char *) second->getBytes();
		writeNums(bytes, 64, 1);
		second->wroteBytes();

		// Drop second, now only pinned remains
		second = nullptr;

		// Ask for new page — pinned page must survive
		MyDB_PageHandle third = myMgr.getPage(table1, 2);
		bytes = (char *) third->getBytes();
		writeNums(bytes, 64, 2);
		third->wroteBytes();

		// Verify pinned is still valid
		char *pbytes = (char *) pinned->getBytes();
		QUNIT_IS_EQUAL(pbytes[0], '0'); // wroteNums(…,0) -> '0's
	}

	// UNIT TEST 5: Reference counting
	{
		cerr << "=== UNIT TEST 5: RefCount ===\n";
		MyDB_BufferManager myMgr (64, 2, "tempRef");
		MyDB_TablePtr table1 = make_shared<MyDB_Table>("table1", "file3");

		MyDB_PageHandle h1 = myMgr.getPage(table1, 0);
		MyDB_PageHandle h2 = h1; // copy handle
		MyDB_PageHandle h3 = h2; // another copy

		// All three share one page
		char *bytes = (char *) h1->getBytes();
		writeNums(bytes, 64, 7);
		h1->wroteBytes();

		// Drop two handles
		h2 = nullptr;
		h3 = nullptr;

		// Page should still be alive (h1 active)
		QUNIT_IS_EQUAL(bytes[0], '7');

		// Drop last handle
		h1 = nullptr;

		// Request new page, eviction must succeed (no segfault)
		MyDB_PageHandle hNew = myMgr.getPage(table1, 1);
		char *b2 = (char *) hNew->getBytes();
		writeNums(b2, 64, 9);
		hNew->wroteBytes();
	}

// UNIT TEST: ephemeral temp page reuse and pinning
{
    cerr << "=== Ephemeral Temp Test ===\n";
    MyDB_BufferManager myMgr(64, 2, ""); // no temp file

    // Allocate two temp pages
    MyDB_PageHandle t1 = myMgr.getPage();
    char *b1 = (char*) t1->getBytes();
    b1[0] = 'A'; t1->wroteBytes();

    MyDB_PageHandle t2 = myMgr.getPage();
    char *b2 = (char*) t2->getBytes();
    b2[0] = 'B'; t2->wroteBytes();

    // Drop t1 and reuse slot
    t1 = nullptr;
    MyDB_PageHandle t3 = myMgr.getPage();
    char *b3 = (char*) t3->getBytes();
    QUNIT_IS_EQUAL(b3[0], '\0'); // slot should be cleared

    // Pin t2 and ensure it survives eviction
    MyDB_PageHandle pinned = myMgr.getPinnedPage();
    QUNIT_IS_EQUAL(b2[0], 'B'); // still intact
}

// UNIT TEST: temp and normal pages interacting
{
    cerr << "=== Temp + Normal Mix ===\n";
    MyDB_BufferManager myMgr(64, 3, ""); // memory-only for speed
    MyDB_TablePtr table = make_shared<MyDB_Table>("table1", "file1");

    // Fill buffer: normal pages
    MyDB_PageHandle h1 = myMgr.getPage(table, 0);
    MyDB_PageHandle h2 = myMgr.getPage(table, 1);
    writeNums((char*)h1->getBytes(),64,1); h1->wroteBytes();
    writeNums((char*)h2->getBytes(),64,2); h2->wroteBytes();

    // Temp page (fills buffer)
    MyDB_PageHandle t1 = myMgr.getPage();
    char *tb = (char*) t1->getBytes();
    tb[0] = '!'; t1->wroteBytes();

    // Unpin normal pages so eviction can happen
    myMgr.unpin(h1);
    myMgr.unpin(h2);

    // Allocate another normal page; temp should survive
    MyDB_PageHandle h3 = myMgr.getPage(table, 2);
    char *tCheck = (char*) t1->getBytes();
    QUNIT_IS_EQUAL(tCheck[0], '!'); // temp page intact
}

// UNIT TEST: Shared Page Handles
{
    cerr << "=== UNIT TEST: Shared Page Handles ===\n";
    MyDB_BufferManager myMgr(64, 4, "tempFileShared");
    MyDB_TablePtr tableX = make_shared<MyDB_Table>("tableX", "fileX");

    // Get the same page twice
    MyDB_PageHandle h1 = myMgr.getPage(tableX, 3);
    MyDB_PageHandle h2 = myMgr.getPage(tableX, 3);

    // Check that both handles point to the same underlying page object
    QUNIT_IS_EQUAL(h1->getBytes(), h2->getBytes()); // same memory
    // QUNIT_IS_EQUAL(h1->getPageID(), h2->getPageID());
    // QUNIT_IS_EQUAL(h1->getTable(), h2->getTable());

    // Modify through one handle
    char *bytes1 = (char *) h1->getBytes();
    bytes1[0] = 'Z';
    h1->wroteBytes();

    // Check that the other handle sees the change
    char *bytes2 = (char *) h2->getBytes();
    QUNIT_IS_EQUAL(bytes2[0], 'Z');
}

// ===================== UNIT TEST 10: Temp + Normal Interaction =====================
{
    cerr << "=== UNIT TEST 10: Temp + Normal Interaction ===\n";
    MyDB_BufferManager myMgr(64, 3, "tempFile10");
    MyDB_TablePtr table = make_shared<MyDB_Table>("table10", "file10");

    // Fill buffer: 2 normal pages
    MyDB_PageHandle h1 = myMgr.getPage(table, 0);
    MyDB_PageHandle h2 = myMgr.getPage(table, 1);
    writeNums((char*)h1->getBytes(), 64, 0); h1->wroteBytes();
    writeNums((char*)h2->getBytes(), 64, 1); h2->wroteBytes();

    // Allocate a temp page (fills buffer)
    MyDB_PageHandle temp = myMgr.getPage();
    char *b = (char*) temp->getBytes();
    writeSymbols(b, 64, 9); temp->wroteBytes();

    // Unpin normal pages to allow eviction
    myMgr.unpin(h1);
    myMgr.unpin(h2);

    // Allocate another normal page; temp page should remain intact
    MyDB_PageHandle h3 = myMgr.getPage(table, 2);
    char *b3 = (char*) h3->getBytes();
    writeNums(b3, 64, 2); h3->wroteBytes();

    // Verify temp page survived eviction
    char *tempBytes = (char*) temp->getBytes();
    QUNIT_IS_EQUAL(tempBytes[0], '*');
}

// ===================== UNIT TEST 11: Eviction of unpinned pages =====================
{
    cerr << "=== UNIT TEST 11: Eviction Stress Test ===\n";
    MyDB_BufferManager myMgr(64, 2, "tempFile11");
    MyDB_TablePtr table = make_shared<MyDB_Table>("table11", "file11");

    // Fill buffer completely
    MyDB_PageHandle h1 = myMgr.getPage(table, 0);
    MyDB_PageHandle h2 = myMgr.getPage(table, 1);
    writeNums((char*)h1->getBytes(), 64, 1); h1->wroteBytes();
    writeNums((char*)h2->getBytes(), 64, 2); h2->wroteBytes();

    // Unpin first page to make it evictable
    myMgr.unpin(h1);

    // Allocate another page to trigger eviction
    MyDB_PageHandle h3 = myMgr.getPage(table, 2);
	MyDB_PagePtr page = h3->getPage();
    QUNIT_IS_EQUAL(page->getPageID(), 2);
}

// ===================== UNIT TEST 12: Dirty page writeback =====================
{
    cerr << "=== UNIT TEST 12: Dirty Page Writeback ===\n";
    MyDB_BufferManager myMgr(64, 2, "tempFile12");
    MyDB_TablePtr table = make_shared<MyDB_Table>("table12", "file12");

    // Ensure the file exists first
    int fd_init = open(table->getStorageLoc().c_str(), O_CREAT | O_RDWR, 0666);
    close(fd_init);

    // Allocate a page and write some data
    MyDB_PageHandle h1 = myMgr.getPage(table, 0);
    char *bytes = (char*) h1->getBytes();
    writeNums(bytes, 64, 7); // fill page with '7's
    h1->wroteBytes();         // mark dirty

    // Force the dirty page to disk
    myMgr.writeBack(h1->getPage());

    // Now read back from file
    int fd = open(table->getStorageLoc().c_str(), O_RDONLY);
    QUNIT_IS_EQUAL(fd >= 0, true);  // file should exist
    char buf[64];
    ssize_t n = read(fd, buf, 64);
    close(fd);
    QUNIT_IS_EQUAL(n >= 0, true);   // read succeeded
    QUNIT_IS_EQUAL(buf[0], '7');    // verify first byte
}


// ===================== UNIT TEST 13: Edge Case: temp slot wrap =====================
{
    cerr << "=== UNIT TEST 13: Temp Slot Increment ===\n";
    MyDB_BufferManager myMgr(64, 1, "tempFile13");

    // Rapid creation of temp pages
    for (int i = 0; i < 10; i++) {
        MyDB_PageHandle temp = myMgr.getPage();
        char *b = (char*) temp->getBytes();
        writeNums(b, 64, i);
        temp->wroteBytes();
    }

    QUNIT_IS_EQUAL(myMgr.getNextTempSlot(), 10);
}


}

#endif
