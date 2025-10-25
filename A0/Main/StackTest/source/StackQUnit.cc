
#ifndef STACK_TEST_CC
#define STACK_TEST_CC

#include "QUnit.h"
#include "Stack.h"
#include <memory>
#include <iostream>

int main () {

	QUnit::UnitTest qunit(std :: cerr, QUnit :: verbose);
	
	// UNIT TEST 1
	// just make sure we can create an empty stack, and that it is empty
	{
		
		Stack <int> myStack;
		QUNIT_IS_EQUAL (myStack.isEmpty (), true);

	}

	// UNIT TEST 2
	// make sure we can push/pop a bunch of items
	{

		Stack <int> myStack;
		for (int i = 0; i < 10; i++) {
			myStack.push (i);
		}

		// now, pop everything off
		for (int i = 0; i < 10; i++) {
			QUNIT_IS_EQUAL (myStack.pop (), 9 - i);
		}
	}

	// UNIT TEST 3
	// make sure that the destructor works correctly... unless the stack correctly deallocates
	// all of the items it contains, this test will fail
	{
		static int temp = 0;
		struct Tester {
			Tester () {temp++;}
			~Tester () {temp--;}
		};

		{
			Stack <std :: shared_ptr <Tester>> myStack;
			std :: shared_ptr <Tester> myGuy = std :: make_shared <Tester> ();
			for (int i = 0; i < 10; i++) {
				myStack.push (myGuy);
			}
		}

		QUNIT_IS_EQUAL (temp, 0);
	}

	// UNIT TEST 4
	// make sure we can push/pop a bunch of items and then do it again
	{

		Stack <int> myStack;
		for (int i = 0; i < 10; i++) {
			myStack.push (i);
		}

		for (int i = 0; i < 5; i++) {
			QUNIT_IS_EQUAL (myStack.pop (), 9 - i);
		}

		for (int i = 5; i < 20; i++) {
			myStack.push (i);
		}

		for (int i = 0; i < 20; i++) {
			QUNIT_IS_EQUAL (myStack.pop (), 19 - i);
		}
		
		QUNIT_IS_EQUAL (myStack.isEmpty (), true);
	}

	{
        Stack<char> charStack;
        std::string letters = "abcdefghijklmnopqrstuvwxyz";

        for (char c : letters) {
            charStack.push(c);
        }

        for (int i = (int)letters.size() - 1; i >= 0; --i) {
            QUNIT_IS_EQUAL(charStack.pop(), letters[i]);
        }

        QUNIT_IS_EQUAL(charStack.isEmpty(), true);
    }

    // UNIT TEST 6
    // Test with std::string objects
    {
        Stack<std::string> stringStack;
        stringStack.push("hello");
        stringStack.push("world");
        stringStack.push("stack");
        stringStack.push("test");

        QUNIT_IS_EQUAL(stringStack.pop(), "test");
        QUNIT_IS_EQUAL(stringStack.pop(), "stack");
        QUNIT_IS_EQUAL(stringStack.pop(), "world");
        QUNIT_IS_EQUAL(stringStack.pop(), "hello");
        QUNIT_IS_EQUAL(stringStack.isEmpty(), true);
    }

    // UNIT TEST 8
    // Test with std::pair<int, std::string>
    {
        Stack<std::pair<int, std::string>> pairStack;
        pairStack.push(std::make_pair(1, "one"));
        pairStack.push(std::make_pair(2, "two"));
        pairStack.push(std::make_pair(3, "three"));

        auto p = pairStack.pop();
        QUNIT_IS_EQUAL(p.first, 3);
        QUNIT_IS_EQUAL(p.second, "three");

        p = pairStack.pop();
        QUNIT_IS_EQUAL(p.first, 2);
        QUNIT_IS_EQUAL(p.second, "two");

        p = pairStack.pop();
        QUNIT_IS_EQUAL(p.first, 1);
        QUNIT_IS_EQUAL(p.second, "one");

        QUNIT_IS_EQUAL(pairStack.isEmpty(), true);
    }

    // UNIT TEST 9
    // Stress test pushing and popping 10,000 integers
    {
        Stack<int> bigStack;
        const int N = 10000;

        for (int i = 0; i < N; i++) {
            bigStack.push(i);
        }

        bool allGood = true;
        for (int i = N - 1; i >= 0; i--) {
            if (bigStack.pop() != i) {
                allGood = false;
                break;
            }
        }
        QUNIT_IS_TRUE(allGood);
        QUNIT_IS_EQUAL(bigStack.isEmpty(), true);
    }

    // UNIT TEST 10
    // Push pointers and check lifetime management with raw pointers (be careful!)
    {
        struct Test {
            int val;
            Test(int v) : val(v) {}
            bool operator==(const Test& other) const { return val == other.val; }
        };

        Stack<Test*> ptrStack;
        for (int i = 0; i < 10; i++) {
            ptrStack.push(new Test(i));
        }

        for (int i = 9; i >= 0; i--) {
            Test* t = ptrStack.pop();
            QUNIT_IS_EQUAL(t->val, i);
            delete t; // clean up
        }

        QUNIT_IS_EQUAL(ptrStack.isEmpty(), true);
    }

    // UNIT TEST 11
    // Test that the Stack destructor properly deletes all elements
    {
        static int constructed = 0;

        struct Tracker {
            Tracker() { constructed++; }
            ~Tracker() { constructed--; }
        };

        {
            Stack<std::shared_ptr<Tracker>> trackedStack;
            for (int i = 0; i < 100; i++) {
                trackedStack.push(std::make_shared<Tracker>());
            }
            QUNIT_IS_EQUAL(constructed, 100);
            // Stack will go out of scope here
        }

        QUNIT_IS_EQUAL(constructed, 0);  // If destructor worked, this should be true
    }

    // UNIT TEST 12
    // ComplexObject with internal dynamic resource cleanup
    {
        static int objectCount = 0;
        static int resourceCount = 0;

        struct ComplexObject {
            int* resource;

            ComplexObject(int val) {
                resource = new int(val);
                objectCount++;
                resourceCount++;
            }

            ~ComplexObject() {
                delete resource;
                objectCount--;
                resourceCount--;
            }

            int value() const {
                return *resource;
            }

            bool operator==(const ComplexObject& other) const {
                return *resource == *(other.resource);
            }
        };

        {
            Stack<std::shared_ptr<ComplexObject>> complexStack;
            for (int i = 0; i < 50; i++) {
                complexStack.push(std::make_shared<ComplexObject>(i));
            }

            // While stack is alive, we should have 50 objects and 50 resources
            QUNIT_IS_EQUAL(objectCount, 50);
            QUNIT_IS_EQUAL(resourceCount, 50);

            // Optionally pop and check values
            for (int i = 49; i >= 0; i--) {
                auto obj = complexStack.pop();
                QUNIT_IS_EQUAL(obj->value(), i);
            }

            QUNIT_IS_EQUAL(complexStack.isEmpty(), true);

            // Now 0 objects but resources still held by popped elements (unless shared_ptrs are gone)
        }

        // Stack is destroyed; all shared_ptrs out of scope
        QUNIT_IS_EQUAL(objectCount, 0);
        QUNIT_IS_EQUAL(resourceCount, 0);
    }

    // UNIT TEST 13
    // Complex object with multiple fields: int, string, and a dynamic pointer
    {
        static int objectCount = 0;
        static int dataCount = 0;

        struct ComplexObject {
            int id;
            std::string name;
            int* data;

            ComplexObject(int _id, const std::string& _name, int val)
                : id(_id), name(_name), data(new int(val)) {
                objectCount++;
                dataCount++;
            }

            ~ComplexObject() {
                delete data;
                objectCount--;
                dataCount--;
            }

            // Compare by ID, name, and data value
            bool operator==(const ComplexObject& other) const {
                return id == other.id && name == other.name && *data == *(other.data);
            }
        };

        {
            Stack<std::shared_ptr<ComplexObject>> objStack;

            for (int i = 0; i < 10; i++) {
                objStack.push(std::make_shared<ComplexObject>(i, "Obj" + std::to_string(i), i * 10));
            }

            QUNIT_IS_EQUAL(objectCount, 10);
            QUNIT_IS_EQUAL(dataCount, 10);

            for (int i = 9; i >= 0; i--) {
                auto obj = objStack.pop();
                QUNIT_IS_EQUAL(obj->id, i);
                QUNIT_IS_EQUAL(obj->name, "Obj" + std::to_string(i));
                QUNIT_IS_EQUAL(*(obj->data), i * 10);
            }

            QUNIT_IS_EQUAL(objStack.isEmpty(), true);
        }

        // After the stack goes out of scope, everything should be cleaned up
        QUNIT_IS_EQUAL(objectCount, 0);
        QUNIT_IS_EQUAL(dataCount, 0);
    }
}

#endif

