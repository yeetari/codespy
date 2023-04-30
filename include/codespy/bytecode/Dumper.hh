#pragma once

#include <codespy/bytecode/Visitor.hh>

namespace codespy::bc {

class Dumper : public Visitor {
public:
    void visit(StringView this_name, StringView super_name) override;
    void visit_field(StringView name, StringView descriptor) override;
    void visit_method(AccessFlags access_flags, StringView name, StringView descriptor) override;
    void visit_code(std::uint16_t max_stack, std::uint16_t max_locals) override;
    void visit_exception_range(std::int32_t start_pc, std::int32_t end_pc, std::int32_t handler_pc,
                               StringView type_name) override;
    void visit_offset(std::int32_t offset) override;
    void visit_constant(Constant constant) override;
    void visit_load(BaseType type, std::uint8_t local_index) override;
    void visit_store(BaseType type, std::uint8_t local_index) override;
    void visit_array_load(BaseType type) override;
    void visit_array_store(BaseType type) override;
    void visit_cast(BaseType from_type, BaseType to_type) override;
    void visit_new(StringView descriptor) override;
    void visit_get_field(StringView owner, StringView name, StringView descriptor, bool instance) override;
    void visit_invoke(InvokeKind kind, StringView owner, StringView name, StringView descriptor) override;
    void visit_math_op(BaseType, MathOp math_op) override;
    void visit_stack_op(StackOp stack_op) override;
    void visit_iinc(std::uint8_t local_index, std::int32_t increment) override;
    void visit_goto(std::int32_t offset) override;
    void visit_if_compare(CompareOp compare_op, std::int32_t true_offset, bool with_zero) override;
    void visit_return(BaseType type) override;
    void visit_throw() override;
};

} // namespace codespy::bc
