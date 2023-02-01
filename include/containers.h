#pragma once

#include <cstdint>
#include <vector>

template<typename T>
class Stack final {

public:
    Stack() = default;
    Stack(size_t size);

public:
    void push(T&& item);
    size_t top();
    T pop();
    const T& peek();
    bool full();
    bool empty();

private:
    std::vector<T> items;
};

template<typename T>
void Stack<T>::push(T&& item) {
    items.emplace_back(item);
}

template<typename T>
T Stack<T>::pop() {
    T item = peek();
    items.erase(items.end());
    return item;
}

template<typename T>
const T& Stack<T>::peek() {
    return items.at(top());
}

template<typename T>
bool Stack<T>::full() {
    return !empty();
}

template<typename T>
bool Stack<T>::empty() {
    return items.empty();
}

template<typename T>
size_t Stack<T>::top() {
    return items.size() - 1;
}

template<typename T>
Stack<T>::Stack(size_t size) {
    items.reserve(size);
}


template<typename T>
class Queue final {

public:
    Queue() = default;
    Queue(size_t size);

public:
    void enqueue(T&& item);
    T dequeue();
    const T& peek();
    bool full();
    bool empty();
    const T& front();
    const T& rear();

private:
    std::vector<T> items;
    size_t m_Front = -1;
    size_t m_Rear = -1;
};

template<typename T>
Queue<T>::Queue(size_t size) {
    items.reserve(size);
}

template<typename T>
void Queue<T>::enqueue(T &&item) {
    items.emplace_back(item);
    m_Rear++;
}

template<typename T>
T Queue<T>::dequeue() {
    T item = items[m_Front];
    items.erase(items.begin() + m_Front);
    m_Front++;
    return item;
}

template<typename T>
const T &Queue<T>::peek() {
    return front();
}

template<typename T>
bool Queue<T>::full() {
    return !empty();
}

template<typename T>
bool Queue<T>::empty() {
    return items.empty();
}

template<typename T>
const T &Queue<T>::front() {
    return items.at(m_Front);
}

template<typename T>
const T &Queue<T>::rear() {
    return items.at(m_Rear);
}

template<typename T>
class RingQueue final {

public:
    RingQueue() = default;
    RingQueue(size_t size);

public:
    bool full();
    bool empty();
    bool enqueue(T&& item);
    T dequeue();

private:
    void reset();

private:
    std::vector<T> items;
    size_t front = -1;
    size_t rear = -1;
};

template<typename T>
RingQueue<T>::RingQueue(size_t size) {
    items.reserve(size);
}

template<typename T>
bool RingQueue<T>::empty() {
    return front == -1;
}

template<typename T>
bool RingQueue<T>::full() {
    return (front == 0 && rear == items.size() - 1) || front = rear + 1;
}

template<typename T>
bool RingQueue<T>::enqueue(T &&item) {
    if (full())
        return false;

    if (front == -1)
        front = 0;

    rear = (rear + 1) % items.capacity();
    items[rear] = item;
}

template<typename T>
void RingQueue<T>::reset() {
    front = -1;
    rear = -1;
}

template<typename T>
T RingQueue<T>::dequeue() {
    
}
