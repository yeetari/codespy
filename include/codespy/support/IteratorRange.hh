#pragma once

namespace codespy {

template <typename It>
class IteratorRange {
    It m_begin;
    It m_end;

public:
    IteratorRange(It begin, It end) : m_begin(begin), m_end(end) {}

    It begin() const { return m_begin; }
    It end() const { return m_end; }
};

template <typename It>
class MutableIterator {
    It m_it;

public:
    MutableIterator(It it) : m_it(it) {}
    MutableIterator &operator++() { return *this; }
    decltype(auto) operator*() { return *m_it++; }
    bool operator==(const MutableIterator &other) const { return m_it == other.m_it; }
};

template <typename It>
auto make_range(It begin, It end) {
    return IteratorRange(begin, end);
}

template <typename Range>
auto adapt_mutable_range(Range &&range) {
    return make_range(MutableIterator(range.begin()), MutableIterator(range.end()));
}

} // namespace codespy
