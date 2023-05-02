#pragma once

#include <codespy/container/FixedBuffer.hh>
#include <codespy/support/Result.hh>
#include <codespy/support/StreamError.hh>

#include <cstdint>

namespace codespy {

struct Stream;

} // namespace codespy

namespace codespy::bc {

class ConstantPool;
struct ClassVisitor;
struct CodeVisitor;

enum class ParseError {
    BadMagic,
    InvalidArrayType,
    UnknownConstantPoolEntry,
    UnknownOpcode,
    UnhandledAttribute,
};

class CodeAttribute {
    ConstantPool &m_constant_pool;
    std::uint16_t m_max_stack;
    std::uint16_t m_max_locals;
    FixedBuffer<std::uint8_t> m_buffer;

public:
    CodeAttribute(ConstantPool &constant_pool, std::uint16_t max_stack, std::uint16_t max_locals,
                  FixedBuffer<std::uint8_t> &&buffer)
        : m_constant_pool(constant_pool), m_max_stack(max_stack), m_max_locals(max_locals),
          m_buffer(std::move(buffer)) {}

    Result<std::int32_t, ParseError, StreamError> parse_inst(std::int32_t pc, CodeVisitor &visitor);

    std::uint16_t max_stack() const { return m_max_stack; }
    std::uint16_t max_locals() const { return m_max_locals; }
    std::int32_t code_end() const { return static_cast<std::int32_t>(m_buffer.size()); }
};

Result<void, ParseError, StreamError> parse_class(Stream &stream, ClassVisitor &visitor);

} // namespace codespy::bc
