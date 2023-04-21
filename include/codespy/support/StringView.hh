#pragma once

#include <codespy/support/Span.hh>

#include <cassert>
#include <cstddef>

namespace codespy {

class StringView : public Span<const char> {
public:
    using Span::Span;
    constexpr StringView(const char *c_string) : Span(c_string, __builtin_strlen(c_string)) {}

    constexpr StringView substr(std::size_t begin) const;
    constexpr StringView substr(std::size_t begin, std::size_t end) const;
    constexpr bool operator==(StringView other) const;
    constexpr std::size_t length() const { return size(); }
};

constexpr StringView StringView::substr(std::size_t begin) const {
    return substr(begin, length());
}

constexpr StringView StringView::substr(std::size_t begin, std::size_t end) const {
    assert(begin + (end - begin) <= size());
    return {data() + begin, end - begin};
}

constexpr bool StringView::operator==(StringView other) const {
    if (data() == nullptr) {
        return other.data() == nullptr;
    }
    if (other.data() == nullptr) {
        return false;
    }
    if (length() != other.length()) {
        return false;
    }
    return __builtin_memcmp(data(), other.data(), length()) == 0;
}

} // namespace codespy
