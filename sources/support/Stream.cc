#include <codespy/support/Stream.hh>

#include <codespy/container/Array.hh>
#include <codespy/support/Result.hh>
#include <codespy/support/Span.hh>
#include <codespy/support/StreamError.hh>
#include <codespy/support/String.hh>
#include <codespy/support/StringView.hh>

namespace codespy {

Result<std::size_t, StreamError> Stream::seek(StreamOffset, SeekMode) {
    return StreamError::NotImplemented;
}

Result<std::size_t, StreamError> Stream::read(Span<void>) {
    return StreamError::NotImplemented;
}

Result<void, StreamError> Stream::write(Span<const void>) {
    return StreamError::NotImplemented;
}

Result<std::uint8_t, StreamError> Stream::read_byte() {
    std::uint8_t byte;
    if (CODESPY_TRY(read({&byte, 1})) != 1) {
        return StreamError::Truncated;
    }
    return byte;
}

Result<void, StreamError> Stream::write_byte(std::uint8_t byte) {
    CODESPY_TRY(write({&byte, 1}));
    return {};
}

// TODO: Inconsistent Span sizing, maybe Span can be reworked to default to size_t and allow implicit size type
//       upcasting.
Result<String, StreamError> Stream::read_string() {
    const auto length = CODESPY_TRY(read_varint<std::uint32_t>());
    String value(length);
    if (CODESPY_TRY(read({value.data(), length})) != length) {
        return StreamError::Truncated;
    }
    return value;
}

Result<void, StreamError> Stream::write_string(StringView string) {
    CODESPY_TRY(write_varint(string.length()));
    CODESPY_TRY(write(string));
    return {};
}

} // namespace codespy
