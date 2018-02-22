#pragma once
#include "array.h"
#include <algorithm>
#include <ostream>
#include <string>

template<typename T>
class CircularArray
{
    Array<T> data;
    unsigned n_max;
    unsigned n{0};
    unsigned next{0};

    unsigned i_begin() const
    {
        return (next - n + n_max) % n_max;
    }

    unsigned pmod(signed x, unsigned n) const
    {
        return (x % n + n) % n;
    }

public:
    CircularArray() = default;

    CircularArray(unsigned n_max)
        : n_max(n_max)
    {
        data = Array<T>(n_max);
    }

    T& operator[](signed i)
    {
        if (i >= 0)
        {
            assert("CircularArray::operator[] const: i >= n" && unsigned(i) < n);
            return data[(i_begin() + i) % n_max];
        }

        assert("CircularArray::operator[] const: i < -n" && unsigned(-i) <= n);
        return data[(next + i + n_max) % n_max];
    }

    const T& operator[](signed i) const
    {
        if (i >= 0)
        {
            assert("CircularArray::operator[] const: i >= n" && unsigned(i) < n);
            return data[(i_begin() + i) % n_max];
        }

        assert("CircularArray::operator[] const: i < -n" && unsigned(-i) <= n);
        return data[(next + i + n_max) % n_max];
    }

    void push(const T& value)
    {
        data[next] = value;
        ++next %= n_max;
        n = std::min(n + 1, n_max);
    }

    friend std::ostream& operator<<(std::ostream& out, const CircularArray& circularArray)
    {
        out << '[';
        
        std::string separator;
        for (unsigned i = 0; i < circularArray.n; ++i)
        {
            out << separator << circularArray[i];
            separator = ", ";
        }

        return out << ']';
    }
};
