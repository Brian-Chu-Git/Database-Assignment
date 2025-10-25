
#ifndef SORT_C
#define SORT_C

#include "MyDB_PageReaderWriter.h"
#include "MyDB_TableRecIterator.h"
#include "MyDB_TableRecIteratorAlt.h"
#include "MyDB_TableReaderWriter.h"
#include "Sorting.h"

using namespace std;

void mergeIntoFile (MyDB_TableReaderWriter &sortIntoMe, vector <MyDB_RecordIteratorAltPtr> &mergeUs,
        function <bool ()> comparator, MyDB_RecordPtr lhs, MyDB_RecordPtr rhs) {

	if (mergeUs.empty ()) return;

	MyDB_RecordPtr appendRec = sortIntoMe.getEmptyRecord ();

	size_t n = mergeUs.size ();
	vector<bool> valid (n, false);
	for (size_t i = 0; i < n; ++i) {
		if (mergeUs[i]) {
			bool has = mergeUs[i]->advance ();
			if (has) {
				valid[i] = true;
			}
		}
	}

	auto heapComp = [&](int a, int b) {
		// defensive checks: ensure the iterators and their current pointers are valid
		if (!mergeUs[a] && !mergeUs[b]) return false;
		if (!mergeUs[a]) return false; // treat a as "greater" so b sorts first
		if (!mergeUs[b]) return true;

		void *aptr = mergeUs[a]->getCurrentPointer ();
		void *bptr = mergeUs[b]->getCurrentPointer ();
		if (!aptr && !bptr) return false;
		if (!aptr) return false;
		if (!bptr) return true;

		// now it's safe to deserialize into lhs/rhs and call the comparator
		lhs->fromBinary (aptr);
		rhs->fromBinary (bptr);
		return comparator () == false;
	};

	vector<int> heapVec;
	for (int i = 0; i < (int) n; ++i) if (valid[i]) heapVec.push_back (i);

	std::function<bool(int,int)> funcComp = heapComp;
	std::make_heap (heapVec.begin (), heapVec.end (), funcComp);

	while (!heapVec.empty ()) {
		std::pop_heap (heapVec.begin (), heapVec.end (), funcComp);
		int idx = heapVec.back ();
		heapVec.pop_back ();

		mergeUs[idx]->getCurrent (appendRec);
		sortIntoMe.append (appendRec);

		bool more = mergeUs[idx]->advance ();
		if (more) {
			mergeUs[idx]->getCurrent (appendRec);
			heapVec.push_back (idx);
			std::push_heap (heapVec.begin (), heapVec.end (), funcComp);
		}
	}

}

vector <MyDB_PageReaderWriter> mergeIntoList (MyDB_BufferManagerPtr parent, MyDB_RecordIteratorAltPtr leftIter,
		MyDB_RecordIteratorAltPtr rightIter, function <bool ()> comparator, MyDB_RecordPtr lhs, MyDB_RecordPtr rhs) {

	vector <MyDB_PageReaderWriter> outPages;
	if (!parent) return outPages;

	MyDB_PageReaderWriter curPage (*parent);

	bool leftValid = false;	
	bool rightValid = false;

	if (leftIter) {
		leftValid = leftIter->advance ();
		if (leftValid) leftIter->getCurrent (lhs);
	}
	if (rightIter) {
		rightValid = rightIter->advance ();
		if (rightValid) rightIter->getCurrent (rhs);
	}

	bool curHasRecords = false;

	while (leftValid && rightValid) {
		bool takeLeft = comparator ();

		MyDB_RecordPtr toAppend = (takeLeft ? lhs : rhs);

		// defensive: ensure the iterator's current pointer is valid before appending
		void *checkPtr = (takeLeft ? leftIter->getCurrentPointer() : rightIter->getCurrentPointer());
		if (!checkPtr) {
			// if pointer invalid, stop consuming from that side
			if (takeLeft) { leftValid = false; continue; } else { rightValid = false; continue; }
		}

		if (!curPage.append (toAppend)) {
			if (curHasRecords) {
				outPages.push_back (curPage);
			}
			curPage = MyDB_PageReaderWriter (*parent);
			curHasRecords = false;
			if (!curPage.append (toAppend)) {
				return outPages;
			}
		}
		curHasRecords = true;

		if (takeLeft) {
			bool more = leftIter->advance ();
			if (more) {
				leftIter->getCurrent (lhs);
			} else {
				leftValid = false;
			}
		} else {
			bool more = rightIter->advance ();
			if (more) {
				rightIter->getCurrent (rhs);
			} else {
				rightValid = false;
			}
		}
	}

	while (leftValid) {
		if (!curPage.append (lhs)) {
			if (curHasRecords) outPages.push_back (curPage);
			curPage = MyDB_PageReaderWriter (*parent);
			curHasRecords = false;
			if (!curPage.append (lhs)) return outPages;
		}
		curHasRecords = true;
		bool more = leftIter->advance ();
		if (more) leftIter->getCurrent (lhs); else leftValid = false;
	}

	while (rightValid) {
		if (!curPage.append (rhs)) {
			if (curHasRecords) outPages.push_back (curPage);
			curPage = MyDB_PageReaderWriter (*parent);
			curHasRecords = false;
			if (!curPage.append (rhs)) return outPages;
		}
		curHasRecords = true;
		bool more = rightIter->advance ();
		if (more) rightIter->getCurrent (rhs); else rightValid = false;
	}

	if (curHasRecords) outPages.push_back (curPage);

	return	 outPages;
}
	
void sort (int runSize, MyDB_TableReaderWriter &sortMe, MyDB_TableReaderWriter &sortIntoMe,
		function <bool ()> comparator, MyDB_RecordPtr lhs, MyDB_RecordPtr rhs) {

	if (runSize <= 0) {
		return;
	}

	int totalPages = sortMe.getNumPages ();
	if (totalPages == 0) {
		// nothing to do
		return;
	}
	int pageIdx = 0;

	vector <MyDB_RecordIteratorAltPtr> runIters;

	while (pageIdx < totalPages) {
		vector <MyDB_PageReaderWriter> pages;

		for (int i = 0; i < runSize && pageIdx < totalPages; ++i, ++pageIdx) {
			MyDB_PageReaderWriter pg = sortMe[pageIdx];

			pg.sortInPlace (comparator, lhs, rhs);

			pages.push_back (pg);
		}

		vector <vector<MyDB_PageReaderWriter>> lists;
		lists.reserve (pages.size ());
		for (auto &pg : pages) {
			lists.emplace_back();
			lists.back().push_back (pg);
		}

		MyDB_BufferManagerPtr parent = sortMe.getBufferMgr ();

		while (lists.size () > 1) {
			vector <vector<MyDB_PageReaderWriter>> newLists;
			for (size_t i = 0; i + 1 < lists.size (); i += 2) {
				MyDB_RecordIteratorAltPtr leftIter = getIteratorAlt (lists[i]);
				MyDB_RecordIteratorAltPtr rightIter = getIteratorAlt (lists[i + 1]);
				vector <MyDB_PageReaderWriter> merged = mergeIntoList (parent, leftIter, rightIter, comparator, lhs, rhs);
				newLists.push_back (std::move (merged));
			}
			if (lists.size () % 2 == 1) {
				newLists.push_back (std::move (lists.back ()));
			}
			lists.swap (newLists);
		}

		vector <MyDB_PageReaderWriter> finalRun;
		if (!lists.empty ()) finalRun = std::move (lists[0]);

		if (!finalRun.empty ()) {
			MyDB_RecordIteratorAltPtr runIter = getIteratorAlt (finalRun);
			runIters.push_back (runIter);
		}
	}

	if (!runIters.empty ()) {
		mergeIntoFile (sortIntoMe, runIters, comparator, lhs, rhs);
	}
}


#endif
