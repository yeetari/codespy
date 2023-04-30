#pragma once

#include <codespy/container/Vector.hh>
#include <codespy/support/Result.hh>
#include <codespy/support/StreamError.hh>

#include <cstdint>

namespace codespy {

struct Stream;

} // namespace codespy

namespace codespy::bc {

struct Visitor;

enum class ParseError {
    BadMagic,
    InvalidArrayType,
    UnknownConstantPoolEntry,
    UnknownOpcode,
    UnhandledAttribute,
};

Result<void, ParseError, StreamError> parse_class(Stream &stream, Visitor &visitor);

} // namespace codespy::bc
