#pragma once

#include <codespy/bytecode/Visitor.hh>
#include <codespy/container/Vector.hh>
#include <codespy/support/String.hh>
#include <codespy/support/UniquePtr.hh>

#include <deque>
#include <unordered_map>

namespace codespy::ir {

class BasicBlock;
class Context;
class Function;
class FunctionType;
class JavaClass;
class JavaField;
class Type;
class Value;

} // namespace codespy::ir

namespace codespy::bc {

class Frontend : public ClassVisitor, public CodeVisitor {
    using Stack = Vector<ir::Value *, std::uint16_t>;
    struct BlockInfo {
        ir::BasicBlock *block{nullptr};
        Stack entry_stack;
        String handler_type;
        bool handler{false};
        bool visited{false};
    };

    struct ExceptionRange {
        std::int32_t start_pc;
        std::int32_t end_pc;
        std::int32_t handler_pc;
    };

private:
    ir::Context &m_context;
    // TODO: Make key StringView to avoid copies.
    std::unordered_map<String, ir::JavaClass> m_class_map;

    ir::JavaClass *m_class;
    ir::Function *m_function;
    ir::BasicBlock *m_block;
    // TODO: Some kind of sparse storage.
    std::unordered_map<std::int32_t, BlockInfo> m_block_map;
    // TODO: Can probably be a vector.
    std::unordered_map<std::uint16_t, ir::Value *> m_local_map;
    Vector<ExceptionRange> m_exception_ranges;
    std::deque<std::int32_t> m_queue;
    Stack m_stack;

    ir::Type *lower_base_type(BaseType base_type);
    ir::Type *parse_type(StringView descriptor, std::size_t *length = nullptr);
    ir::FunctionType *parse_function_type(StringView descriptor, ir::Type *this_type);

    ir::JavaClass *ensure_class(StringView name);
    ir::BasicBlock *materialise_block(std::int32_t offset, bool save_stack);
    ir::Value *materialise_local(std::uint16_t index);

    template <typename F>
    void emit_switch(std::size_t case_count, std::int32_t default_pc, F next_case);

public:
    explicit Frontend(ir::Context &context) : m_context(context) {}
    Frontend(const Frontend &) = delete;
    Frontend(Frontend &&) = delete;
    ~Frontend();

    Frontend &operator=(const Frontend &) = delete;
    Frontend &operator=(Frontend &&) = delete;

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

    std::unordered_map<String, ir::JavaClass> &class_map() { return m_class_map; }
};

} // namespace codespy::bc
