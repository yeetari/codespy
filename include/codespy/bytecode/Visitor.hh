#pragma once

#include <codespy/bytecode/Definitions.hh>
#include <codespy/support/StringView.hh>

#include <cstdint>

namespace codespy::bc {

struct Visitor {
    virtual void visit(StringView this_name, StringView super_name) = 0;
    virtual void visit_field(StringView name, StringView descriptor) = 0;
    virtual void visit_method(AccessFlags access_flags, StringView name, StringView descriptor) = 0;

    virtual void visit_code(std::uint16_t max_stack, std::uint16_t max_locals) = 0;
    virtual void visit_constant(Constant constant) = 0;
    virtual void visit_load(BaseType type, std::uint8_t local_index) = 0;
    virtual void visit_store(BaseType type, std::uint8_t local_index) = 0;
    virtual void visit_cast(BaseType from_type, BaseType to_type) = 0;
    virtual void visit_get_field(StringView owner, StringView name, StringView descriptor, bool instance) = 0;
    virtual void visit_invoke(InvokeKind kind, StringView owner, StringView name, StringView descriptor) = 0;
    virtual void visit_stack_op(StackOp stack_op) = 0;
    virtual void visit_if_cmp(CompareOp compare_op, std::int32_t true_offset, bool with_zero) = 0;
    virtual void visit_return(BaseType type) = 0;
};

} // namespace codespy::bc
