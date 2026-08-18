#pragma once
#include <vector>

template <typename T>
class TArray : public std::vector<T>
{
public:
    using std::vector<T>::vector;

    void Push(const T& value)
    {
        this->push_back(value);
    }

    void Pop()
    {
        this->pop_back();
    }
};