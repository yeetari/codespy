#pragma once

#include <codespy/bytecode/Visitor.hh>

namespace codespy::bc {

class Dumper : public ClassVisitor, public CodeVisitor {
public:
    void visit(StringView this_name, StringView super_name) override;
    void visit_field(StringView name, StringView descriptor) override;
    void visit_method(AccessFlags access_flags, StringView name, StringView descriptor) override;
    void visit_code(CodeAttribute &code) override;
    void visit_exception_range(std::int32_t start_pc, std::int32_t end_pc, std::int32_t handler_pc,
                               StringView type_name) override;
    void visit_constant(Constant constant) override;
    void visit_load(BaseType type, std::uint8_t local_index) override;
    void visit_store(BaseType type, std::uint8_t local_index) override;
    void visit_array_load(BaseType type) override;
    void visit_array_store(BaseType type) override;
    void visit_cast(BaseType from_type, BaseType to_type) override;
    void visit_compare(BaseType type, bool greater_on_nan) override;
    void visit_new(StringView descriptor, std::uint8_t dimensions) override;
    void visit_get_field(StringView owner, StringView name, StringView descriptor, bool instance) override;
    void visit_put_field(StringView owner, StringView name, StringView descriptor, bool instance) override;
    void visit_invoke(InvokeKind kind, StringView owner, StringView name, StringView descriptor) override;
    void visit_math_op(BaseType, MathOp math_op) override;
    void visit_monitor_op(MonitorOp monitor_op) override;
    void visit_reference_op(ReferenceOp reference_op) override;
    void visit_stack_op(StackOp stack_op) override;
    void visit_type_op(TypeOp type_op, StringView descriptor) override;
    void visit_iinc(std::uint8_t local_index, std::int32_t increment) override;
    void visit_goto(std::int32_t offset) override;
    void visit_if_compare(CompareOp compare_op, std::int32_t true_offset, CompareRhs compare_rhs) override;
    void visit_table_switch(std::int32_t low, std::int32_t high, std::int32_t default_pc,
                            Span<std::int32_t> table) override;
    void visit_lookup_switch(std::int32_t default_pc, Span<std::pair<std::int32_t, std::int32_t>> table) override;
    void visit_return(BaseType type) override;
};

} // namespace codespy::bc
