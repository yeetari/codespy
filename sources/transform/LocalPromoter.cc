#include <codespy/transform/LocalPromoter.hh>

#include <codespy/container/Vector.hh>
#include <codespy/ir/Cfg.hh>
#include <codespy/ir/Context.hh>
#include <codespy/ir/Dominance.hh>
#include <codespy/ir/Function.hh>
#include <codespy/ir/Instructions.hh>

#include <unordered_map>
#include <unordered_set>

namespace codespy::ir {
namespace {

struct PhiInfo {
    Local *local;
    unsigned incoming_index{0};
};

class LocalPromoter {
    Function *m_function;
    DominanceInfo m_dom_info;
    std::unordered_map<Local *, Vector<Value *>> m_reaching_value_map;
    std::unordered_map<PhiInst *, PhiInfo> m_phi_info_map;
    std::unordered_set<BasicBlock *> m_visited_blocks;

public:
    LocalPromoter(Function *function, DominanceInfo &&dom_info) : m_function(function), m_dom_info(std::move(dom_info)) {}

    bool handle_trivial_local(Local *local);
    void rename_recursive(BasicBlock *block);
    void run();
};

bool LocalPromoter::handle_trivial_local(Local *local) {
    if (!local->has_uses()) {
        // Trivially dead.
        return true;
    }

    bool in_single_block = true;
    bool has_single_store = true;
    BasicBlock *single_block = nullptr;
    StoreInst *single_store = nullptr;
    for (auto *user : local->users()) {
        if (auto *inst = ir::value_cast<Instruction>(user)) {
            if (single_block == nullptr) {
                single_block = inst->parent();
            } else {
                in_single_block &= single_block == inst->parent();
            }
        }
        if (auto *store = ir::value_cast<StoreInst>(user)) {
            has_single_store &= single_store == nullptr;
            single_store = store;
        }
    }

    // No stores, every use is undefined.
    if (has_single_store && single_store == nullptr) {
        // TODO: Replace loads with undef value.
        assert(false);
        return true;
    }

    if (has_single_store) {
        // Single store implies that every use can only ever resolve to the stored value or undef, depending on whether
        // the store dominates the use or not respectively.
        for (auto *user : codespy::adapt_mutable_range(local->users())) {
            if (auto *load = ir::value_cast<LoadInst>(user)) {
                // TODO: undef value.
                auto *reaching_value = m_dom_info.dominates(single_store, load) ? single_store->value() : nullptr;
                assert(reaching_value != nullptr);
                load->replace_all_uses_with(reaching_value);
                load->remove_from_parent();
            }
        }
        single_store->remove_from_parent();
        return true;
    }

    if (!in_single_block) {
        return false;
    }

    // Symbolic execution of memory operations.
    Value *reaching_value = nullptr;
    for (auto *inst : codespy::adapt_mutable_range(*single_block)) {
        if (auto *load = ir::value_cast<LoadInst>(inst)) {
            if (load->pointer() == local) {
                assert(reaching_value != nullptr);
                load->replace_all_uses_with(reaching_value);
                load->remove_from_parent();
            }
        } else if (auto *store = ir::value_cast<StoreInst>(inst)) {
            if (store->pointer() == local) {
                reaching_value = store->value();
                store->remove_from_parent();
            }
        }
    }
    return true;
}

void LocalPromoter::rename_recursive(BasicBlock *block) {
    if (!m_visited_blocks.insert(block).second) {
        return;
    }

    // Symbolic execution of memory operations.
    for (auto *inst : *block) {
        if (auto *load = ir::value_cast<LoadInst>(inst)) {
            auto *local = ir::value_cast<Local>(load->pointer());
            if (local != nullptr && !m_reaching_value_map[local].empty()) {
                load->replace_all_uses_with(m_reaching_value_map.at(local).last());
            }
        } else if (auto *store = ir::value_cast<StoreInst>(inst)) {
            // TODO: Assuming all locals promotable here, may not be in the future.
            auto *local = ir::value_cast<Local>(store->pointer());
            if (local != nullptr) {
                m_reaching_value_map[local].push(store->value());
            }
        } else if (auto *phi = ir::value_cast<PhiInst>(inst); m_phi_info_map.contains(phi)) {
            const auto &info = m_phi_info_map.at(phi);
            m_reaching_value_map[info.local].push(phi);
        }
    }

    // Update successor PHIs with reaching values.
    for (auto *succ : ir::succs_of(block)) {
        for (auto *inst : *succ) {
            auto *phi = ir::value_cast<PhiInst>(inst);
            if (phi == nullptr) {
                // PHIs must be contiguous at the top of a block, so the first non-PHI we see indicates no more
                // PHIs are left.
                break;
            }
            if (!m_phi_info_map.contains(phi)) {
                // PHI not inserted by us.
                continue;
            }
            auto &info = m_phi_info_map.at(phi);
            auto *reaching_value = block->context().constant_null();
            if (!m_reaching_value_map[info.local].empty()) {
                reaching_value = m_reaching_value_map[info.local].last();
            }
            phi->set_incoming(info.incoming_index++, block, reaching_value);
        }
    }

    // Recurse over successors.
    for (auto *succ : ir::succs_of(block)) {
        rename_recursive(succ);
    }

    for (auto *inst : codespy::adapt_mutable_range(*block)) {
        if (auto *load = ir::value_cast<LoadInst>(inst)) {
            auto *local = ir::value_cast<Local>(load->pointer());
            if (local != nullptr) {
                load->remove_from_parent();
            }
        } else if (auto *store = ir::value_cast<StoreInst>(inst)) {
            // TODO: Assuming all locals promotable here, may not be in the future.
            auto *local = ir::value_cast<Local>(store->pointer());
            if (local != nullptr) {
                m_reaching_value_map[local].pop();
                store->remove_from_parent();
            }
        } else if (auto *phi = ir::value_cast<PhiInst>(inst); m_phi_info_map.contains(phi)) {
            const auto &info = m_phi_info_map.at(phi);
            m_reaching_value_map[info.local].pop();
        }
    }
}

void LocalPromoter::run() {
    for (auto *local : codespy::adapt_mutable_range(m_function->locals())) {
        if (handle_trivial_local(local)) {
            m_function->remove_local(local);
            continue;
        }

        // Otherwise there are multiple stores, need to insert PHIs at join points.
        std::unordered_set<BasicBlock *> visited;
        for (auto *user : local->users()) {
            auto *store = ir::value_cast<StoreInst>(user);
            if (store == nullptr) {
                continue;
            }
            for (auto *df : m_dom_info.frontiers(store->parent())) {
                if (!visited.insert(df).second) {
                    continue;
                }
                const auto pred_count = std::distance(ir::pred_begin(df), ir::pred_end(df));
                auto *phi = df->prepend<PhiInst>(pred_count);
                m_phi_info_map.emplace(phi, PhiInfo{.local = local});
            }
        }
    }

    if (m_function->locals().empty()) {
        // No locals left, nothing else to do.
        return;
    }

    // Rename pass.
    rename_recursive(m_function->entry_block());

    // Delete any dead locals.
    for (auto *local : codespy::adapt_mutable_range(m_function->locals())) {
        if (!local->has_uses()) {
            m_function->remove_local(local);
        }
    }
}

} // namespace

void promote_locals(Function *function) {
    auto dom_info = ir::compute_dominance(function);
    LocalPromoter(function, std::move(dom_info)).run();
}

} // namespace codespy::ir
