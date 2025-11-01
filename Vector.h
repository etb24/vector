#pragma once

#include <cstddef>
#include <iterator>
#include <stdexcept>
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
    Vector() noexcept
        : _capacity(0), _size(0), _data(nullptr) {}

    // copy constructor
    Vector(const Vector& other)
        : _capacity(other._size), _size(0), _data(nullptr)
    {
        if (_capacity > 0) {
            _data = static_cast<pointer_type>(::operator new(_capacity * sizeof(T)));
            try {
                for (; _size < other._size; ++_size) {
                    new (&_data[_size]) T(other._data[_size]);
                }
            } catch (...) {
                for (size_type i = 0; i < _size; ++i) {
                    _data[i].~T();
                }
                ::operator delete(_data);
                _data = nullptr;
                _size = 0;
                _capacity = 0;
                throw;
            }
        }
    }

    // move constructor
    Vector(Vector&& other) noexcept
        : _capacity(other._capacity), _size(other._size), _data(other._data)
    {
        // leave other in a valid, empty state
        other._data = nullptr;
        other._size = 0;
        other._capacity  = 0;
    }

    Vector& operator=(const Vector& other)
    {
        if (this != &other) {
            Vector temp(other);
            swap(temp);
        }
        return *this;
    }

    Vector& operator=(Vector&& other) noexcept
    {
        if (this != &other) {
            clear();
            ::operator delete(_data);
            _data = other._data;
            _size = other._size;
            _capacity = other._capacity;
            other._data = nullptr;
            other._size = other._capacity = 0;
        }
        return *this;
    }

    void swap(Vector& other) noexcept
    {
        std::swap(_data, other._data);
        std::swap(_size, other._size);
        std::swap(_capacity, other._capacity);
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

    [[nodiscard]] bool empty() const
    {
        return _size == 0;
    }

    pointer_type data()
    {
        return _data;
    }

    const_pointer_type data() const noexcept
    {
        return _data;
    }

    reference operator[](size_type index)
    {
        if (index >= _size) throw std::out_of_range("index out of range");
        return _data[index];
    }

    const_reference operator[](size_type index) const
    {
        if (index >= _size) throw std::out_of_range("index out of range");
        return _data[index];
    }

    reference front()
    {
        return _data[0];
    }

    const_reference front() const
    {
        return _data[0];
    }

    reference back()
    {
        return _data[_size - 1];
    }

    const_reference back() const
    {
        return _data[_size - 1];
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

    template<typename... Args>
    reference emplace_back(Args&& ... args)
    {
        ensure_capacity();
        new (&_data[_size]) T(std::forward<Args>(args)...);
        _size++;
        return _data[_size - 1]; // size was incremented previously
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

        // try-catch to roll back potential throws for memory leakage
        size_type i = 0;
        try {
            // move to new mem
            for (; i < _size; i++)
            {
                new (&new_data[i]) T(std::move_if_noexcept(_data[i]));
            }
        } catch (...) {
            // destroy constructed elements and mem if throw
            for (size_type j = 0; j < i; ++j)
            {
                new_data[j].~T();
            }
            ::operator delete(new_data);
            throw;
        }

        // destroy old elements and point to new
        for (size_type k = 0; k < _size; k++)
        {
            _data[k].~T();
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
