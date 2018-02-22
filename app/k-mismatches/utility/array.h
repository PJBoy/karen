#pragma once
#include "mismatches.h"
#include <algorithm>
#include <cassert>
#include <functional>
#include <initializer_list>
#include <memory>
#include <numeric>
#include <ostream>


template<typename T>
class Array
{
    unsigned n{0};
    std::unique_ptr<T[]> data;

    unsigned i{0};

public:
    Array() = default;

    Array(unsigned n)
        : n(n)
    {
        data.reset(new T[n]);
    }

    Array(const Array& rhs)
    {
        n = rhs.n;
        i = rhs.i;
        data.reset(new T[n]);
        std::copy(std::cbegin(rhs), std::cend(rhs), std::begin(*this));
    }

    Array(Array&&) = default;

    Array& operator=(const Array& rhs)
    {
        n = rhs.n;
        i = rhs.i;
        data.reset(new T[n]);
        std::copy(std::cbegin(rhs), std::cend(rhs), std::begin(*this));

        return *this;
    }

    Array& operator=(Array&& rhs)
    {
        n = rhs.n;
        i = rhs.i;
        data = std::move(rhs.data);

        return *this;
    };

    const T& operator[](unsigned i) const
    {
        assert("Array::operator[] const: i >= n" && i < n);

        return data[i];
    }

    T& operator[](unsigned i)
    {
        assert("Array::operator[] const: i >= n" && i < n);
            
        return data[i];
    }

    T* begin()
    {
        return &data[0];
    }

    const T* begin() const
    {
        return &data[0];
    }

    const T* cbegin() const
    {
        return begin();
    }

    T* end()
    {
        return &data[n];
    }

    const T* end() const
    {
        return &data[n];
    }

    const T* cend() const
    {
        return end();
    }

    std::reverse_iterator<T*> rbegin()
    {
        return std::make_reverse_iterator(end());
    }

    std::reverse_iterator<const T*> rbegin() const
    {
        return std::make_reverse_iterator(end());
    }

    std::reverse_iterator<const T*> crbegin() const
    {
        return rbegin();
    }

    std::reverse_iterator<T*> rend()
    {
        return std::make_reverse_iterator(begin());
    }

    std::reverse_iterator<const T*> rend() const
    {
        return std::make_reverse_iterator(begin());
    }

    std::reverse_iterator<const T*> crend() const
    {
        return rend();
    }

    unsigned size() const
    {
        return n;
    }

    void push_back(const T& value)
    {
        assert("Array::push_back: pushed too many values" && i < n);

        data[i++] = value;
    }

    T pop_back()
    {
        assert("Array::pop_back: popped too many values" && i > 0);

        return data[--i];
    }

    friend std::ostream& operator<<(std::ostream& out, Array<T> array)
    {
        out << '[';

        std::string separator;
        for (const T& datum : array)
        {
            out << separator << datum;
            separator = ", ";
        }

        return out << ']';
    }

    T* back()
    {
        return &data[i];
    }

    unsigned back_i() const
    {
        return i;
    }
};


template<typename T>
class MultiArray
{
    Array<T> data;
    unsigned n;                  // Size of all data (product of dimensions)
    Array<unsigned> dimensions;  // Dimensions of the multiarray, used for bounds checking
    Array<unsigned> multipliers; // Multipliers a_i such that coordinates x_i access data[m] where m = x_0 + sum_{i=1} a_{i-1} x_i

public:
    MultiArray() = default;

    MultiArray(std::initializer_list<unsigned> dimensions_in)
    {
        const unsigned n_dimensions(unsigned(std::size(dimensions_in)));

        dimensions = Array<unsigned>(n_dimensions);
        std::copy(std::cbegin(dimensions_in), std::cend(dimensions_in), std::begin(dimensions));
        
        multipliers = Array<unsigned>(n_dimensions - 1);
        std::partial_sum(std::crbegin(dimensions), std::crend(dimensions) - 1, std::rbegin(multipliers), std::multiplies<>());
        
        n = dimensions[0] * multipliers[0];
        data = Array<T>(n);
    }

    T& operator[](std::initializer_list<unsigned> coordinates)
    {
        assert("MultiArray::operator[]: coordinate_i >= dimension_i" && std::inner_product(std::cbegin(coordinates), std::cend(coordinates), std::cbegin(dimensions), true, std::logical_and<>(), std::less<>()));

        const unsigned i = std::inner_product(std::cbegin(coordinates), std::cend(coordinates) - 1, std::cbegin(multipliers), *std::crbegin(coordinates));
        return data[i];
    }

    const T& operator[](std::initializer_list<unsigned> coordinates) const
    {
        assert("MultiArray::operator[]: coordinate_i >= dimension_i" && std::inner_product(std::cbegin(coordinates), std::cend(coordinates), std::cbegin(dimensions), true, std::logical_and<>(), std::less<>()));

        const unsigned i = std::inner_product(std::cbegin(coordinates), std::cend(coordinates) - 1, std::cbegin(multipliers), *std::crbegin(coordinates));
        return data[i];
    }

    T* begin()
    {
        return &data[0];
    }

    const T* begin() const
    {
        return &data[0];
    }

    const T* cbegin() const
    {
        return begin();
    }

    T* end()
    {
        return &data[n];
    }

    const T* end() const
    {
        return &data[n];
    }

    const T* cend() const
    {
        return end();
    }

    unsigned size() const
    {
        return n;
    }
};
