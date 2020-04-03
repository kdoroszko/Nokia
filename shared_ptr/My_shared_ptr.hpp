#pragma once

#include <utility>
#include <cstddef>

template <typename T>
class My_unique_ptr
{
public:

    explicit My_unique_ptr(T *new_ptr = nullptr) : ptr(new_ptr)
    {
        counter = new unsigned int();
        if(ptr)
            (*counter)++;
    }

    My_unique_ptr(const My_unique_ptr &new_ptr) : ptr(new_ptr.ptr), counter(new_ptr.counter)
    {
        (*counter)++;
    }

    My_unique_ptr(My_unique_ptr &&other) : ptr(other.ptr), counter(other.counter)
    {
        other.ptr = nullptr;
        other.conuter = nullptr;
    }

    ~My_unique_ptr()
    {
        (*counter)--;

        if(*counter == 0)
        {
            delete ptr;
            delete counter;
        }
    }

    My_unique_ptr &operator= (const My_unique_ptr &new_ptr)
    {
        ptr = new_ptr.ptr;
        counter = new_ptr.counter;
        (*counter)++;

        return *this;
    }

    My_unique_ptr &operator= (My_unique_ptr &&other)
    {
        T *temp_ptr = ptr;
        ptr = std::exchange(other.ptr, nullptr);

        unsigned int *temp_counter = counter;
        counter = std::exchange(other.counter, nullptr);

        delete temp_ptr;
        delete temp_counter;
        return *this;
    }

    T& operator*() const
    {
        return *(this->get());
    }

    T* operator->() const
    {
        return this->get();
    }

    bool operator== (const My_unique_ptr &other) const
    {
        return ptr == other.ptr;
    }

    bool operator== (const std::nullptr_t) const
    {
        return !ptr;
    }

    explicit operator bool() const
    {
        return ptr ? true : false;
    }

    T* get() const
    {
        return ptr;
    }

    void reset(T *other = nullptr)
    {
        delete std::exchange(ptr, other);
    }

private:

    T *ptr;
    unsigned int *counter;
};