#pragma once

#include <codespy/bytecode/Definitions.hh>
#include <codespy/support/StringView.hh>

#include <cstdint>

namespace codespy::bc {

class CodeAttribute;

struct ClassVisitor {
    virtual void visit(StringView this_name, StringView super_name) = 0;
    virtual void visit_field(StringView name, StringView descriptor) = 0;
    virtual void visit_method(AccessFlags access_flags, StringView name, StringView descriptor) = 0;
    virtual void visit_exception_range(std::int32_t start_pc, std::int32_t end_pc, std::int32_t handler_pc,
                                       StringView type_name) = 0;
    virtual void visit_code(CodeAttribute &code) = 0;
};

struct CodeVisitor {
    virtual void visit_constant(Constant /*constant*/) {}
    virtual void visit_load(BaseType /*type*/, std::uint8_t /*local_index*/) {}
    virtual void visit_store(BaseType /*type*/, std::uint8_t /*local_index*/) {}
    virtual void visit_array_load(BaseType /*type*/) {}
    virtual void visit_array_store(BaseType /*type*/) {}
    virtual void visit_cast(BaseType /*from_type*/, BaseType /*to_type*/) {}
    virtual void visit_compare(BaseType /*type*/, bool /*greater_on_nan*/) {}
    virtual void visit_new(StringView /*descriptor*/, std::uint8_t /*dimensions*/ = 1) {}
    virtual void visit_get_field(StringView /*owner*/, StringView /*name*/, StringView /*descriptor*/,
                                 bool /*instance*/) {}
    virtual void visit_put_field(StringView /*owner*/, StringView /*name*/, StringView /*descriptor*/,
                                 bool /*instance*/) {}
    virtual void visit_invoke(InvokeKind /*kind*/, StringView /*owner*/, StringView /*name*/,
                              StringView /*descriptor*/) {}
    virtual void visit_math_op(BaseType /*type*/, MathOp /*math_op*/) {}
    virtual void visit_monitor_op(MonitorOp /*monitor_op*/) {}
    virtual void visit_reference_op(ReferenceOp /*reference_op*/) {}
    virtual void visit_stack_op(StackOp /*stack_op*/) {}
    virtual void visit_type_op(TypeOp /*type_op*/, StringView /*descriptor*/) {}
    virtual void visit_iinc(std::uint8_t /*local_index*/, std::int32_t /*increment*/) {}
    virtual void visit_goto(std::int32_t /*offset*/) {}
    virtual void visit_if_compare(CompareOp /*compare_op*/, std::int32_t /*true_offset*/, CompareRhs /*compare_rhs*/) {}
    virtual void visit_table_switch(std::int32_t /*low*/, std::int32_t /*high*/, std::int32_t /*default_pc*/,
                                    Span<std::int32_t> /*table*/) {}
    virtual void visit_lookup_switch(std::int32_t /*default_pc*/,
                                     Span<std::pair<std::int32_t, std::int32_t>> /*table*/) {}
    virtual void visit_return(BaseType /*type*/) {}
};

} // namespace codespy::bc
