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
auto make_range(It begin, It end) {
    return IteratorRange(begin, end);
}

} // namespace codespy
