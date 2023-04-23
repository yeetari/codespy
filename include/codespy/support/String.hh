#pragma once

#include <codespy/support/Span.hh>
#include <codespy/support/StringView.hh>
#include <codespy/support/Utility.hh>

#include <cstddef>
#include <functional>
#include <string_view>
#include <utility>

namespace codespy {

class String {
    char *m_data{nullptr};
    std::size_t m_length{0};

public:
    static String copy_raw(const char *data, std::size_t length);
    static String move_raw(char *data, std::size_t length);

    constexpr String() = default;
    explicit String(std::size_t length);
    String(const char *c_string) : String(copy_raw(c_string, __builtin_strlen(c_string))) {}
    String(StringView view) : String(copy_raw(view.data(), view.length())) {}
    String(const String &other) : String(copy_raw(other.m_data, other.m_length)) {}
    String(String &&other)
        : m_data(std::exchange(other.m_data, nullptr)), m_length(std::exchange(other.m_length, 0u)) {}
    ~String();

    String &operator=(const String &) = delete;
    String &operator=(String &&);

    char *begin() const { return m_data; }
    char *end() const { return m_data + m_length; }

    char *data() { return m_data; }
    const char *data() const { return m_data; }

    bool ends_with(StringView end);

    operator StringView() const { return view(); }
    StringView view() const { return {m_data, m_length}; }

    char *disown() { return std::exchange(m_data, nullptr); }

    bool operator==(const String &other) const;
    bool empty() const { return m_length == 0; }
    std::size_t length() const { return m_length; }
};

} // namespace codespy

namespace std {

template <>
struct hash<codespy::String> {
    std::size_t operator()(const codespy::String &string) const {
        return hash<std::string_view>{}(std::string_view{string.data(), string.length()});
    }
};

} // namespace std
