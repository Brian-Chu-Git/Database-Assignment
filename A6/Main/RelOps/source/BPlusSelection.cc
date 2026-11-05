
#ifndef BPLUS_SELECTION_C                                        
#define BPLUS_SELECTION_C

#include "BPlusSelection.h"

BPlusSelection :: BPlusSelection (MyDB_BPlusTreeReaderWriterPtr inputIn, MyDB_TableReaderWriterPtr outputIn,
		MyDB_AttValPtr lowIn, MyDB_AttValPtr highIn,
		string selectionPredicateIn, vector <string> projectionsIn) {
            input = inputIn;
            output = outputIn;
            low = lowIn;
            high = highIn;
            selectionPredicate = selectionPredicateIn;
            projections = projectionsIn;
        }

void BPlusSelection :: run () {
    auto inputRec = input->getEmptyRecord();
    auto outputRec = output->getEmptyRecord();

    auto predFunc = inputRec->compileComputation(selectionPredicate);

    auto iter = input->getRangeIteratorAlt(low, high);

    vector<func> projFuncs;
    for (auto &p : projections) {
        projFuncs.push_back(inputRec->compileComputation(p));
    }

    while (iter->advance()) {
        iter->getCurrent(inputRec);

        if (!predFunc()->toBool()) continue;

        int i = 0;
        for (auto &f : projFuncs) {
            outputRec->getAtt (i++)->set (f());
        }

        outputRec->recordContentHasChanged();
        output->append(outputRec);
    }
}

#endif
