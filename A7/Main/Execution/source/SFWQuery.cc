
#ifndef SFW_QUERY_CC
#define SFW_QUERY_CC

#include "ParserTypes.h"

// helper to generate deterministic temporary table names
static pair<string,string> makeNextTempName () {
	static int tempCounter = 0;
	string n = string ("tempTable") + to_string (tempCounter);
	string l = string ("tempTableLoc") + to_string (tempCounter);
	tempCounter++;
	return make_pair (n, l);
}
	
// builds and optimizes a logical query plan for a SFW query, returning the logical query plan
pair <LogicalOpPtr, double> SFWQuery :: optimizeQueryPlan (map <string, MyDB_TablePtr> &allTables) {

	MyDB_SchemaPtr totSchema = make_shared <MyDB_Schema> ();
	
	for (auto &table : tablesToProcess) {
		auto tablePtr = allTables[table.first];
		auto aliasedTable = tablePtr->alias (table.second);
		for (auto &att : aliasedTable->getSchema ()->getAtts ()) {
			totSchema->appendAtt (att);
		}
	}
	
	return optimizeQueryPlan (allTables, totSchema, allDisjunctions);
}

// builds and optimizes a logical query plan for a SFW query, returning the logical query plan
pair <LogicalOpPtr, double> SFWQuery :: optimizeQueryPlan (map <string, MyDB_TablePtr> &allTables, 
	MyDB_SchemaPtr totSchema, vector <ExprTreePtr> &allDisjunctions) {

	LogicalOpPtr bestExp = nullptr;
	MyDB_StatsPtr bestStats = nullptr;
	double bestCost = 9e99;

	// case where no joins (single table)
	if (tablesToProcess.size () == 1) {

		string tableName = tablesToProcess[0].first;
		string tableAlias = tablesToProcess[0].second;
		MyDB_TablePtr baseTable = allTables[tableName];
		
		MyDB_TablePtr aliasedTable = baseTable->alias (tableAlias);
		
		vector <ExprTreePtr> tablePredicates;
		for (auto &pred : allDisjunctions) {
			if (pred->referencesTable (tableAlias)) {
				tablePredicates.push_back (pred);
			}
		}
		
		// for single-table, include only attributes referenced in the SELECT clause
		MyDB_SchemaPtr outputSchema = make_shared <MyDB_Schema> ();
		
		for (auto &att : aliasedTable->getSchema ()->getAtts ()) {
			bool needed = false;
			string attName = att.first.substr (tableAlias.length () + 1);
			
			for (auto &sel : valuesToSelect) {
				if (sel->referencesAtt (tableAlias, attName)) {
					needed = true;
					break;
				}
			}
			
			if (needed) {
				outputSchema->appendAtt (att);
			}
		}
		
		auto tmp = makeNextTempName ();
		MyDB_TablePtr outputTable = make_shared <MyDB_Table> (tmp.first, tmp.second, outputSchema);
		
		bestStats = make_shared <MyDB_Stats> (aliasedTable);
		bestStats = bestStats->costSelection (tablePredicates);
		
		bestExp = make_shared <LogicalTableScan> (aliasedTable, outputTable, bestStats, tablePredicates);
		bestCost = bestStats->getTupleCount ();
		
		return make_pair (bestExp, bestCost);
	}

	// more than one table; try all possible splits
	int numTables = tablesToProcess.size ();
	for (int mask = 1; mask < (1 << numTables) - 1; mask++) {
		
		vector <pair <string, string>> leftTables;
		vector <pair <string, string>> rightTables;
		
		for (int i = 0; i < numTables; i++) {
			if (mask & (1 << i)) {
				leftTables.push_back (tablesToProcess[i]);
			} else {
				rightTables.push_back (tablesToProcess[i]);
			}
		}
		
		vector <ExprTreePtr> leftCNF;
		vector <ExprTreePtr> rightCNF;
		vector <ExprTreePtr> topCNF;
		
		for (auto &pred : allDisjunctions) {
			bool refsLeft = false, refsRight = false;
			
			for (auto &t : leftTables) {
				if (pred->referencesTable (t.second)) {
					refsLeft = true;
					break;
				}
			}
			
			for (auto &t : rightTables) {
				if (pred->referencesTable (t.second)) {
					refsRight = true;
					break;
				}
			}
			
			if (refsLeft && !refsRight) {
				leftCNF.push_back (pred);
			} else if (refsRight && !refsLeft) {
				rightCNF.push_back (pred);
			} else {
				topCNF.push_back (pred);
			}
		}
		
		vector<ExprTreePtr> leftVals;
		vector<ExprTreePtr> rightVals;
		
		for (auto &sel : valuesToSelect) {
			for (auto &t : leftTables) {
				if (sel->referencesTable (t.second)) { leftVals.push_back (sel); break; }
			}
			for (auto &t : rightTables) {
				if (sel->referencesTable (t.second)) { rightVals.push_back (sel); break; }
			}
		}
		
		for (auto &pred : topCNF) {
			for (auto &t : leftTables) {
				auto aliasedTable = allTables[t.first]->alias (t.second);
				for (auto &att : aliasedTable->getSchema ()->getAtts ()) {
					string attName = att.first.substr (t.second.length () + 1);
					if (pred->referencesAtt (t.second, attName)) {
						leftVals.push_back (make_shared<Identifier> (const_cast<char*>(t.second.c_str()), const_cast<char*>(attName.c_str())));
					}
				}
			}
			for (auto &t : rightTables) {
				auto aliasedTable = allTables[t.first]->alias (t.second);
				for (auto &att : aliasedTable->getSchema ()->getAtts ()) {
					string attName = att.first.substr (t.second.length () + 1);
					if (pred->referencesAtt (t.second, attName)) {
						rightVals.push_back (make_shared<Identifier> (const_cast<char*>(t.second.c_str()), const_cast<char*>(attName.c_str())));
					}
				}
			}
		}
		
		SFWQuery leftQuery;
		leftQuery.tablesToProcess = leftTables;
		leftQuery.allDisjunctions = leftCNF;
		leftQuery.valuesToSelect = leftVals;
		
		SFWQuery rightQuery;
		rightQuery.tablesToProcess = rightTables;
		rightQuery.allDisjunctions = rightCNF;
		rightQuery.valuesToSelect = rightVals;
		
		MyDB_SchemaPtr leftSchema = make_shared <MyDB_Schema> ();
		for (auto &t : leftTables) {
			auto aliasedTable = allTables[t.first]->alias (t.second);
			for (auto &att : aliasedTable->getSchema ()->getAtts ()) {
				leftSchema->appendAtt (att);
			}
		}
		
		MyDB_SchemaPtr rightSchema = make_shared <MyDB_Schema> ();
		for (auto &t : rightTables) {
			auto aliasedTable = allTables[t.first]->alias (t.second);
			for (auto &att : aliasedTable->getSchema ()->getAtts ()) {
				rightSchema->appendAtt (att);
			}
		}
		
		auto [leftExp, leftCost] = leftQuery.optimizeQueryPlan (allTables, leftSchema, leftCNF);
		auto [rightExp, rightCost] = rightQuery.optimizeQueryPlan (allTables, rightSchema, rightCNF);
		
		MyDB_StatsPtr leftStats = leftExp->getStats ();
		MyDB_StatsPtr rightStats = rightExp->getStats ();
		
		MyDB_SchemaPtr joinSchema = make_shared <MyDB_Schema> ();
		for (auto &att : leftSchema->getAtts ()) {
			joinSchema->appendAtt (att);
		}
		for (auto &att : rightSchema->getAtts ()) {
			joinSchema->appendAtt (att);
		}
		
		MyDB_SchemaPtr outputSchema = make_shared <MyDB_Schema> ();
		for (auto &att : joinSchema->getAtts ()) {
			bool needed = false;
			
			size_t underscore_pos = att.first.find ('_');
			if (underscore_pos != string::npos) {
				string tableAlias = att.first.substr (0, underscore_pos);
				string attName = att.first.substr (underscore_pos + 1);
				
				for (auto &sel : valuesToSelect) {
					if (sel->referencesAtt (tableAlias, attName)) {
						needed = true;
						break;
					}
				}
			}
			
			if (needed) {
				outputSchema->appendAtt (att);
			}
		}
		
		auto jt = makeNextTempName ();
		MyDB_TablePtr joinTable = make_shared <MyDB_Table> (jt.first, jt.second, outputSchema);
		
		MyDB_StatsPtr joinStats = leftStats->costJoin (topCNF, rightStats);
		
		double totalCost = leftCost + rightCost + joinStats->getTupleCount ();
		
		if (totalCost < bestCost) {
			bestCost = totalCost;
			bestExp = make_shared <LogicalJoin> (leftExp, rightExp, joinTable, topCNF, joinStats);
			bestStats = joinStats;
		}
	}
	
	return make_pair (bestExp, bestCost);
}

void SFWQuery :: print () {
	cout << "Selecting the following:\n";
	for (auto a : valuesToSelect) {
		cout << "\t" << a->toString () << "\n";
	}
	cout << "From the following:\n";
	for (auto a : tablesToProcess) {
		cout << "\t" << a.first << " AS " << a.second << "\n";
	}
	cout << "Where the following are true:\n";
	for (auto a : allDisjunctions) {
		cout << "\t" << a->toString () << "\n";
	}
	cout << "Group using:\n";
	for (auto a : groupingClauses) {
		cout << "\t" << a->toString () << "\n";
	}
}


SFWQuery :: SFWQuery (struct ValueList *selectClause, struct FromList *fromClause,
        struct CNF *cnf, struct ValueList *grouping) {
        valuesToSelect = selectClause->valuesToCompute;
        tablesToProcess = fromClause->aliases;
        allDisjunctions = cnf->disjunctions;
        groupingClauses = grouping->valuesToCompute;
}

SFWQuery :: SFWQuery (struct ValueList *selectClause, struct FromList *fromClause,
        struct CNF *cnf) {
        valuesToSelect = selectClause->valuesToCompute;
        tablesToProcess = fromClause->aliases;
	allDisjunctions = cnf->disjunctions;
}

SFWQuery :: SFWQuery (struct ValueList *selectClause, struct FromList *fromClause) {
        valuesToSelect = selectClause->valuesToCompute;
        tablesToProcess = fromClause->aliases;
        allDisjunctions.push_back (make_shared <BoolLiteral> (true));
}

#endif
