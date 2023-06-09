#pragma once

#include <codespy/ir/BasicBlock.hh>
#include <codespy/ir/Instruction.hh>
#include <codespy/support/IteratorRange.hh>

#include <iterator>

namespace codespy::ir {

class PredecessorIterator {
    UserIterator m_it;

public:
    explicit PredecessorIterator(UserIterator it) : m_it(it) {
        if (!it.at_end() && !value_is<Instruction>(*it)) {
            ++*this;
        }
    }

    PredecessorIterator &operator++() {
        do {
            ++m_it;
        } while (!m_it.at_end() && !value_is<Instruction>(*m_it));
        return *this;
    }
    PredecessorIterator operator++(int) {
        PredecessorIterator tmp = *this;
        ++*this;
        return tmp;
    }

    bool operator==(const PredecessorIterator &other) const { return m_it == other.m_it; }

    BasicBlock *operator*() const { return value_cast<Instruction>(*m_it)->parent(); }
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
    BasicBlock *const m_block;
    unsigned m_index;

public:
    SuccessorIterator(BasicBlock *block, unsigned index) : m_block(block), m_index(index) {}

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

    BasicBlock *operator*() const { return m_block->successor(m_index); }
    BasicBlock *operator->() const { return operator*(); }
};

inline auto succ_begin(BasicBlock *block) {
    return SuccessorIterator(block, 0);
}

inline auto succ_end(BasicBlock *block) {
    return SuccessorIterator(block, block->successor_count());
}

inline auto succs_of(BasicBlock *block) {
    return codespy::make_range(succ_begin(block), succ_end(block));
}

} // namespace codespy::ir

namespace std {

template <>
struct iterator_traits<codespy::ir::PredecessorIterator> {
    using difference_type = std::ptrdiff_t;
    using value_type = codespy::ir::BasicBlock *;
    using pointer = value_type *;
    using reference = value_type &;
    using iterator_category = std::forward_iterator_tag;
};

template <>
struct iterator_traits<codespy::ir::SuccessorIterator> {
    using difference_type = std::ptrdiff_t;
    using value_type = codespy::ir::BasicBlock *;
    using pointer = value_type *;
    using reference = value_type &;
    using iterator_category = std::forward_iterator_tag;
};

} // namespace std
