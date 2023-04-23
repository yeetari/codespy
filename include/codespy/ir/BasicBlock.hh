#pragma once

#include <codespy/container/List.hh>
#include <codespy/container/Vector.hh>
#include <codespy/ir/Instruction.hh>
#include <codespy/ir/Value.hh>
#include <codespy/support/UniquePtr.hh>

namespace codespy::ir {

class Context;

class BasicBlock : public Value, public ListNode {
    Context &m_context;
    List<Instruction> m_insts;

public:
    static constexpr auto k_kind = ValueKind::BasicBlock;

    explicit BasicBlock(Context &context);

    using iterator = decltype(m_insts)::iterator;
    iterator begin() const { return m_insts.begin(); }
    iterator end() const { return m_insts.end(); }

    template <HasOpcode Inst, typename... Args>
    Inst *insert(iterator before, Args &&...args);
    template <HasOpcode Inst, typename... Args>
    Inst *insert(Instruction *before, Args &&...args);
    template <HasOpcode Inst, typename... Args>
    Inst *prepend(Args &&...args);
    template <HasOpcode Inst, typename... Args>
    Inst *append(Args &&...args);
    iterator remove(Instruction *inst);

    Context &context() const { return m_context; }
    const List<Instruction> &insts() { return m_insts; }
};

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
