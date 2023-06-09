#include <codespy/support/StringBuilder.hh>

#include <codespy/container/Array.hh>
#include <codespy/container/Vector.hh>
#include <codespy/support/Span.hh>
#include <codespy/support/String.hh>
#include <codespy/support/StringView.hh>
#include <codespy/support/Utility.hh>

#include <algorithm>
#include <cstdint>
#include <cstdio>

namespace codespy {

void StringBuilder::append_single(double arg, const char *) {
    // TODO: Own implementation.
    Array<char, 20> buf{};
    int length = snprintf(buf.data(), buf.size(), "%f", arg); // NOLINT
    m_buffer.extend(StringView(buf.data(), std::min(buf.size(), static_cast<std::size_t>(length))));
}

void StringBuilder::append_single(std::int64_t arg, const char *opts) {
    if (arg < 0) {
        arg = -arg;
        m_buffer.push('-');
    }
    append_single(static_cast<std::uint64_t>(arg), opts);
}

void StringBuilder::append_single(std::uint64_t arg, const char *opts) {
    const size_t base = opts[0] == 'h' ? 16 : 10;
    Array<char, 30> buf{};
    std::uint8_t len = 0;
    do {
        auto digit = static_cast<char>(arg % base);
        buf[len++] = digit < 10 ? '0' + digit : 'a' + digit - 10; // NOLINT
        arg /= base;
    } while (arg > 0);

    if (const char pad = opts[1]; pad != '\0') {
        const char pad_char = opts[2] != '\0' ? opts[2] : '0';
        for (std::uint8_t i = len; i < pad - '0'; i++) {
            buf[len++] = pad_char;
        }
    }
    if (opts[0] == 'h') {
        buf[len++] = 'x';
        buf[len++] = '0';
    }
    for (std::uint8_t i = len; i > 0; i--) {
        m_buffer.push(buf[i - 1]);
    }
}

void StringBuilder::append_single(StringView arg, const char *) {
    m_buffer.extend(arg);
}

void StringBuilder::append(char ch) {
    m_buffer.push(ch);
}

void StringBuilder::truncate(std::size_t by) {
    for (size_t i = 0; i < by; i++) {
        m_buffer.pop();
    }
}

String StringBuilder::build() {
    m_buffer.push('\0');
    auto buffer = m_buffer.take_all();
    return String::move_raw(buffer.data(), buffer.size() - 1);
}

String StringBuilder::build_copy() const {
    return String::copy_raw(m_buffer.data(), m_buffer.size());
}

} // namespace codespy
