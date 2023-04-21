#pragma once

#include <codespy/support/Span.hh>
#include <codespy/support/Stream.hh>

#include <algorithm>
#include <cstdint>
#include <cstring>

namespace codespy {

class SpanStream final : public Stream {
    const Span<const void> m_span;
    std::size_t m_head{0};

public:
    explicit SpanStream(Span<const void> span) : m_span(span) {}

    Result<std::size_t, StreamError> seek(StreamOffset offset, SeekMode mode) override;
    Result<std::size_t, StreamError> read(Span<void> data) override;
};

inline Result<std::size_t, StreamError> SpanStream::seek(StreamOffset offset, SeekMode mode) {
    switch (mode) {
    case SeekMode::Set:
        m_head = offset;
        break;
    case SeekMode::Add:
        m_head += offset;
        break;
    default:
        return StreamError::NotImplemented;
    }
    return m_head;
}

inline Result<size_t, StreamError> SpanStream::read(Span<void> data) {
    const auto to_read = std::min(data.size(), m_span.size() - m_head);
    std::memcpy(data.data(), m_span.byte_offset(m_head), to_read);
    m_head += to_read;
    return to_read;
}

} // namespace codespy
