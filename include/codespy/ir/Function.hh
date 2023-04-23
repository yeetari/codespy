#pragma once

#include <codespy/container/List.hh>
#include <codespy/ir/BasicBlock.hh>
#include <codespy/support/String.hh>

namespace codespy::ir {

class Context;
class FunctionType;

class Argument : public Value, public ListNode {
public:
    static constexpr auto k_kind = ValueKind::Argument;

    explicit Argument(Type *type);
};

class Function : public Value {
    Context &m_context;
    String m_name;
    List<Argument> m_arguments;
    List<BasicBlock> m_blocks;

public:
    static constexpr auto k_kind = ValueKind::Function;

    Function(Context &context, String name, FunctionType *type);

    BasicBlock *append_block();
    Argument *argument(std::size_t index);

    FunctionType *function_type() const;
    Context &context() const { return m_context; }
    const String &name() const { return m_name; }
    const List<Argument> &arguments() const { return m_arguments; }
    const List<BasicBlock> &blocks() const { return m_blocks; }
};

} // namespace codespy::ir
