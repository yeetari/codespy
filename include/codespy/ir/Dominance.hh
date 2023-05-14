#pragma once

#include <codespy/support/IteratorRange.hh>

#include <unordered_map>
#include <unordered_set>

namespace codespy::ir {

class BasicBlock;
class Function;
class Instruction;

class DominanceInfo {
    std::unordered_map<BasicBlock *, BasicBlock *> m_idoms;
    std::unordered_map<BasicBlock *, std::unordered_set<BasicBlock *>> m_frontiers;

public:
    DominanceInfo(std::unordered_map<BasicBlock *, BasicBlock *> &&idoms,
                  std::unordered_map<BasicBlock *, std::unordered_set<BasicBlock *>> &&frontiers)
        : m_idoms(std::move(idoms)), m_frontiers(std::move(frontiers)) {}

    bool dominates(BasicBlock *dominator, BasicBlock *block) const;
    bool dominates(Instruction *def, Instruction *use) const;
    bool strictly_dominates(BasicBlock *dominator, BasicBlock *block) const;
    bool strictly_dominates(Instruction *def, Instruction *use) const;
    IteratorRange<std::unordered_set<BasicBlock *>::const_iterator> frontiers(BasicBlock *block) const;
    const std::unordered_map<BasicBlock *, BasicBlock *> &idoms() const { return m_idoms; }
};

DominanceInfo compute_dominance(Function *function);

} // namespace codespy::ir
