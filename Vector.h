#pragma once

#include <cstddef>
#include <iterator>
#include <stdexcept>
#include <utility>
#include <memory>

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

    // Creates an iterator that does not point to any element.
    VectorIterator()
        : _pointer(nullptr){}

    // Creates an iterator bound to the given raw pointer.
    explicit VectorIterator(pointer_type pointer)
        : _pointer(pointer) {}

    // Moves the iterator forward to the next element (prefix).
    VectorIterator& operator++()
    {
        ++_pointer;
        return *this;
    }

    // Moves the iterator forward and returns the previous position (postfix).
    VectorIterator operator++(int)
    {
        VectorIterator iterator(*this);
        ++(*this);
        return iterator;
    }

    // Moves the iterator backward to the previous element (prefix).
    VectorIterator& operator--()
    {
        --_pointer;
        return *this;
    }

    // Moves the iterator backward and returns the previous position (postfix).
    VectorIterator operator--(int)
    {
        VectorIterator iterator(*this);
        --(*this);
        return iterator;
    }

    // Provides indexed access relative to the current iterator.
    reference_type operator[](size_t index) const
    {
        return _pointer[index];
    }

    // Exposes the underlying pointer to access members.
    pointer_type operator->()
    {
        return _pointer;
    }

    // Exposes the underlying pointer for const access to members.
    const_pointer_type operator->() const
    {
        return _pointer;
    }

    // Dereferences the iterator to obtain the referenced element.
    reference_type operator*()
    {
        return *_pointer;
    }

    // Dereferences the iterator to obtain the referenced element (const).
    const_reference operator*() const
    {
        return *_pointer;
    }

    // Checks whether two iterators refer to the same element.
    bool operator==(const VectorIterator& other) const
    {
        return _pointer == other._pointer;
    }

    // Checks whether two iterators refer to different elements.
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

    // Constructs an empty vector with zero capacity.
    Vector() noexcept
        : _capacity(0), _size(0), _data(nullptr) {}

    // Copies elements from another vector, allocating exactly enough storage.
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

    // Takes ownership of another vector's storage without copying elements.
    Vector(Vector&& other) noexcept
        : _capacity(other._capacity), _size(other._size), _data(other._data)
    {
        // leave other in a valid, empty state
        other._data = nullptr;
        other._size = 0;
        other._capacity  = 0;
    }

    // Assigns from another vector by making a deep copy.
    Vector& operator=(const Vector& other)
    {
        if (this != &other) {
            Vector temp(other);
            swap(temp);
        }
        return *this;
    }

    // Assigns from another vector by transferring ownership of its storage.
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

    // Exchanges all with another vector.
    void swap(Vector& other) noexcept
    {
        std::swap(_data, other._data);
        std::swap(_size, other._size);
        std::swap(_capacity, other._capacity);
    }

    // Releases all elements and frees any owned storage.
    ~Vector()
    {
        for (size_type i = 0; i < _size; i++)
        {
            _data[i].~T();
        }
        ::operator delete(_data);
    }

    // Returns how many elements are currently stored.
    [[nodiscard]] size_type size() const
    {
        return _size;
    }

    // Returns how many elements can be stored without further allocation.
    [[nodiscard]] size_type capacity() const
    {
        return _capacity;
    }

    // Indicates whether the vector contains no elements.
    [[nodiscard]] bool empty() const
    {
        return _size == 0;
    }

    // Provides direct access to the underlying mutable buffer.
    pointer_type data()
    {
        return _data;
    }

    // Provides direct access to the underlying immutable buffer.
    const_pointer_type data() const noexcept
    {
        return _data;
    }

    // Returns a reference to the element at the supplied index.
    reference operator[](size_type index)
    {
        if (index >= _size) throw std::out_of_range("index out of range");
        return _data[index];
    }

    // Returns a const reference to the element at the supplied index.
    const_reference operator[](size_type index) const
    {
        if (index >= _size) throw std::out_of_range("index out of range");
        return _data[index];
    }

    // Returns a reference to the first element.
    reference front()
    {
        return _data[0];
    }

    // Returns a const reference to the first element.
    const_reference front() const
    {
        return _data[0];
    }

    // Returns a reference to the last element.
    reference back()
    {
        return _data[_size - 1];
    }

    // Returns a const reference to the last element.
    const_reference back() const
    {
        return _data[_size - 1];
    }

    // Destroys all elements while retaining allocated storage.
    void clear() {
        for (size_type i = 0; i < _size; ++i) {
            _data[i].~T();
        }
        _size = 0;
    }

    // Appends a copy of the provided value to the end of the vector.
    void push_back(const T& value)
    {
        ensure_capacity();
        new (&_data[_size]) T((value));
        ++_size;
    }

    // Appends the provided value by moving it into the vector.
    void push_back(T&& value)
    {
        ensure_capacity();
        new (&_data[_size]) T(std::move(value)); // move-construct
        ++_size;
    }

    // Constructs a new element in place at the end using the supplied arguments.
    template<typename... Args>
    reference emplace_back(Args&& ... args)
    {
        ensure_capacity();
        new (&_data[_size]) T(std::forward<Args>(args)...);
        _size++;
        return _data[_size - 1]; // size was incremented previously
    }

    // Removes the last element if the vector is not empty.
    void pop_back()
    {
        if (_size > 0)
        {
            _data[_size - 1].~T();
            --_size;
        }
    }

    // Reserves memory for new_cap capacity, preserving existing elements.
    void reserve(size_type new_cap)
    {
        if (new_cap < _size) return;
        if (new_cap > _capacity) reallocate(new_cap);
    }

    // Grows or shrinks the vector to the requested size.
    void resize(size_type new_size)
    {
        size_type curr_size = size();

        // shrink cap
        if (curr_size > new_size)
        {
            // destroy trailing elements
            for (size_type i = new_size; i < _size; ++i) {
                _data[i].~T();
            }
            _size = new_size;
            return;
        }

        // reserves new mem
        if (curr_size < new_size)
        {
            reserve(new_size);
            try
            {
                for (size_type i = curr_size; i < new_size; ++i)
                {
                    new (&_data[i]) T(); // init
                }
            } catch (...)
            {
                // destroy constructed elements if throws
                for (size_type j = curr_size; j < new_size; ++j)
                {
                    _data[j].~T(); // clean up if throw
                }
                throw;
            }
        }
        _size = new_size;
    }

    // VectorIterators
    // Returns an iterator to the first element.
    iterator begin() noexcept
    {
        return iterator(_data);
    }

    // Returns an iterator one past the last element.
    iterator end() noexcept
    {
        return iterator(_data + _size);
    }

    // Returns a const iterator to the first element.
    const_iterator begin() const noexcept
    {
        return const_iterator(_data);
    }

    // Returns a const iterator one past the last element.
    const_iterator end() const noexcept
    {
        return const_iterator(_data + _size);
    }

    // Returns a const iterator to the first element (C++ standard naming).
    const_iterator cbegin() const noexcept
    {
        return const_iterator(_data);
    }

    // Returns a const iterator one past the last element (C++ standard naming).
    const_iterator cend() const noexcept
    {
        return const_iterator(_data + _size);
    }

private:
    // Eventually move to an Allocator (?)
    // Allocates new storage and moves existing elements into it.
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

    // Expands capacity when additional space is required.
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
