
#ifndef REG_SELECTION_C                                        
#define REG_SELECTION_C

#include "RegularSelection.h"

RegularSelection :: RegularSelection (MyDB_TableReaderWriterPtr inputIn, MyDB_TableReaderWriterPtr outputIn,
		string selectionPredicateIn, vector <string> projectionsIn) {
            input = inputIn;
            output = outputIn;
            selectionPredicate = selectionPredicateIn;
            projections = projectionsIn;
        }

void RegularSelection :: run () {
    auto inputRec = input->getEmptyRecord();
    auto outputRec = output->getEmptyRecord();

    func predFunc = inputRec->compileComputation(selectionPredicate);

    vector<func> projFuncs;
    for (auto &p : projections) {
        projFuncs.push_back(inputRec->compileComputation(p));
    }

    auto iter = input->getIteratorAlt();

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
