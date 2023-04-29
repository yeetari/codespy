#pragma once

#include <codespy/bytecode/Visitor.hh>
#include <codespy/container/Vector.hh>
#include <codespy/support/String.hh>
#include <codespy/support/UniquePtr.hh>

#include <unordered_map>

namespace codespy::ir {

class BasicBlock;
class Context;
class Function;
class FunctionType;
class Type;
class Value;

} // namespace codespy::ir

namespace codespy::bc {

class Frontend : public Visitor {
    using Stack = Vector<ir::Value *, std::uint16_t>;
    struct BlockInfo {
        ir::BasicBlock *block{nullptr};
        Stack entry_stack;
    };

private:
    UniquePtr<ir::Context> m_context;
    StringView m_this_name;
    ir::Function *m_function;
    ir::BasicBlock *m_block;
    std::unordered_map<std::int32_t, BlockInfo> m_block_map;
    std::unordered_map<String, ir::Function *> m_function_map;
    std::unordered_map<std::uint16_t, ir::Value *> m_local_map;
    Stack m_stack;

    ir::Type *lower_base_type(BaseType base_type);
    ir::Type *parse_type(StringView descriptor, std::size_t *length = nullptr);
    ir::FunctionType *parse_function_type(StringView descriptor, ir::Type *this_type);
    ir::Function *materialise_function(StringView owner, StringView name, StringView descriptor, ir::Type *this_type);
    ir::BasicBlock *materialise_block(std::int32_t offset);
    ir::Value *materialise_local(std::uint16_t index);

public:
    Frontend();
    Frontend(const Frontend &) = delete;
    Frontend(Frontend &&) = delete;
    ~Frontend();

    Frontend &operator=(const Frontend &) = delete;
    Frontend &operator=(Frontend &&) = delete;

    void visit(StringView this_name, StringView super_name) override;
    void visit_field(StringView name, StringView descriptor) override;
    void visit_method(AccessFlags access_flags, StringView name, StringView descriptor) override;

    void visit_code(std::uint16_t max_stack, std::uint16_t max_locals) override;
    void visit_jump_target(std::int32_t offset) override;
    void visit_offset(std::int32_t offset) override;
    void visit_constant(Constant constant) override;
    void visit_load(BaseType type, std::uint8_t local_index) override;
    void visit_store(BaseType type, std::uint8_t local_index) override;
    void visit_cast(BaseType from_type, BaseType to_type) override;
    void visit_get_field(StringView owner, StringView name, StringView descriptor, bool instance) override;
    void visit_invoke(InvokeKind kind, StringView owner, StringView name, StringView descriptor) override;
    void visit_math_op(BaseType, MathOp math_op) override;
    void visit_stack_op(StackOp stack_op) override;
    void visit_iinc(std::uint8_t local_index, std::int32_t increment) override;
    void visit_goto(std::int32_t offset) override;
    void visit_if_compare(CompareOp compare_op, std::int32_t true_offset, bool with_zero) override;
    void visit_return(BaseType type) override;

    Vector<ir::Function *> functions();
};

} // namespace codespy::bc
