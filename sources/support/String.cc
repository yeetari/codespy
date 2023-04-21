#include <support/String.hh>

#include <support/StringView.hh>
#include <support/Utility.hh>

#include <assert.h>
#include <string.h>

namespace jamf {

String String::copy_raw(const char *data, size_t length) {
    String string(length);
    memcpy(string.m_data, data, length);
    return string;
}

String String::move_raw(char *data, size_t length) {
    assert(data[length] == '\0');
    String string;
    string.m_data = data;
    string.m_length = length;
    return string;
}

String::String(size_t length) : m_length(length) {
    m_data = new char[length + 1];
    m_data[length] = '\0';
}

String::~String() {
    delete[] m_data;
}

String &String::operator=(String &&other) {
    m_data = exchange(other.m_data, nullptr);
    m_length = exchange(other.m_length, 0u);
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

} // namespace jamf
