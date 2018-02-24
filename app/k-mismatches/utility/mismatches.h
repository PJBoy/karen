#pragma once
#include <iostream>
#include <string>

const unsigned ALPHABET_SIZE = 256;


class Mismatches
{
    bool no;
    unsigned n, k;

public:
    Mismatches(bool no = true)
        : no(no)
    {}

    Mismatches(unsigned k, unsigned n = 0)
        : k(k), n(n)
    {
        no = n > k;
    }

    Mismatches operator+(unsigned rhs) const
    {
        Mismatches t(k, n);
        return t += rhs;
    }

    Mismatches& operator+=(unsigned rhs)
    {
        n += rhs;
        no = n > k;

        return *this;
    }

    bool operator<(Mismatches rhs) const
    {
        return *this && n < rhs.n;
    }

    operator bool() const
    {
        return !no;
    }

    operator unsigned() const
    {
        return n;
    }

    operator std::string() const
    {
        if (no)
            return "X";

        return std::to_string(n);
    }

    friend std::ostream& operator<<(std::ostream& stream, const Mismatches& mismatches)
    {
        return stream << std::string(mismatches);
    }
};
