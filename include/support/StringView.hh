#pragma once

#include <support/Span.hh>

#include <assert.h>
#include <stddef.h>

namespace jamf {

class StringView : public Span<const char, size_t> {
public:
    using Span::Span;
    constexpr StringView(const char *c_string) : Span(c_string, __builtin_strlen(c_string)) {}

    constexpr StringView substr(size_t begin) const;
    constexpr StringView substr(size_t begin, size_t end) const;
    constexpr bool operator==(StringView other) const;
    constexpr size_t length() const { return size(); }
};

constexpr StringView StringView::substr(size_t begin) const {
    return substr(begin, length());
}

constexpr StringView StringView::substr(size_t begin, size_t end) const {
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

} // namespace jamf
