#pragma once

namespace codespy {

class ListNode {
    template <typename>
    friend class List;

private:
    ListNode *m_prev{nullptr};
    ListNode *m_next{nullptr};

public:
    ListNode *prev() const { return m_prev; }
    ListNode *next() const { return m_next; }
};

} // namespace codespy
