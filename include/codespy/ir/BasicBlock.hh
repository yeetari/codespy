#pragma once

#include <codespy/container/List.hh>
#include <codespy/container/Vector.hh>
#include <codespy/ir/Instruction.hh>
#include <codespy/ir/Value.hh>
#include <codespy/support/UniquePtr.hh>

namespace codespy::ir {

class BasicBlock;
class Context;
class Function;

class ExceptionHandler : public Instruction {
public:
    static constexpr auto k_opcode = Opcode::ExceptionHandler;

    ExceptionHandler(BasicBlock *parent, Type *exception_type, BasicBlock *target);
    BasicBlock *target() const;
};

class BasicBlock : public Value, public ListNode {
    Context &m_context;
    Function *m_parent;
    List<Instruction> m_insts;
    List<ExceptionHandler> m_handlers;

public:
    static constexpr auto k_kind = ValueKind::BasicBlock;

    BasicBlock(Context &context, Function *parent);

    using iterator = decltype(m_insts)::iterator;
    iterator begin() const { return m_insts.begin(); }
    iterator end() const { return m_insts.end(); }

    void add_handler(Type *exception_type, BasicBlock *target);

    template <HasOpcode Inst, typename... Args>
    Inst *insert(iterator before, Args &&...args);
    template <HasOpcode Inst, typename... Args>
    Inst *insert(Instruction *before, Args &&...args);
    template <HasOpcode Inst, typename... Args>
    Inst *prepend(Args &&...args);
    template <HasOpcode Inst, typename... Args>
    Inst *append(Args &&...args);
    iterator remove(Instruction *inst);
    List<BasicBlock>::iterator remove_from_parent();

    BasicBlock *successor(unsigned index) const;
    unsigned successor_count() const;

    bool has_terminator() const;
    Instruction *terminator() const;

    Context &context() const { return m_context; }
    const List<Instruction> &insts() const { return m_insts; }
    const List<ExceptionHandler> &handlers() const { return m_handlers; }
};

inline ExceptionHandler::ExceptionHandler(BasicBlock *parent, Type *exception_type, BasicBlock *target)
    : Instruction(k_opcode, parent, exception_type, 1) {
    set_operand(0, target);
}

inline BasicBlock *ExceptionHandler::target() const {
    return static_cast<BasicBlock *>(operand(0));
}

template <HasOpcode Inst, typename... Args>
Inst *BasicBlock::insert(iterator before, Args &&...args) {
    return m_insts.emplace<Inst>(before, this, std::forward<Args>(args)...);
}

template <HasOpcode Inst, typename... Args>
Inst *BasicBlock::insert(Instruction *before, Args &&...args) {
    return insert<Inst>(iterator(before), std::forward<Args>(args)...);
}

template <HasOpcode Inst, typename... Args>
Inst *BasicBlock::prepend(Args &&...args) {
    return insert<Inst>(m_insts.begin(), std::forward<Args>(args)...);
}

template <HasOpcode Inst, typename... Args>
Inst *BasicBlock::append(Args &&...args) {
    return insert<Inst>(m_insts.end(), std::forward<Args>(args)...);
}

} // namespace codespy::ir
