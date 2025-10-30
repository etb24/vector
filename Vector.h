#pragma once
#include <cstddef>
#include <utility>


template <typename T>
class VectorIterator
{
public:
    using iterator_concept  = std::contiguous_iterator_tag;
    using value_type = T;
    using difference_type = std::ptrdiff_t;
    using pointer_type = T*;
    using const_pointer_type = const T*;
    using reference_type = T&;
    using const_reference = const T&;

    VectorIterator()
        : _pointer(nullptr){}

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

    reference_type operator[](size_t index) const
    {
        return _pointer[index];
    }


    pointer_type operator->()
    {
        return _pointer;
    }

    const_pointer_type operator->() const
    {
        return _pointer;
    }

    reference_type operator*()
    {
        return *_pointer;
    }

    const_reference operator*() const
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

template<typename T> // declare allocator in header for future
class Vector
{
public:
    using value_type = T;
    using size_type = std::size_t;
    using pointer_type = T*;
    using const_pointer_type = const T*;
    using reference = T&;
    using const_reference = const T&;
    using iterator = VectorIterator<T>;
    using const_iterator = VectorIterator<const T>;

    // default constructor
    Vector()
        : _capacity(0), _size(0), _data(nullptr){}

    // move constructor
    Vector(Vector&& other) noexcept
        : _capacity(other._capacity), _size(other._size), _data(other._data)
    {
        // leave other in a valid, empty state
        other._data = nullptr;
        other._size = 0;
        other._capacity  = 0;
    }

    // deconstructor
    ~Vector()
    {
        for (size_type i = 0; i < _size; i++)
        {
            _data[i].~T();
        }
        ::operator delete(_data);
    }

    [[nodiscard]] size_type size() const
    {
        return _size;
    }

    [[nodiscard]] size_type capacity() const
    {
        return _capacity;
    }

    reference operator[](size_type index)
    {
        if (index >= _size)
        {
            //assert
        }
        return _data[index];
    }

    const_reference operator[](size_type index) const
    {
        if (index >= _size)
        {
            //assert
        }
        return _data[index];
    }

    void clear() {
        for (size_type i = 0; i < _size; ++i) {
            _data[i].~T();
        }
        _size = 0;
    }

    // eventually move modifiers
    void push_back(const T& value)
    {
        ensure_capacity();
        new (&_data[_size]) T((value));
        ++_size;
    }

    void push_back(T&& value)
    {
        ensure_capacity();
        new (&_data[_size]) T(std::move(value)); // move-construct
        ++_size;
    }

    void pop_back()
    {
        if (_size > 0)
        {
            _data[_size - 1].~T();
            --_size;
        }
    }

    // VectorIterators
    iterator begin() noexcept
    {
        return iterator(_data);
    }

    iterator end() noexcept
    {
        return iterator(_data + _size);
    }

    const_iterator begin() const noexcept
    {
        return const_iterator(_data);
    }

    const_iterator end() const noexcept
    {
        return const_iterator(_data + _size);
    }

    const_iterator cbegin() const noexcept
    {
        return const_iterator(_data);
    }

    const_iterator cend() const noexcept
    {
        return const_iterator(_data + _size);
    }

private:
    // eventually move to an Allocator
    void reallocate(const size_t new_capacity)
    {
        // allocate new mem
        auto new_data = static_cast<pointer_type>(::operator new(new_capacity * sizeof(T)));

        // move to new mem
        for (size_type i = 0; i < _size; i++)
        {
            new (&new_data[i]) T(std::move(_data[i]));
        }

        // destroy old elements and point to new
        for (size_type i = 0; i < _size; i++)
        {
            _data[i].~T();
        }

        ::operator delete(_data);
        _data = new_data;
        _capacity = new_capacity;
    }

    void ensure_capacity()
    {
        if (_size == _capacity)
        {
            const size_type new_capacity = (_capacity == 0) ? 1 : (_capacity * 2);
            reallocate(new_capacity);
        }
    }

private:
    size_t _capacity = 0;
    size_t _size = 0;
    T* _data = nullptr;

};