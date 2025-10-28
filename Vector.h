#pragma once
#include <cstddef>
#include <utility>


template <typename Vector>
class VectorIterator
{
public:
    using value_type = typename Vector::value_type;
    using pointer_type = value_type*;
    using reference_type = value_type&;

    explicit VectorIterator(pointer_type pointer)
        : _pointer(pointer) {}

    VectorIterator& operator++()
    {
        ++_pointer;
        return *this;
    }

    VectorIterator operator++(int)
    {
        VectorIterator iterator(*this);
        ++(*this);
        return iterator;
    }

    VectorIterator& operator--()
    {
        --_pointer;
        return *this;
    }

    VectorIterator operator--(int)
    {
        VectorIterator iterator(*this);
        --(*this);
        return iterator;
    }

    reference_type operator[](int index)
    {
        return *(_pointer + index);
    }


    pointer_type operator->()
    {
        return _pointer;
    }

    reference_type operator*()
    {
        return *_pointer;
    }

    bool operator==(const VectorIterator& other) const
    {
        return _pointer == other._pointer;
    }

    bool operator!=(const VectorIterator& other) const
    {
        return _pointer != other._pointer;
    }

private:
    pointer_type _pointer;
};

template<class T> // declare allocator in header for future
class Vector
{
public:
    using value_type = T;
    using iterator = VectorIterator<Vector<T>>;

    // default constructor
    Vector()
        : _data(nullptr), _size(0), _capacity(0){}

    // move constructor
    Vector(Vector&& other) noexcept
        : _data(other._data), _size(other._size), _capacity(other._capacity)
    {
        // leave other in a valid, empty state
        other._data = nullptr;
        other._size = 0;
        other._capacity  = 0;
    }

    // deconstructor
    ~Vector()
    {
        for (std::size_t i = 0; i < _size; i++)
        {
            _data[i].~T();
        }
        ::operator delete(_data);
    }

    [[nodiscard]] std::size_t size() const
    {
        return _size;
    }

    [[nodiscard]] std::size_t capacity() const
    {
        return _capacity;
    }

    T& operator[](std::size_t index)
    {
        return _data[index];
    }

    const T& operator[](std::size_t index) const
    {
        return _data[index];
    }

    void clear() {
        for (std::size_t i = 0; i < _size; ++i) {
            _data[i].~T();
        }
        _size = 0;
    }

    void push_back(const T& value)
    {
        if (_size == _capacity)
        {
            reallocate(capacity() * 1.5);
        }
        new (&_data[_size]) T(std::move(value));
        ++_size;
    }

    void pop_back()
    {
        if (_size > 0)
        {
            _data[_size].~T();
            --_size;
        }
    }

    iterator begin()
    {
        return iterator(_data);
    }

    iterator end()
    {
        return iterator(_data + _size);
    }

private:
    // eventually move to an Allocator
    void reallocate(size_t new_capacity)
    {
        // allocate new mem
        T* new_data = static_cast<T*>(::operator new(new_capacity));

        // move to new mem
        for (std::size_t i = 0; i < _size; i++)
        {
            new (&new_data[i]) T(std::move(_data[i]));
        }

        // destroy old elements and point to new
        for (std::size_t i = 0; i < _capacity; i++)
        {
            _data[i].~T();
        }

        ::operator delete(_data);
        _data = new_data;
        _capacity = new_capacity;
    }

private:
    size_t _capacity = 0;
    size_t _size = 0;
    T* _data = nullptr;

};