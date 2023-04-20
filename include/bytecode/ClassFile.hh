#pragma once

#include <container/Vector.hh>
#include <support/Result.hh>
#include <support/StreamError.hh>

#include <cstdint>

namespace jamf {

struct Stream;
struct Visitor;

enum class ParseError {
    BadMagic,
    UnknownConstantPoolEntry,
    UnknownOpcode,
    UnhandledAttribute,
};

Result<void, ParseError, StreamError> parse_class(Stream &stream, Visitor &visitor);

} // namespace jamf
