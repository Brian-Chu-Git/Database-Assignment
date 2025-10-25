
#ifndef BPLUS_C
#define BPLUS_C

#include "MyDB_BPlusTreeReaderWriter.h"
#include "MyDB_INRecord.h"
#include "MyDB_PageListIteratorSelfSortingAlt.h"
#include "MyDB_PageReaderWriter.h"
#include <algorithm>

MyDB_BPlusTreeReaderWriter ::MyDB_BPlusTreeReaderWriter(
    string orderOnAttName, MyDB_TablePtr forMe, MyDB_BufferManagerPtr myBuffer)
    : MyDB_TableReaderWriter(forMe, myBuffer) {

    // find the ordering attribute
    auto res = forMe->getSchema()->getAttByName(orderOnAttName);

    // remember information about the ordering attribute
    orderingAttType = res.second;
    whichAttIsOrdering = res.first;

    // and the root location
    rootLocation = getTable()->getRootLocation();
}

MyDB_RecordIteratorAltPtr MyDB_BPlusTreeReaderWriter ::getRangeItrAltHelper(
    MyDB_AttValPtr low, MyDB_AttValPtr high, bool sorted) {
    vector<MyDB_PageReaderWriter> pagesInRange;
    discoverPages(rootLocation, pagesInRange, low, high);

    auto lhs = getEmptyRecord();
    auto rhs = getEmptyRecord();
    auto cmp = buildComparator(lhs, rhs);

    auto lowRec = getEmptyRecord();
    lowRec->getAtt(whichAttIsOrdering)->set(low);
    auto highRec = getEmptyRecord();
    highRec->getAtt(whichAttIsOrdering)->set(high);
    MyDB_RecordPtr curRec = getEmptyRecord();
    auto lowCmp = buildComparator(curRec, lowRec);
    auto highCmp = buildComparator(highRec, curRec);

    return make_shared<MyDB_PageListIteratorSelfSortingAlt>(
        pagesInRange, lhs, rhs, cmp, curRec, lowCmp, highCmp, sorted);
}

MyDB_RecordIteratorAltPtr
MyDB_BPlusTreeReaderWriter ::getSortedRangeIteratorAlt(MyDB_AttValPtr low,
                                                       MyDB_AttValPtr high) {
    return getRangeItrAltHelper(low, high, true);
}

MyDB_RecordIteratorAltPtr
MyDB_BPlusTreeReaderWriter ::getRangeIteratorAlt(MyDB_AttValPtr low,
                                                 MyDB_AttValPtr high) {
    return getRangeItrAltHelper(low, high, false);
}

bool MyDB_BPlusTreeReaderWriter ::discoverPages(
    int whichPage, vector<MyDB_PageReaderWriter> &list, MyDB_AttValPtr low,
    MyDB_AttValPtr high) {
    auto pageRW = this->operator[](whichPage);

    // base case: got to a leaf node
    if (pageRW.getType() == MyDB_PageType::RegularPage) {
        list.push_back(pageRW);
        return true;
    }

    // otherwise, iterate through the internal node's INRecords to find leaf
    // nodes that might have keys in the range

    auto highInRec = getINRecord();
    highInRec->setKey(high);
    auto lowInRec = getINRecord();
    lowInRec->setKey(low);

    auto inRec = getINRecord();
    auto iter = pageRW.getIterator(inRec);
    while (iter->hasNext()) {
        iter->getNext();

        bool reachedLeaf = false;

        auto curGtHigh = buildComparator(highInRec, inRec);
        auto curLtLow = buildComparator(inRec, lowInRec);

        // if current key < low, all recs down this ptr are too small
        if (curLtLow())
            continue;

        // if we reached a leaf, we are at deepest level of internal nodes,
        // meaning for subsequent inRecs at this level, we don't have to go down
        // so we can save I/Os (checking page type needs getBytes())
        if (reachedLeaf) {
            list.push_back(this->operator[](inRec->getPtr()));
        } else {
            reachedLeaf = discoverPages(inRec->getPtr(), list, low, high);
        }

        // we want to only keep the first key > high
        if (curGtHigh())
            break;
    }

    return false;
}

void MyDB_BPlusTreeReaderWriter::appendInf(MyDB_PageReaderWriter &inPage,
                                           int ptr) {
    MyDB_INRecordPtr infInRec = getINRecord();
    infInRec->setPtr(ptr);
    inPage.append(infInRec);
}

void MyDB_BPlusTreeReaderWriter::setRootLocation(int loc) {
    rootLocation = loc;
    forMe->setRootLocation(loc);
}

void MyDB_BPlusTreeReaderWriter ::append(MyDB_RecordPtr appendMe) {
    // if the tree is empty (doesn't have at least 2 pages: a root and a leaf),
    // initialize it;
    // we use DirectoryPage for internal nodes and RegularPage for leaf nodes
    if (getNumPages() < 2) {
        setRootLocation(0);
        auto root = this->operator[](0);
        root.clear();
        root.setType(MyDB_PageType::DirectoryPage);
        appendInf(root, 1);

        auto leaf = this->operator[](1);
        leaf.clear();
        leaf.setType(MyDB_PageType::RegularPage);
        leaf.append(appendMe);

        forMe->setLastPage(1);
        return;
    }

    // otherwise, insert the rec into the tree using a recursive helper
    auto kickedUpInRec = append(forMe->getRootLocation(), appendMe);

    // if there was no split on root, we're done
    if (kickedUpInRec == nullptr)
        return;

    // otherwise, we have to create a new root that points to the old root and
    // the splitted root
    int oldRootLoc = forMe->getRootLocation();
    forMe->setLastPage(forMe->lastPage() + 1);
    setRootLocation(forMe->lastPage());
    auto newRoot = this->operator[](forMe->lastPage());
    newRoot.clear();
    newRoot.setType(MyDB_PageType::DirectoryPage);
    newRoot.append(kickedUpInRec);
    appendInf(newRoot, oldRootLoc);
}

MyDB_RecordPtr MyDB_BPlusTreeReaderWriter ::split(MyDB_PageReaderWriter splitMe,
                                                  MyDB_RecordPtr andMe) {
    // andMe could be either a INRecord or a data record
    auto getRecord = [this, &splitMe]() {
        return (splitMe.getType() == MyDB_PageType::RegularPage)
                   ? getEmptyRecord()
                   : getINRecord();
    };
    auto pageType = (splitMe.getType() == MyDB_PageType::RegularPage)
                        ? MyDB_PageType::RegularPage
                        : MyDB_PageType::DirectoryPage;

    // put all recs in a vector, including andMe
    vector<MyDB_RecordPtr> allRecs;
    auto curRec = getRecord();
    auto iter = splitMe.getIteratorAlt();
    while (iter->advance()) {
        iter->getCurrent(curRec);
        auto copyRec = getRecord();
        copyRec->fromBinary(iter->getCurrentPointer());
        allRecs.push_back(copyRec);
    }
    allRecs.push_back(andMe);

    // sort all recs
    MyDB_RecordPtr lhs = getRecord();
    MyDB_RecordPtr rhs = getRecord();
    auto cmp = buildComparator(lhs, rhs);
    // we need buffers to construct lhs and rhs for comparison in sorting
    vector<char> aBuf(splitMe.getPageSize()), bBuf(splitMe.getPageSize());
    stable_sort(allRecs.begin(), allRecs.end(),
                [&](MyDB_RecordPtr a, MyDB_RecordPtr b) {
                    a->toBinary(aBuf.data());
                    b->toBinary(bBuf.data());
                    lhs->fromBinary(aBuf.data());
                    rhs->fromBinary(bBuf.data());
                    return cmp();
                });

    // put smaller half in new page; larger half stays in splitMe
    forMe->setLastPage(forMe->lastPage() + 1);
    auto newPageLoc = forMe->lastPage();
    auto newPage = this->operator[](newPageLoc);
    newPage.clear();
    newPage.setType(pageType);
    int midIndex = allRecs.size() / 2;
    for (int i = 0; i <= midIndex; i++) {
        newPage.append(allRecs[i]);
    }
    splitMe.clear();
    splitMe.setType(pageType);
    for (int i = midIndex + 1; i < allRecs.size(); i++) {
        splitMe.append(allRecs[i]);
    }

    // return the INRecord that points to the new page
    auto newInRec = getINRecord();
    newInRec->setKey(getKey(allRecs[midIndex]));
    newInRec->setPtr(newPageLoc);
    return newInRec;
}

MyDB_RecordPtr MyDB_BPlusTreeReaderWriter ::append(int whichPage,
                                                   MyDB_RecordPtr appendMe) {
    // base case: whichPage is a leaf node
    auto pageRW = this->operator[](whichPage);
    if (pageRW.getType() == MyDB_PageType::RegularPage) {
        // if there is space, append and we're done
        if (pageRW.append(appendMe))
            return nullptr;
        // otherwise, split the page and return the INRecord
        else
            return split(pageRW, appendMe);
    }

    // otherwise, we are at an internal node, so we find the child to recurse on
    auto curInRec = getINRecord();
    auto curLtAppendMe = buildComparator(curInRec, appendMe);

    auto iter = pageRW.getIterator(curInRec);
    while (iter->hasNext()) {
        iter->getNext();

        // if current key >= appendMe, we found the child to recurse on
        if (!curLtAppendMe()) {
            auto kickedUpInRec = append(curInRec->getPtr(), appendMe);

            // if there was no split, we're done
            if (kickedUpInRec == nullptr)
                return nullptr;

            // otherwise, try to append the kicked up INRecord to this page
            if (pageRW.append(kickedUpInRec)) {
                auto lhs = getINRecord();
                auto rhs = getINRecord();
                auto cmp = buildComparator(lhs, rhs);
                pageRW.sortInPlace(cmp, lhs, rhs);
                return nullptr;
            }

            // if no room, split this page and return the INRecord
            else
                return split(pageRW, kickedUpInRec);
        }
    }

    return nullptr;
}

MyDB_INRecordPtr MyDB_BPlusTreeReaderWriter ::getINRecord() {
    return make_shared<MyDB_INRecord>(orderingAttType->createAttMax());
}

void MyDB_BPlusTreeReaderWriter ::printTree() {
    // just print out each page in the table for now
    for (int i = 0; i < getNumPages(); i++) {
        std::cout << "Page\t" << i << "\t";
        auto pageRW = this->operator[](i);
        if (pageRW.getType() == MyDB_PageType::DirectoryPage) {
            if (i == rootLocation)
                std::cout << "(root, internal):" << std::endl;
            else
                std::cout << "(internal):" << std::endl;
            auto inRec = getINRecord();
            auto iter = this->operator[](i).getIterator(inRec);
            while (iter->hasNext()) {
                iter->getNext();
                std::cout << "\t" << inRec->getKey()->toString() << "->"
                          << inRec->getPtr() << std::endl;
            }
        } else {
            std::cout << "(leaf):" << std::endl;
            auto rec = getEmptyRecord();
            auto iter = this->operator[](i).getIterator(rec);
            while (iter->hasNext()) {
                iter->getNext();
                std::cout << "\t" << rec << std::endl;
            }
        }
    }
}

MyDB_AttValPtr MyDB_BPlusTreeReaderWriter ::getKey(MyDB_RecordPtr fromMe) {

    // in this case, got an IN record
    if (fromMe->getSchema() == nullptr)
        return fromMe->getAtt(0)->getCopy();

    // in this case, got a data record
    else
        return fromMe->getAtt(whichAttIsOrdering)->getCopy();
}

function<bool()>
MyDB_BPlusTreeReaderWriter ::buildComparator(MyDB_RecordPtr lhs,
                                             MyDB_RecordPtr rhs) {

    MyDB_AttValPtr lhAtt, rhAtt;

    // in this case, the LHS is an IN record
    if (lhs->getSchema() == nullptr) {
        lhAtt = lhs->getAtt(0);

        // here, it is a regular data record
    } else {
        lhAtt = lhs->getAtt(whichAttIsOrdering);
    }

    // in this case, the LHS is an IN record
    if (rhs->getSchema() == nullptr) {
        rhAtt = rhs->getAtt(0);

        // here, it is a regular data record
    } else {
        rhAtt = rhs->getAtt(whichAttIsOrdering);
    }

    // now, build the comparison lambda and return
    if (orderingAttType->promotableToInt()) {
        return [lhAtt, rhAtt] { return lhAtt->toInt() < rhAtt->toInt(); };
    } else if (orderingAttType->promotableToDouble()) {
        return [lhAtt, rhAtt] { return lhAtt->toDouble() < rhAtt->toDouble(); };
    } else if (orderingAttType->promotableToString()) {
        return [lhAtt, rhAtt] { return lhAtt->toString() < rhAtt->toString(); };
    } else {
        cout << "This is bad... cannot do anything with the >.\n";
        exit(1);
    }
}

#endif
