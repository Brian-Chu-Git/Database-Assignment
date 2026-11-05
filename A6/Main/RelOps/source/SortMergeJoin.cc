
#ifndef SORTMERGE_CC
#define SORTMERGE_CC

#include "Aggregate.h"
#include "MyDB_Record.h"
#include "MyDB_PageReaderWriter.h"
#include "MyDB_TableReaderWriter.h"
#include "SortMergeJoin.h"
#include "Sorting.h"

SortMergeJoin :: SortMergeJoin (MyDB_TableReaderWriterPtr leftInputIn, MyDB_TableReaderWriterPtr rightInputIn,
		MyDB_TableReaderWriterPtr outputIn, string finalSelectionPredicateIn, 
		vector <string> projectionsIn,
		pair <string, string> equalityCheckIn, string leftSelectionPredicateIn,
		string rightSelectionPredicateIn) {

    output = outputIn;
	finalSelectionPredicate = finalSelectionPredicateIn;
	projections = projectionsIn;
	equalityCheck = equalityCheckIn;
	leftTable = leftInputIn;
	rightTable = rightInputIn;
	leftSelectionPredicate = leftSelectionPredicateIn;
	rightSelectionPredicate = rightSelectionPredicateIn;
}

void SortMergeJoin :: run () {
	int runSize = leftTable->getBufferMgr ()->numPages / 2;

	MyDB_RecordPtr leftTemp1 = leftTable->getEmptyRecord();
	MyDB_RecordPtr leftTemp2 = leftTable->getEmptyRecord();
	auto leftComp = buildRecordComparator(leftTemp1, leftTemp2, equalityCheck.first);
	auto leftIter = buildItertorOverSortedRuns(runSize, *leftTable, leftComp, leftTemp1, leftTemp2, leftSelectionPredicate);

	MyDB_RecordPtr rightTemp1 = rightTable->getEmptyRecord();
	MyDB_RecordPtr rightTemp2 = rightTable->getEmptyRecord();
	auto rightComp = buildRecordComparator(rightTemp1, rightTemp2, equalityCheck.second);
	auto rightIter = buildItertorOverSortedRuns(runSize, *rightTable, rightComp, rightTemp1, rightTemp2, rightSelectionPredicate);

	MyDB_SchemaPtr mySchemaOut = make_shared <MyDB_Schema> ();
	for (auto &p : leftTable->getTable ()->getSchema ()->getAtts ())
		mySchemaOut->appendAtt (p);
	for (auto &p : rightTable->getTable ()->getSchema ()->getAtts ())
		mySchemaOut->appendAtt (p);

	MyDB_RecordPtr combinedRec = make_shared <MyDB_Record> (mySchemaOut);
	combinedRec->buildFrom (leftTemp1, rightTemp1);

	func finalPredicate = combinedRec->compileComputation (finalSelectionPredicate);

	vector <func> finalComputations;
	for (string s : projections) {
		finalComputations.push_back (combinedRec->compileComputation (s));
	}

	MyDB_RecordPtr outputRec = output->getEmptyRecord ();

	func lt = combinedRec->compileComputation(" < (" + equalityCheck.first + ", " + equalityCheck.second + ")");
	func gt = combinedRec->compileComputation(" > (" + equalityCheck.first + ", " + equalityCheck.second + ")");
	func eq = combinedRec->compileComputation(" == (" + equalityCheck.first + ", " + equalityCheck.second + ")");

	vector<MyDB_PageReaderWriter> leftPages;
	vector<MyDB_PageReaderWriter> rightPages;
	MyDB_PageReaderWriter leftPage(true, *(leftTable->getBufferMgr()));
	MyDB_PageReaderWriter rightPage(true, *(rightTable->getBufferMgr()));

	auto leftRevComp = buildRecordComparator(leftTemp2, leftTemp1, equalityCheck.first);

	if (leftIter->advance() && rightIter->advance()) {
		while (true) {
			bool outOfRecords = false;
			leftIter->getCurrent(leftTemp1);
			rightIter->getCurrent(rightTemp1);
			
			if (lt()->toBool()) {
				if (!leftIter->advance()) outOfRecords = true;
			}
			else if (gt()->toBool()) {
				if (!rightIter->advance()) outOfRecords = true;
			}
			else if (eq()->toBool()) {
				leftPage.clear();
				rightPage.clear();
				leftPages.clear();
				rightPages.clear();

				leftPages.push_back(leftPage);
				rightPages.push_back(rightPage);

				while (true) {
					leftIter->getCurrent(leftTemp2);
					if (!leftComp() && !leftRevComp()) {
						if (!leftPage.append(leftTemp2)) {
							MyDB_PageReaderWriter newPage(true, *(leftTable->getBufferMgr()));
							leftPage = newPage;
							leftPages.push_back(leftPage);
							leftPage.append(leftTemp2);
						}
					}
					else {
						break;
					}

					if (!leftIter->advance()) {
						outOfRecords = true;
						break;
					}
				}

				while (true) {
					rightIter->getCurrent(rightTemp1);
					combinedRec->buildFrom(leftTemp1, rightTemp1);

					if (eq()->toBool()) {
						if (!rightPage.append(rightTemp1)) {
							MyDB_PageReaderWriter newPage(true, *(rightTable->getBufferMgr()));
							rightPage = newPage;
							rightPages.push_back(rightPage);
							rightPage.append(rightTemp1);
						}
					}
					else {
						break;
					}

					if (!rightIter->advance()) {
						outOfRecords = true;
						break;
					}
				}

				MyDB_RecordIteratorAltPtr leftPagesIter;

				if (leftPages.size() == 1) {
					leftPagesIter = leftPages[0].getIteratorAlt();
				}
				else {
					leftPagesIter = getIteratorAlt(leftPages);
				}

				while (leftPagesIter->advance()) {
					leftPagesIter->getCurrent(leftTemp1);

					MyDB_RecordIteratorAltPtr rightPagesIter;

					if (rightPages.size() == 1) {
						rightPagesIter = rightPages[0].getIteratorAlt();
					}
					else {
						rightPagesIter = getIteratorAlt(rightPages);
					}
					
					while (rightPagesIter->advance()) {
						rightPagesIter->getCurrent(rightTemp1);
						int i = 0;
						for (auto &f : finalComputations) {
							outputRec->getAtt (i++)->set (f());
						}

						outputRec->recordContentHasChanged();
						output->append(outputRec);
					}
				}
			}
			if (outOfRecords) break;
		}
	}
}

#endif
