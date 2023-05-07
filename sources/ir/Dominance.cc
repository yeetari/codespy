#include <codespy/ir/Dominance.hh>

#include <codespy/ir/BasicBlock.hh>
#include <codespy/ir/Cfg.hh>
#include <codespy/ir/Function.hh>

#include <unordered_map>

// A node D dominates a node N if every path from the entry to N must go from D (considered strict if N != D)
// The set of all dominators D1, D2, Dn that dominate N is denoted Dom(N)
// Each node N excl. the entry has an immediate dominator (idom), which is the unique node that strictly dominates N but
//  is itself dominated by all the nodes in Dom(N)
// A dominator tree is a tree of immediate dominators, which is useful as a node D dominates a node N iff D is a
//  (proper) ancestor of N in the tree
// If D is N's immediate dominator then every node in {Dom(N) - N} is also in Dom(D)
// Implementation of https://www.cs.rice.edu/~keith/Embed/dom.pdf

namespace codespy::ir {
namespace {

void dfs(std::unordered_map<BasicBlock *, unsigned> &index_map, Vector<BasicBlock *> &post_order, BasicBlock *block) {
    index_map[block];
    for (auto *succ : ir::succs_of(block)) {
        if (!index_map.contains(succ)) {
            dfs(index_map, post_order, succ);
        }
    }
    index_map[block] = post_order.size();
    post_order.push(block);
}

} // namespace

void compute_dominance(Function *function) {
    if (function->blocks().empty()) {
        return;
    }

    std::unordered_map<BasicBlock *, unsigned> index_map;
    Vector<BasicBlock *> order;
    dfs(index_map, order, function->entry_block());

    std::unordered_map<BasicBlock *, BasicBlock *> idoms;

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
            auto *new_idom = *ir::pred_begin(block);

            // For all other predecessors of block
            for (auto *pred : ir::preds_of(block)) {
                if (pred != new_idom && idoms.contains(pred)) {
                    new_idom = intersect(pred, new_idom);
                }
            }

            changed |= std::exchange(idoms[block], new_idom) != new_idom;
        }
    } while (std::exchange(changed, false));
}

} // namespace codespy::ir
