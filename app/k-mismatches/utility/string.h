#pragma once
#include <cassert>
#include <string>

// ACHTUNG:
// this is really just a couple of pointers to positions in a string
// it doesn't store the string itself (that's mainly the point - no copying)
// so make sure to keep the string alive for the entire time this class is used w.r.t. that string

struct String
{
    using iterator_t = std::string::const_iterator;

    iterator_t beginIt, endIt;
    unsigned n;

    String() = default;

    String(const std::string& string)
        : beginIt(std::cbegin(string)), endIt(std::cend(string)), n(unsigned(endIt - beginIt))
    {}

    String(iterator_t begin, iterator_t end)
        : beginIt(begin), endIt(end), n(unsigned(end - begin))
    {
        assert("String::String: begin > end" && begin <= end);
    }

    // Read-only character access
    const char& operator[](unsigned i) const
    {
        assert("String::operator[]: i >= n" && i < n);
        return beginIt[i];
    }

    // Returns substring for slice: i_begin <= i < i_end
    String operator()(unsigned i_begin, unsigned i_end) const
    {
        return substr(i_begin, i_end);
    }

    String substr(unsigned i_begin, unsigned i_end) const
    {
        assert("String::substr: i_end > n" && i_end <= n);

        return String(beginIt + i_begin, beginIt + i_end);
    }

    iterator_t begin() const
    {
        return beginIt;
    }

    iterator_t cbegin() const
    {
        return beginIt;
    }

    iterator_t end() const
    {
        return endIt;
    }

    iterator_t cend() const
    {
        return endIt;
    }

    unsigned size() const
    {
        return n;
    }
};
