#pragma once

#include <bytecode/Definitions.hh>
#include <support/StringView.hh>

#include <cstdint>

namespace jamf {

struct Visitor {
    virtual void visit(StringView this_name, StringView super_name) = 0;
    virtual void visit_field(StringView name, StringView descriptor) = 0;
    virtual void visit_method(StringView name, StringView descriptor) = 0;

    virtual void visit_code(std::uint16_t max_stack, std::uint16_t max_locals) = 0;
    virtual void visit_constant(Constant constant) = 0;
    virtual void visit_load(BaseType type, std::uint8_t local_index) = 0;
    virtual void visit_store(BaseType type, std::uint8_t local_index) = 0;
    virtual void visit_cast(BaseType from_type, BaseType to_type) = 0;
    virtual void visit_get_field(StringView owner, StringView name, StringView descriptor, bool instance) = 0;
    virtual void visit_invoke(InvokeKind kind, StringView owner, StringView name, StringView descriptor) = 0;
    virtual void visit_return(BaseType type) = 0;
};

} // namespace jamf
