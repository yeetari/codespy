#pragma once

#include <codespy/ir/BasicBlock.hh>
#include <codespy/ir/Instruction.hh>
#include <codespy/support/IteratorRange.hh>

namespace codespy::ir {

class PredecessorIterator {
    UserIterator m_it;

public:
    explicit PredecessorIterator(UserIterator it) : m_it(it) {
        if (!it.at_end() && !it->is<Instruction>()) {
            ++*this;
        }
    }

    PredecessorIterator &operator++() {
        do {
            ++m_it;
        } while (!m_it.at_end() && !m_it->is<Instruction>());
        return *this;
    }
    PredecessorIterator operator++(int) {
        PredecessorIterator tmp = *this;
        ++*this;
        return tmp;
    }

    bool operator==(const PredecessorIterator &other) const { return m_it == other.m_it; }

    BasicBlock *operator*() const { return m_it->as<Instruction>()->parent(); }
    BasicBlock *operator->() const { return operator*(); }
};

inline auto pred_begin(BasicBlock *block) {
    return PredecessorIterator(block->user_begin());
}

inline auto pred_end(BasicBlock *block) {
    return PredecessorIterator(block->user_end());
}

inline auto preds_of(BasicBlock *block) {
    return codespy::make_range(pred_begin(block), pred_end(block));
}

class SuccessorIterator {
    Instruction *const m_terminator;
    unsigned m_index;

public:
    SuccessorIterator(Instruction *terminator, unsigned index) : m_terminator(terminator), m_index(index) {}

    SuccessorIterator &operator++() {
        m_index++;
        return *this;
    }
    SuccessorIterator operator++(int) {
        SuccessorIterator tmp = *this;
        ++*this;
        return tmp;
    }

    bool operator==(const SuccessorIterator &other) const { return m_index == other.m_index; }

    BasicBlock *operator*() const { return m_terminator->successor(m_index); }
    BasicBlock *operator->() const { return operator*(); }
};

inline auto succ_begin(Instruction *terminator) {
    return SuccessorIterator(terminator, 0);
}
inline auto succ_begin(BasicBlock *block) {
    return succ_begin(block->terminator());
}

inline auto succ_end(Instruction *terminator) {
    return SuccessorIterator(terminator, terminator->successor_count());
}
inline auto succ_end(BasicBlock *block) {
    return succ_end(block->terminator());
}

inline auto succs_of(BasicBlock *block) {
    return codespy::make_range(succ_begin(block), succ_end(block));
}

} // namespace codespy::ir
