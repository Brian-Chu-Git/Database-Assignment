

#ifndef BPLUS_H
#define BPLUS_H

#include "MyDB_BufferManager.h"
#include "MyDB_INRecord.h"
#include "MyDB_Record.h"
#include "MyDB_RecordIterator.h"
#include "MyDB_RecordIteratorAlt.h"
#include "MyDB_Table.h"
#include "MyDB_TableReaderWriter.h"
#include <functional>
#include <memory>

// create a smart pointer for the catalog
using namespace std;
class MyDB_PageReaderWriter;
class MyDB_BPlusTreeReaderWriter;
typedef shared_ptr<MyDB_BPlusTreeReaderWriter> MyDB_BPlusTreeReaderWriterPtr;

class MyDB_BPlusTreeReaderWriter : public MyDB_TableReaderWriter {

  public:
    /* NOTE THAT EACH OF THESE METHODS ARE REQUIRED */

    // create a BTree TableReaderWriter
    MyDB_BPlusTreeReaderWriter(string nameOfAttToOrderOn, MyDB_TablePtr forMe,
                               MyDB_BufferManagerPtr myBuffer);

    // gets an instance of an alternate iterator over the table... this is an
    // iterator that has the alternate getCurrent ()/advance () interface
    // return all records with a key value in the range [low, high], inclusive
    MyDB_RecordIteratorAltPtr getRangeIteratorAlt(MyDB_AttValPtr low,
                                                  MyDB_AttValPtr high);

    // gets an instance of an alternate iterator over the table... this is an
    // iterator that has the alternate getCurrent ()/advance () interface...
    // returned records must be sorted return all records with a key value in
    // the range [low, high], inclusive
    MyDB_RecordIteratorAltPtr getSortedRangeIteratorAlt(MyDB_AttValPtr low,
                                                        MyDB_AttValPtr high);

    // append a record to the B+-Tree
    void append(MyDB_RecordPtr appendMe);

    // print the contents of the tree to the screen
    void printTree();

  private:
    MyDB_RecordIteratorAltPtr
    getRangeItrAltHelper(MyDB_AttValPtr low, MyDB_AttValPtr high, bool sorted);

    // append an infinity INRecord to the given internal page;
    // we need an infinity InRecord in each level of internal nodes
    void appendInf(MyDB_PageReaderWriter &inPage, int ptr);

    // set the rootLocations in both this class and the table (I don't know why
    // there are two rootLocations, so just keeping them in sync)
    void setRootLocation(int loc);

    /* NOTE THAT EACH OF THESE METHODS ARE OPTIONAL.  They are a suggestion
       for a set of helper methods that you might consider including in
       order to get your stuff to work. */

    // gets a list of pages that might have data for an iterator... any leaf
    // page that can possibly have a value in the range [low, high],
    // inclusive should be returned from this call;
    // return true if the page pointed to by whichPage was at the leaf level;
    bool discoverPages(int whichPage, vector<MyDB_PageReaderWriter> &list,
                       MyDB_AttValPtr low, MyDB_AttValPtr high);

    // appends a record to the named page; if there is a split, then an
    // MyDB_INRecordPtr is returned that points to the record holding the
    // (key, ptr) pair pointing to the new page.  Note that the new page
    // always holds the lower 1/2 of the records on the page; the upper 1/2
    // remains in the original page
    MyDB_RecordPtr append(int whichPage, MyDB_RecordPtr appendMe);

    // splits the given page (plus the record andMe) around the median.  A
    // MyDB_INRecordPtr is returned that points to the record holding the
    // (key, ptr) pair pointing to the new page.  Note that the new page
    // always holds the lower 1/2 of the records on the page; the upper 1/2
    // remains in the original page
    MyDB_RecordPtr split(MyDB_PageReaderWriter splitMe, MyDB_RecordPtr andMe);

    // constructs and returns an empty internal node record for this
    // particular tree (the default key is the maximum value)
    MyDB_INRecordPtr getINRecord();

    // gets the search key from a LN record
    MyDB_AttValPtr getKey(MyDB_RecordPtr fromMe);

    // constructs an returns a comparator for the two records given... both
    // must either be IN records for this particular tree, or they must be
    // LN records for this tree, or a combination.  The resulting comparator
    // returns true if and only if the first record has a key value less
    // than the second record
    function<bool()> buildComparator(MyDB_RecordPtr lhs, MyDB_RecordPtr rhs);

    // the location (page number) of the root in the tree
    int rootLocation;

    // the type of the attribute that we are ordering on
    MyDB_AttTypePtr orderingAttType;

    // the number of the attribute that we are ordering on, in the data
    // records
    int whichAttIsOrdering;
};

#endif
