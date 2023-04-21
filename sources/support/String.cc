#include <codespy/support/String.hh>

#include <codespy/support/StringView.hh>
#include <codespy/support/Utility.hh>

#include <cassert>
#include <cstring>

namespace codespy {

String String::copy_raw(const char *data, std::size_t length) {
    String string(length);
    std::memcpy(string.m_data, data, length);
    return string;
}

String String::move_raw(char *data, std::size_t length) {
    assert(data[length] == '\0');
    String string;
    string.m_data = data;
    string.m_length = length;
    return string;
}

String::String(std::size_t length) : m_length(length) {
    m_data = new char[length + 1];
    m_data[length] = '\0';
}

String::~String() {
    delete[] m_data;
}

String &String::operator=(String &&other) {
    m_data = std::exchange(other.m_data, nullptr);
    m_length = std::exchange(other.m_length, 0u);
    return *this;
}

bool String::ends_with(StringView end) {
    if (end.empty()) {
        return true;
    }
    if (empty()) {
        return false;
    }
    if (end.length() > m_length) {
        return false;
    }
    return memcmp(&m_data[m_length - end.length()], end.data(), end.length()) == 0;
}

bool String::operator==(const String &other) const {
    return StringView(m_data, m_length) == StringView(other.m_data, other.m_length);
}

} // namespace codespy
