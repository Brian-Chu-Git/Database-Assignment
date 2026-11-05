
#ifndef SQL_EXPRESSIONS
#define SQL_EXPRESSIONS

#include "MyDB_AttType.h"
#include <map>
#include <vector>
#include <string>
#include <memory>
#include <iostream>
#include "MyDB_Catalog.h"
#include "MyDB_Table.h"

// create a smart pointer for database tables
using namespace std;
class ExprTree;
typedef shared_ptr <ExprTree> ExprTreePtr;

// this class encapsules a parsed SQL expression (such as "this.that > 34.5 AND 4 = 5")

// class ExprTree is a pure virtual class... the various classes that implement it are below
class ExprTree {
public:
	enum ReturnType { boolType, intType, doubleType, stringType, errType };
	virtual string toString () = 0;
	virtual ~ExprTree () {}

	virtual ReturnType typeCheck(map<string, MyDB_TablePtr> &allTables, vector<pair<string, string>> &tablesToProcess,
	MyDB_CatalogPtr catalog) = 0;
	virtual bool isAggregate() = 0;
};

class BoolLiteral : public ExprTree {

private:
	bool myVal;
public:
	
	BoolLiteral (bool fromMe) {
		myVal = fromMe;
	}

	string toString () {
		if (myVal) {
			return "bool[true]";
		} else {
			return "bool[false]";
		}
	}	

	ReturnType typeCheck(map<string, MyDB_TablePtr> &allTables, vector<pair<string, string>> &tablesToProcess, MyDB_CatalogPtr catalog) {
		return boolType;
	}

	bool isAggregate() {
		return false;
	}
};

class DoubleLiteral : public ExprTree {

private:
	double myVal;
public:

	DoubleLiteral (double fromMe) {
		myVal = fromMe;
	}

	string toString () {
		return "double[" + to_string (myVal) + "]";
	}	

	ReturnType typeCheck(map<string, MyDB_TablePtr> &allTables, vector<pair<string, string>> &tablesToProcess, MyDB_CatalogPtr catalog) {
		return doubleType;
	}

	bool isAggregate() {
		return false;
	}

	~DoubleLiteral () {}
};

// this implement class ExprTree
class IntLiteral : public ExprTree {

private:
	int myVal;
public:

	IntLiteral (int fromMe) {
		myVal = fromMe;
	}

	string toString () {
		return "int[" + to_string (myVal) + "]";
	}

	ReturnType typeCheck(map<string, MyDB_TablePtr> &allTables, vector<pair<string, string>> &tablesToProcess, MyDB_CatalogPtr catalog) {
		return intType;
	}

	bool isAggregate() {
		return false;
	}

	~IntLiteral () {}
};

class StringLiteral : public ExprTree {

private:
	string myVal;
public:

	StringLiteral (char *fromMe) {
		fromMe[strlen (fromMe) - 1] = 0;
		myVal = string (fromMe + 1);
	}

	string toString () {
		return "string[" + myVal + "]";
	}

	ReturnType typeCheck(map<string, MyDB_TablePtr> &allTables, vector<pair<string, string>> &tablesToProcess, MyDB_CatalogPtr catalog) {
		return stringType;
	}

	bool isAggregate() {
		return false;
	}

	~StringLiteral () {}
};

class Identifier : public ExprTree {

private:
	string tableName;
	string attName;
public:

	Identifier (char *tableNameIn, char *attNameIn) {
		tableName = string (tableNameIn);
		attName = string (attNameIn);
	}

	string toString () {
		return "[" + tableName + "_" + attName + "]";
	}	

	ReturnType typeCheck(map<string, MyDB_TablePtr> &allTables, vector<pair<string, string>> &tablesToProcess, MyDB_CatalogPtr catalog) {
		// find the table
		string actualTableName = "";
		bool foundAlias = false;
		for (auto &a : tablesToProcess) {
			if (a.second == tableName) {
				actualTableName = a.first;
				foundAlias = true;
				break;
			}
		}
		if (!foundAlias) {
			cout << "Error: Could not find table alias " << tableName << ".\n";
			return errType;
		}

		if (allTables.find(actualTableName) == allTables.end()) {
			cout << "Error: Could not find table " << actualTableName << " in Catalog.\n";
			return errType;
		}

		string attTypeStr;
		if (!catalog->getString(actualTableName + "." + attName + ".type", attTypeStr)) {
			cout << "Error: Could not find attribute " << attName << " in table " << actualTableName << ".\n";
			return errType;
		}

		if(attTypeStr == "int") {
			return intType;
		} else if(attTypeStr == "double") {
			return doubleType;
		} else if(attTypeStr == "string") {
			return stringType;
		} else if(attTypeStr == "bool") {
			return boolType;
		} else {
			return errType;
		}
	}

	bool isAggregate() {
		return false;
	}

	~Identifier () {}
};

class MinusOp : public ExprTree {

private:

	ExprTreePtr lhs;
	ExprTreePtr rhs;
	
public:

	MinusOp (ExprTreePtr lhsIn, ExprTreePtr rhsIn) {
		lhs = lhsIn;
		rhs = rhsIn;
	}

	string toString () {
		return "- (" + lhs->toString () + ", " + rhs->toString () + ")";
	}	

	ReturnType typeCheck(map<string, MyDB_TablePtr> &allTables, vector<pair<string, string>> &tablesToProcess, MyDB_CatalogPtr catalog) {
		ReturnType leftType = lhs->typeCheck(allTables, tablesToProcess, catalog);
		ReturnType rightType = rhs->typeCheck(allTables, tablesToProcess, catalog);

		if (leftType == errType || rightType == errType) {
			return errType;
		}
		if (leftType == intType && rightType == intType) {
			return intType;
		} else if ((leftType == intType || leftType == doubleType) &&
				   (rightType == intType || rightType == doubleType)) {
			return doubleType;
		} else {
			cout << "Error: Invalid operand types for - \n";
			return errType;
		}
	}

	bool isAggregate() {
		return (lhs->isAggregate() || rhs->isAggregate());
	}

	~MinusOp () {}
};

class PlusOp : public ExprTree {

private:

	ExprTreePtr lhs;
	ExprTreePtr rhs;
	
public:

	PlusOp (ExprTreePtr lhsIn, ExprTreePtr rhsIn) {
		lhs = lhsIn;
		rhs = rhsIn;
	}

	string toString () {
		return "+ (" + lhs->toString () + ", " + rhs->toString () + ")";
	}	

	ReturnType typeCheck(map<string, MyDB_TablePtr> &allTables, vector<pair<string, string>> &tablesToProcess, MyDB_CatalogPtr catalog) {
		ReturnType leftType = lhs->typeCheck(allTables, tablesToProcess, catalog);
		ReturnType rightType = rhs->typeCheck(allTables, tablesToProcess, catalog);

		if (leftType == errType || rightType == errType) {
			return errType;
		}
		if (leftType == stringType || rightType == stringType) {
			return stringType;
		}
		if (leftType == intType && rightType == intType) {
			return intType;
		} else if ((leftType == intType || leftType == doubleType) &&
					(rightType == intType || rightType == doubleType)) {
			return doubleType;
		} else {
			cout << "Error: Invalid operand types for + \n";
			return errType;
		}
	}

	bool isAggregate() {
		return (lhs->isAggregate() || rhs->isAggregate());
	}

	~PlusOp () {}
};

class TimesOp : public ExprTree {

private:

	ExprTreePtr lhs;
	ExprTreePtr rhs;
	
public:

	TimesOp (ExprTreePtr lhsIn, ExprTreePtr rhsIn) {
		lhs = lhsIn;
		rhs = rhsIn;
	}

	string toString () {
		return "* (" + lhs->toString () + ", " + rhs->toString () + ")";
	}	

	ReturnType typeCheck(map<string, MyDB_TablePtr> &allTables, vector<pair<string, string>> &tablesToProcess, MyDB_CatalogPtr catalog) {
		ReturnType leftType = lhs->typeCheck(allTables, tablesToProcess, catalog);
		ReturnType rightType = rhs->typeCheck(allTables, tablesToProcess, catalog);

		if (leftType == errType || rightType == errType) {
			return errType;
		}
		if (leftType == intType && rightType == intType) {
			return intType;
		} else if ((leftType == intType || leftType == doubleType) &&
				   (rightType == intType || rightType == doubleType)) {
			return doubleType;
		} else {
			cout << "Error: Invalid operand types for * \n";
			return errType;
		}
	}

	bool isAggregate() {
		return (lhs->isAggregate() || rhs->isAggregate());
	}

	~TimesOp () {}
};

class DivideOp : public ExprTree {

private:

	ExprTreePtr lhs;
	ExprTreePtr rhs;
	
public:

	DivideOp (ExprTreePtr lhsIn, ExprTreePtr rhsIn) {
		lhs = lhsIn;
		rhs = rhsIn;
	}

	string toString () {
		return "/ (" + lhs->toString () + ", " + rhs->toString () + ")";
	}	

	ReturnType typeCheck(map<string, MyDB_TablePtr> &allTables, vector<pair<string, string>> &tablesToProcess, MyDB_CatalogPtr catalog) {
		ReturnType leftType = lhs->typeCheck(allTables, tablesToProcess, catalog);
		ReturnType rightType = rhs->typeCheck(allTables, tablesToProcess, catalog);

		if (leftType == errType || rightType == errType) {
			return errType;
		}
		if (leftType == intType && rightType == intType) {
			return intType;
		} else if ((leftType == intType || leftType == doubleType) &&
				   (rightType == intType || rightType == doubleType)) {
			return doubleType;
		} else {
			cout << "Error: Invalid operand types for / \n";
			return errType;
		}
	}

	bool isAggregate() {
		return (lhs->isAggregate() || rhs->isAggregate());
	}

	~DivideOp () {}
};

class GtOp : public ExprTree {

private:

	ExprTreePtr lhs;
	ExprTreePtr rhs;
	
public:

	GtOp (ExprTreePtr lhsIn, ExprTreePtr rhsIn) {
		lhs = lhsIn;
		rhs = rhsIn;
	}

	string toString () {
		return "> (" + lhs->toString () + ", " + rhs->toString () + ")";
	}	

	ReturnType typeCheck(map<string, MyDB_TablePtr> &allTables, vector<pair<string, string>> &tablesToProcess, MyDB_CatalogPtr catalog) {
		ReturnType leftType = lhs->typeCheck(allTables, tablesToProcess, catalog);
		ReturnType rightType = rhs->typeCheck(allTables, tablesToProcess, catalog);

		if (leftType == errType || rightType == errType) {
			return errType;
		}
		if (leftType == stringType && rightType == stringType) {
			return boolType;
		}
		if ((leftType == intType || leftType == doubleType) && (rightType == intType || rightType == doubleType)) {
			return boolType;
		}
		else {
			cout << "Error: Invalid operand types for > \n";
			return errType;
		}
	}

	bool isAggregate() {
		return (lhs->isAggregate() || rhs->isAggregate());
	}

	~GtOp () {}
};

class LtOp : public ExprTree {

private:

	ExprTreePtr lhs;
	ExprTreePtr rhs;
	
public:

	LtOp (ExprTreePtr lhsIn, ExprTreePtr rhsIn) {
		lhs = lhsIn;
		rhs = rhsIn;
	}

	string toString () {
		return "< (" + lhs->toString () + ", " + rhs->toString () + ")";
	}	

	ReturnType typeCheck(map<string, MyDB_TablePtr> &allTables, vector<pair<string, string>> &tablesToProcess, MyDB_CatalogPtr catalog) {
		ReturnType leftType = lhs->typeCheck(allTables, tablesToProcess, catalog);
		ReturnType rightType = rhs->typeCheck(allTables, tablesToProcess, catalog);

		if (leftType == errType || rightType == errType) {
			return errType;
		}
		if (leftType == stringType && rightType == stringType) {
			return boolType;
		}
		if ((leftType == intType || leftType == doubleType) && (rightType == intType || rightType == doubleType)) {
			return boolType;
		}
		else {
			cout << "Error: Invalid operand types for < \n";
			return errType;
		}
	}

	bool isAggregate() {
		return (lhs->isAggregate() || rhs->isAggregate());
	}

	~LtOp () {}
};

class NeqOp : public ExprTree {

private:

	ExprTreePtr lhs;
	ExprTreePtr rhs;
	
public:

	NeqOp (ExprTreePtr lhsIn, ExprTreePtr rhsIn) {
		lhs = lhsIn;
		rhs = rhsIn;
	}

	string toString () {
		return "!= (" + lhs->toString () + ", " + rhs->toString () + ")";
	}	

	ReturnType typeCheck(map<string, MyDB_TablePtr> &allTables, vector<pair<string, string>> &tablesToProcess, MyDB_CatalogPtr catalog) {
		ReturnType leftType = lhs->typeCheck(allTables, tablesToProcess, catalog);
		ReturnType rightType = rhs->typeCheck(allTables, tablesToProcess, catalog);

		if (leftType == errType || rightType == errType) {
			return errType;
		}
		if (leftType == stringType && rightType == stringType) {
			return boolType;
		}
		if ((leftType == intType || leftType == doubleType) && (rightType == intType || rightType == doubleType)) {
			return boolType;
		}
		if (leftType == boolType && rightType == boolType) {
			return boolType;
		}
		else {
			cout << "Error: Invalid operand types for != \n";
			return errType;
		}
	}

	bool isAggregate() {
		return (lhs->isAggregate() || rhs->isAggregate());
	}

	~NeqOp () {}
};

class OrOp : public ExprTree {

private:

	ExprTreePtr lhs;
	ExprTreePtr rhs;
	
public:

	OrOp (ExprTreePtr lhsIn, ExprTreePtr rhsIn) {
		lhs = lhsIn;
		rhs = rhsIn;
	}

	string toString () {
		return "|| (" + lhs->toString () + ", " + rhs->toString () + ")";
	}	

	ReturnType typeCheck(map<string, MyDB_TablePtr> &allTables, vector<pair<string, string>> &tablesToProcess, MyDB_CatalogPtr catalog) {
		ReturnType leftType = lhs->typeCheck(allTables, tablesToProcess, catalog);
		ReturnType rightType = rhs->typeCheck(allTables, tablesToProcess, catalog);

		if (leftType == errType || rightType == errType) {
			return errType;
		}
		if (leftType == boolType && rightType == boolType) {
			return boolType;
		}
		else {
			cout << "Error: Invalid operand types for || \n";
			return errType;
		}
	}

	bool isAggregate() {
		return (lhs->isAggregate() || rhs->isAggregate());
	}

	~OrOp () {}
};

class EqOp : public ExprTree {

private:

	ExprTreePtr lhs;
	ExprTreePtr rhs;
	
public:

	EqOp (ExprTreePtr lhsIn, ExprTreePtr rhsIn) {
		lhs = lhsIn;
		rhs = rhsIn;
	}

	string toString () {
		return "== (" + lhs->toString () + ", " + rhs->toString () + ")";
	}	

	ReturnType typeCheck(map<string, MyDB_TablePtr> &allTables, vector<pair<string, string>> &tablesToProcess, MyDB_CatalogPtr catalog) {
		ReturnType leftType = lhs->typeCheck(allTables, tablesToProcess, catalog);
		ReturnType rightType = rhs->typeCheck(allTables, tablesToProcess, catalog);

		if (leftType == errType || rightType == errType) {
			return errType;
		}
		if (leftType == stringType && rightType == stringType) {
			return boolType;
		}
		if ((leftType == intType || leftType == doubleType) && (rightType == intType || rightType == doubleType)) {
			return boolType;
		}
		if (leftType == boolType && rightType == boolType) {
			return boolType;
		}
		else {
			cout << "Error: Invalid operand types for == \n";
			return errType;
		}
	}

	bool isAggregate() {
		return (lhs->isAggregate() || rhs->isAggregate());
	}

	~EqOp () {}
};

class NotOp : public ExprTree {

private:

	ExprTreePtr child;
	
public:

	NotOp (ExprTreePtr childIn) {
		child = childIn;
	}

	string toString () {
		return "!(" + child->toString () + ")";
	}	

	ReturnType typeCheck(map<string, MyDB_TablePtr> &allTables, vector<pair<string, string>> &tablesToProcess, MyDB_CatalogPtr catalog) {
		ReturnType childType = child->typeCheck(allTables, tablesToProcess, catalog);

		if (childType == errType) {
			return errType;
		}
		if (childType == boolType) {
			return boolType;
		}
		else {
			cout << "Error: Invalid operand types for ! \n";
			return errType;
		}
	}

	bool isAggregate() {
		return child->isAggregate();
	}

	~NotOp () {}
};

class SumOp : public ExprTree {

private:

	ExprTreePtr child;
	
public:

	SumOp (ExprTreePtr childIn) {
		child = childIn;
	}

	string toString () {
		return "sum(" + child->toString () + ")";
	}	

	ReturnType typeCheck(map<string, MyDB_TablePtr> &allTables, vector<pair<string, string>> &tablesToProcess, MyDB_CatalogPtr catalog) {
		ReturnType childType = child->typeCheck(allTables, tablesToProcess, catalog);

		if (childType == errType) {
			return errType;
		}
		if (childType == intType || childType == doubleType) {
			return childType;
		}
		else {
			cout << "Error: Invalid operand types for sum \n";
			return errType;
		}
	}

	bool isAggregate() {
		return true;
	}

	~SumOp () {}
};

class AvgOp : public ExprTree {

private:

	ExprTreePtr child;
	
public:

	AvgOp (ExprTreePtr childIn) {
		child = childIn;
	}

	string toString () {
		return "avg(" + child->toString () + ")";
	}	

	ReturnType typeCheck(map<string, MyDB_TablePtr> &allTables, vector<pair<string, string>> &tablesToProcess, MyDB_CatalogPtr catalog) {
		ReturnType childType = child->typeCheck(allTables, tablesToProcess, catalog);

		if (childType == errType) {
			return errType;
		}
		if (childType == intType || childType == doubleType) {
			return doubleType;
		}
		else {
			cout << "Error: Invalid operand types for avg \n";
			return errType;
		}
	}

	bool isAggregate() {
		return true;
	}

	~AvgOp () {}
};

#endif
