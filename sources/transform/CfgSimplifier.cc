#include <codespy/transform/ExceptionPruner.hh>

#include <codespy/ir/BasicBlock.hh>
#include <codespy/ir/Cfg.hh>
#include <codespy/ir/Context.hh>
#include <codespy/ir/Dominance.hh>
#include <codespy/ir/Function.hh>
#include <codespy/ir/Instructions.hh>
#include <codespy/support/Print.hh>

namespace codespy::ir {

static bool do_pass(Function *function) {
    bool changed = false;
    for (auto *block : codespy::adapt_mutable_range(function->blocks())) {
        // Don't touch the entry block.
        if (block == function->entry_block()) {
            continue;
        }

        // Check if trivially dead.
        if (!block->has_uses()) {
            changed |= true;
            block->remove_from_parent();
            continue;
        }

        // Check if the block contains a single unconditional branch instruction.
        if (++block->insts().begin() == block->insts().end()) {
            if (auto *branch = ir::value_cast<ir::BranchInst>(block->terminator())) {
                changed |= true;
                block->replace_all_uses_with(branch->target());
                block->remove_from_parent();
                continue;
            }
        }
    }
    return changed;
}

void simplify_cfg(Function *function) {
    bool changed;
    do {
        changed = do_pass(function);
    } while (changed);
}

} // namespace codespy::ir
