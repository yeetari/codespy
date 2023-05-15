#include <codespy/ir/Dominance.hh>

#include <codespy/ir/BasicBlock.hh>
#include <codespy/ir/Cfg.hh>
#include <codespy/ir/Function.hh>

#include <unordered_map>
#include <unordered_set>

// Implementation of https://www.cs.rice.edu/~keith/Embed/dom.pdf
// A node D dominates a node N if every path from the entry to N must go from D (considered strict if N != D)
// The set of all dominators D1, D2, Dn that dominate N is denoted Dom(N)
// Each node N excl. the entry has an immediate dominator (idom), which is the unique node that strictly dominates N but
//  is itself dominated by all the nodes in Dom(N)
// A dominator tree is a tree of immediate dominators, which is useful as a node D dominates a node N iff D is a
//  (proper) ancestor of N in the tree
// If D is N's immediate dominator then every node in {Dom(N) - N} is also in Dom(D)

namespace codespy::ir {

bool DominanceInfo::dominates(BasicBlock *dominator, BasicBlock *block) const {
    for (auto *idom = block; idom != nullptr; idom = m_idoms.at(idom)) {
        if (idom == dominator) {
            return true;
        }
        if (idom == idom->parent()->entry_block()) {
            return false;
        }
    }
    return false;
}

bool DominanceInfo::strictly_dominates(BasicBlock *dominator, BasicBlock *block) const {
    return dominator != block && dominates(dominator, block);
}

bool DominanceInfo::dominates(Instruction *def, Instruction *user) const {
    if (def == user) {
        return false;
    }

    auto *def_block = def->parent();
    if (def_block != user->parent()) {
        return dominates(def_block, user->parent());
    }

    // Instructions within the same block, compare order in the instruction list.
    for (auto *inst : *def_block) {
        if (inst == def) {
            return true;
        }
        if (inst == user) {
            return false;
        }
    }
    codespy::unreachable();
}

IteratorRange<std::unordered_set<BasicBlock *>::const_iterator> DominanceInfo::frontiers(BasicBlock *block) const {
    if (!m_frontiers.contains(block)) {
        return {{}, {}};
    }
    const auto &set = m_frontiers.at(block);
    return codespy::make_range(set.begin(), set.end());
}

static void dfs(std::unordered_map<BasicBlock *, unsigned> &index_map, Vector<BasicBlock *> &post_order,
                BasicBlock *block) {
    index_map[block];
    for (auto *succ : ir::succs_of(block)) {
        if (!index_map.contains(succ)) {
            dfs(index_map, post_order, succ);
        }
    }
    index_map[block] = post_order.size();
    post_order.push(block);
}

DominanceInfo compute_dominance(Function *function) {
    if (function->blocks().empty()) {
        return {{}, {}};
    }

    std::unordered_map<BasicBlock *, unsigned> index_map;
    Vector<BasicBlock *> order;
    dfs(index_map, order, function->entry_block());

    std::unordered_map<BasicBlock *, BasicBlock *> idoms;
    idoms.emplace(function->entry_block(), function->entry_block());

    auto intersect = [&](BasicBlock *finger1, BasicBlock *finger2) {
        while (finger1 != finger2) {
            while (index_map.at(finger1) < index_map.at(finger2)) {
                finger1 = idoms.at(finger1);
            }
            while (index_map.at(finger2) < index_map.at(finger1)) {
                finger2 = idoms.at(finger2);
            }
        }
        return finger1;
    };

    assert(order.last() == function->entry_block());
    bool changed = false;
    do {
        // For all nodes in reverse postorder (excl. entry)
        for (unsigned i = order.size() - 1; i > 0; i--) {
            auto *block = order[i - 1];

            // For all other predecessors of block
            ir::BasicBlock *new_idom = nullptr;
            for (auto *pred : ir::preds_of(block)) {
                if (!idoms.contains(pred)) {
                    continue;
                }
                if (new_idom == nullptr) {
                    new_idom = pred;
                } else {
                    new_idom = intersect(pred, new_idom);
                }
            }

            changed |= std::exchange(idoms[block], new_idom) != new_idom;
        }
    } while (std::exchange(changed, false));

    std::unordered_map<BasicBlock *, std::unordered_set<BasicBlock *>> frontiers;
    for (auto *block : function->blocks()) {
        if (std::distance(ir::pred_begin(block), ir::pred_end(block)) < 2) {
            // Not a join point.
            continue;
        }
        for (auto *runner : ir::preds_of(block)) {
            while (runner != idoms.at(block)) {
                frontiers[runner].insert(block);
                runner = idoms.at(runner);
            }
        }
    }
    return {std::move(idoms), std::move(frontiers)};
}

} // namespace codespy::ir
