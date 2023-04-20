#include <support/Stream.hh>

#include <container/Array.hh>
#include <support/Result.hh>
#include <support/Span.hh>
#include <support/StreamError.hh>
#include <support/String.hh>
#include <support/StringView.hh>

namespace jamf {

Result<size_t, StreamError> Stream::seek(StreamOffset, SeekMode) {
    return StreamError::NotImplemented;
}

Result<size_t, StreamError> Stream::read(Span<void>) {
    return StreamError::NotImplemented;
}

Result<void, StreamError> Stream::write(Span<const void>) {
    return StreamError::NotImplemented;
}

Result<uint8_t, StreamError> Stream::read_byte() {
    uint8_t byte;
    if (JAMF_TRY(read({&byte, 1})) != 1) {
        return StreamError::Truncated;
    }
    return byte;
}

Result<void, StreamError> Stream::write_byte(uint8_t byte) {
    JAMF_TRY(write({&byte, 1}));
    return {};
}

// TODO: Inconsistent Span sizing, maybe Span can be reworked to default to size_t and allow implicit size type
//       upcasting.
Result<String, StreamError> Stream::read_string() {
    const auto length = JAMF_TRY(read_varint<uint32_t>());
    String value(length);
    if (JAMF_TRY(read({value.data(), length})) != length) {
        return StreamError::Truncated;
    }
    return value;
}

Result<void, StreamError> Stream::write_string(StringView string) {
    JAMF_TRY(write_varint(string.length()));
    JAMF_TRY(write(string.as<const void, uint32_t>()));
    return {};
}

Result<void, StreamError> Stream::write_c_string(StringView string) {
    JAMF_TRY(write(string.as<const void, uint32_t>()));
    return {};
}

} // namespace jamf
