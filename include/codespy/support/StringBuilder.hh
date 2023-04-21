#pragma once

#include <codespy/container/Array.hh>
#include <codespy/container/Vector.hh>
#include <codespy/support/Integral.hh>
#include <codespy/support/Span.hh>
#include <codespy/support/String.hh>
#include <codespy/support/StringView.hh>

#include <cstddef>
#include <cstdint>

// TODO: Testing and benchmarking.
// TODO: Compile time format strings + checking?

namespace codespy {

class StringBuilder {
    LargeVector<char> m_buffer;

    void append_single(double, const char *);
    void append_single(std::size_t, const char *);
    void append_single(StringView, const char *);

    template <Integral T>
    void append_single(T arg, const char *opts) {
        append_single(static_cast<std::size_t>(arg), opts);
    }

    template <typename T>
    void append_part(StringView fmt, std::size_t &index, const T &arg);

public:
    template <typename... Args>
    void append(StringView fmt, const Args &...args);
    void append(char ch);
    void truncate(std::size_t by);

    String build();
    String build_copy() const;

    bool empty() const { return m_buffer.empty(); }
    size_t length() const { return m_buffer.size(); }
};

template <typename T>
void StringBuilder::append_part(StringView fmt, size_t &index, const T &arg) {
    while (index < fmt.length() && fmt[index] != '{') {
        m_buffer.push(fmt[index++]);
    }

    if (index >= fmt.length() || fmt[index++] != '{') {
        return;
    }

    Array<char, 4> opts{};
    for (uint32_t i = 0; i < opts.size() && fmt[index] != '}';) {
        opts[i++] = fmt[index++];
    }
    // NOLINTNEXTLINE
    append_single(arg, opts.data());
    index++;
}

template <typename... Args>
void StringBuilder::append(StringView fmt, const Args &...args) {
    std::size_t index = 0;
    (append_part(fmt, index, args), ...);
    m_buffer.extend(codespy::make_span(fmt.begin() + index, fmt.length() - index));
}

} // namespace codespy
