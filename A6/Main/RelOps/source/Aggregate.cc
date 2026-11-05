
#ifndef AGG_CC
#define AGG_CC

#include "MyDB_Record.h"
#include "MyDB_PageReaderWriter.h"
#include "MyDB_TableReaderWriter.h"
#include "Aggregate.h"
#include <unordered_map>

using namespace std;

Aggregate :: Aggregate (MyDB_TableReaderWriterPtr inputIn, MyDB_TableReaderWriterPtr outputIn,
		vector <pair <MyDB_AggType, string>> aggsToComputeIn,
		vector <string> groupingsIn, string selectionPredicateIn) {
            input = inputIn;
            output = outputIn;
            aggsToCompute = aggsToComputeIn;
            groupings = groupingsIn;
            selectionPredicate = selectionPredicateIn;
        }

void Aggregate :: run () {
	int numAggs = aggsToCompute.size();
	int numGroups = groupings.size();
	if (output->getTable ()->getSchema ()->getAtts ().size () != numAggs + numGroups) {
		cout << "error, the output schema needs to have the same number of atts as (# of aggs to compute + # groups).\n";
		return;
	}

	unordered_map <size_t, void *> recordMap;
	unordered_map <size_t, vector<double>> sumMap;
	unordered_map <size_t, vector<double>> avgMap;
	unordered_map <size_t, int> cntMap;

	auto inputRec = input->getEmptyRecord();

	MyDB_SchemaPtr mySchemaOut = make_shared <MyDB_Schema> ();
	for (auto &p : output->getTable ()->getSchema ()->getAtts ())
		mySchemaOut->appendAtt (p);

	MyDB_RecordPtr combinedRec = make_shared <MyDB_Record> (mySchemaOut);

	vector<func> groupFuncs;
	for (auto g : groupings) {
		groupFuncs.push_back(inputRec->compileComputation(g));
	}

	vector<pair <MyDB_AggType, func>> aggFuncs;;
	for (auto &p : aggsToCompute) {
		aggFuncs.push_back({p.first, inputRec->compileComputation(p.second)});
	}

	func pred = inputRec->compileComputation (selectionPredicate);

	vector<MyDB_PageReaderWriter> data;
	for (int i = 0; i < input->getNumPages(); i++) {
		MyDB_PageReaderWriter temp = (*input)[i];
		if (temp.getType() == MyDB_PageType::RegularPage)
			data.push_back(temp);
	}

	auto dataIter = getIteratorAlt(data);

	int pageCount = 0;
	while (dataIter->advance()) {
		dataIter->getCurrent(inputRec);

		if (!pred()->toBool()) continue;

		size_t hashVal = 0;
		for (auto &f : groupFuncs) {
			hashVal ^= f()->hash();
		}

		if (recordMap.count (hashVal) == 0) {
			cntMap[hashVal] = 1;

			int i = 0;
			for (auto g : groupFuncs) {
				combinedRec->getAtt(i++)->set(g());
			}

			i = groupFuncs.size();
			for (auto p : aggFuncs) {
				if (p.first == MyDB_AggType::cnt) {
					auto attribute = make_shared<MyDB_IntAttVal>();
					attribute->set(1);
					combinedRec->getAtt(i)->set(attribute);
				}
				if (p.first == MyDB_AggType::sum) {
					combinedRec->getAtt(i)->set(p.second());
					sumMap[hashVal].push_back(combinedRec->getAtt(i)->toDouble());
				}
				if (p.first == MyDB_AggType::avg) {
					combinedRec->getAtt(i)->set(p.second());
					avgMap[hashVal].push_back(combinedRec->getAtt(i)->toDouble());
				}
				i++;
			}

			combinedRec->recordContentHasChanged();

			MyDB_PageReaderWriter temp = output->getPinned(pageCount);
			void* loc = temp.appendAndReturnLocation(combinedRec);
			recordMap[hashVal] = loc;

			if (loc == nullptr) {
				pageCount++;
				temp = output->getPinned(pageCount);
				recordMap[hashVal] = temp.appendAndReturnLocation(combinedRec);
			}
		}
		else {
			combinedRec->fromBinary(recordMap[hashVal]);
			cntMap[hashVal]++;

			int i = groupFuncs.size();
			int sumCount = 0;
			int avgCount = 0;

			for (auto p : aggFuncs) {
				if (p.first == MyDB_AggType::cnt) {
					auto attribute = make_shared<MyDB_IntAttVal>();
					attribute->set(cntMap[hashVal]);
					combinedRec->getAtt(i)->set(attribute);
				}
				if (p.first == MyDB_AggType::sum) {
					combinedRec->getAtt(i)->set(p.second());
					sumMap[hashVal][sumCount] += combinedRec->getAtt(i)->toDouble();
					auto attribute = make_shared<MyDB_DoubleAttVal>();
					attribute->set(sumMap[hashVal][sumCount]);
					combinedRec->getAtt(i)->set(attribute);
					sumCount++;
				}
				if (p.first == MyDB_AggType::avg) {
					combinedRec->getAtt(i)->set(p.second());
					avgMap[hashVal][avgCount] = (avgMap[hashVal][avgCount]*(cntMap[hashVal]-1) + combinedRec->getAtt(i)->toDouble()) / cntMap[hashVal];
					auto attribute = make_shared<MyDB_DoubleAttVal>();
					attribute->set(avgMap[hashVal][avgCount]);
					combinedRec->getAtt(i)->set(attribute);
					avgCount++;
				}
				i++;
			}
			combinedRec->recordContentHasChanged();
			combinedRec->toBinary(recordMap[hashVal]);
		}
	}
}

#endif