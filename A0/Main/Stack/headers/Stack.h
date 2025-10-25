#ifndef STACK_H
#define STACK_H

#include <stdexcept>

// Node class for stack
template <class Data>
class Node {
public:
    Data holdMe;
    Node<Data>* next;

    Node(Data data, Node<Data>* nextNode) : holdMe(data), next(nextNode) {}
};

// Stack class
template <class Data> 
class Stack {
    Node<Data>* head;

public:
    Stack() : head(nullptr) {}

    ~Stack() {
        while (!isEmpty()) {
            pop();
        }
    }

    void push(Data data) { 
        Node<Data>* newNode = new Node<Data>(data, head);
        head = newNode;
    }

    bool isEmpty() const {
        return head == nullptr;
    }

    Data pop() { 
        if (isEmpty()) {
            throw std::runtime_error("Stack is empty!");
        }
        Node<Data>* temp = head;
        Data data = temp->holdMe;
        head = head->next;
        delete temp;
        return data;  
    }
};

#endif
