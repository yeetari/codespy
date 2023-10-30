#include <codespy/transform/ExceptionPruner.hh>

#include <codespy/ir/BasicBlock.hh>
#include <codespy/ir/Cfg.hh>
#include <codespy/ir/Context.hh>
#include <codespy/ir/Dominance.hh>
#include <codespy/ir/Function.hh>
#include <codespy/ir/Instructions.hh>

namespace codespy::ir {

void prune_exceptions(Function *function) {
    Type *runtime_exception_type = function->context().reference_type("java/lang/RuntimeException");
    for (auto *block : function->blocks()) {
        for (auto *handler : codespy::adapt_mutable_range(block->handlers())) {
            if (handler->type() == runtime_exception_type) {
                handler->remove_from_parent();
            }
        }
    }
}

} // namespace codespy::ir
