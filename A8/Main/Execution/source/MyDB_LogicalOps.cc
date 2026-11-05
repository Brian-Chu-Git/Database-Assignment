#ifndef LOG_OP_CC
#define LOG_OP_CC

#include "MyDB_LogicalOps.h"
#include "Aggregate.h"
#include "BPlusSelection.h"
#include "RegularSelection.h"
#include "ScanJoin.h"
#include "SortMergeJoin.h"

MyDB_TableReaderWriterPtr LogicalTableScan :: execute (
    map <string, MyDB_TableReaderWriterPtr> &allTableReaderWriters,
    map <string, MyDB_BPlusTreeReaderWriterPtr> &allBPlusReaderWriters) {

    MyDB_BufferManagerPtr bufferMgr = allTableReaderWriters.begin()->second->getBufferMgr();

    MyDB_TableReaderWriterPtr inputRW = make_shared<MyDB_TableReaderWriter>(inputSpec, bufferMgr);

    MyDB_TableReaderWriterPtr outputRW = make_shared<MyDB_TableReaderWriter>(outputSpec, bufferMgr);

    string predicate = "bool[true]";
    for (auto &p : selectionPred) {
        predicate = "&& (" + predicate + ", " + p->toString() + ")";
    }

    vector<string> projections;
    for (auto &att : outputSpec->getSchema()->getAtts()) {
        projections.push_back("[" + att.first + "]");
    }

    RegularSelection selectionOp(
        inputRW,
        outputRW,
        predicate,
        projections
    );

    selectionOp.run();

    return outputRW;
}

MyDB_TableReaderWriterPtr LogicalJoin::execute(
    map<string, MyDB_TableReaderWriterPtr> &allTableReaderWriters,
    map<string, MyDB_BPlusTreeReaderWriterPtr> &allBPlusReaderWriters) 
{

    // Execute left and right child operations
    MyDB_TableReaderWriterPtr leftOut = leftInputOp->execute(allTableReaderWriters, allBPlusReaderWriters);
    MyDB_TableReaderWriterPtr rightOut = rightInputOp->execute(allTableReaderWriters, allBPlusReaderWriters);

    // Create output table
    MyDB_TableReaderWriterPtr outputRW = make_shared<MyDB_TableReaderWriter>(outputSpec, leftOut->getBufferMgr());

    // Build projections
    vector<string> projections;
    for (auto &att : outputSpec->getSchema()->getAtts()) {
        projections.push_back("[" + att.first + "]");
    }

    // Combine remaining predicates into finalPred
    string finalPred = "bool[true]";
    for (auto &p : outputSelectionPredicate) {
        finalPred = "&& (" + finalPred + ", " + p->toString() + ")";
    }

    // Get schemas
    auto leftSchema = leftOut->getTable()->getSchema()->getAtts();
    auto rightSchema = rightOut->getTable()->getSchema()->getAtts();

    auto isInSchema = [](const std::string &att, const std::vector<std::pair<std::string, MyDB_AttTypePtr>> &schema) -> bool {
        for (auto &s : schema) if (s.first == att) return true;
        return false;
    };

	auto normalize = [](string s) {
		if (!s.empty() && s.front() == '[' && s.back() == ']') {
			return s.substr(1, s.size() - 2);
		}
		return s;
	};


    vector<pair<string, string>> equalityChecks;

    for (auto &p : outputSelectionPredicate) {
        if (p->isEq()) {
            string lhsStrOrig = p->getLHS()->toString();
            string rhsStrOrig = p->getRHS()->toString();

            string lhsStr = normalize(lhsStrOrig);
            string rhsStr = normalize(rhsStrOrig);

            bool lhsInLeft  = isInSchema(lhsStr, leftSchema);
            bool lhsInRight = isInSchema(lhsStr, rightSchema);
            bool rhsInLeft  = isInSchema(rhsStr, leftSchema);
            bool rhsInRight = isInSchema(rhsStr, rightSchema);

            if (lhsInLeft && rhsInRight) {
                equalityChecks.push_back({lhsStrOrig, rhsStrOrig});
            } else if (lhsInRight && rhsInLeft) {
                equalityChecks.push_back({rhsStrOrig, lhsStrOrig});
            } else {
                cerr << "Error: cannot determine which table each side of join equality belongs to!\n";
                return nullptr;
            }
        }
    }

	size_t minPages = min(leftOut->getNumPages(), rightOut->getNumPages());
	size_t bufPages = leftOut->getBufferMgr()->getNumPages();

	bool useSortMerge = (!equalityChecks.empty()) && (minPages > bufPages / 2);

	if (!useSortMerge) {
		ScanJoin join(leftOut, rightOut, outputRW, finalPred, projections, equalityChecks, "bool[true]", "bool[true]");
    	join.run();
	}
	else {
		pair<string,string> mergeEquality = equalityChecks[0];
		SortMergeJoin join(leftOut, rightOut, outputRW, finalPred, projections, mergeEquality, "bool[true]", "bool[true]");
		join.run();
	}
    
    leftOut->getBufferMgr()->killTable(leftOut->getTable());
    rightOut->getBufferMgr()->killTable(rightOut->getTable());

    return outputRW;
}

#endif
