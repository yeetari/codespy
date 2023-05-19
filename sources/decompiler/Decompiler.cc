#include <codespy/decompiler/Decompiler.hh>

#include <codespy/container/Vector.hh>
#include <codespy/decompiler/Ast.hh>
#include <codespy/ir/BasicBlock.hh>
#include <codespy/ir/Cfg.hh>
#include <codespy/ir/Dominance.hh>
#include <codespy/ir/Function.hh>
#include <codespy/ir/Instructions.hh>
#include <codespy/support/Print.hh>

#include <unordered_set>

namespace codespy {
namespace {

class Decompiler {
    std::unordered_map<ir::BasicBlock *, AstSequence *> m_ast_block_map;
    std::unordered_map<ir::BasicBlock *, Vector<ir::Value *>> m_reaching_conditions;

public:
    void iterate_reaching_conditions(ir::BasicBlock *block, Vector<ir::Value *> &condition_stack,
                                     Vector<ir::BasicBlock *> &visit_stack);
    void structure_region(ir::BasicBlock *entry, ir::BasicBlock *exit);
    void print_ast(AstSequence *sequence);
    void print_ast(ir::BasicBlock *function_entry);
};

void dfs(Vector<ir::BasicBlock *> &post_order, std::unordered_set<ir::BasicBlock *> &visited, ir::BasicBlock *block) {
    for (auto *succ : ir::succs_of(block)) {
        if (visited.insert(succ).second) {
            dfs(post_order, visited, succ);
        }
    }
    post_order.push(block);
}

void Decompiler::iterate_reaching_conditions(ir::BasicBlock *block, Vector<ir::Value *> &condition_stack,
                                             Vector<ir::BasicBlock *> &visit_stack) {
    if (std::find(visit_stack.begin(), visit_stack.end(), block) != visit_stack.end()) {
        return;
    }

    visit_stack.push(block);
    for (auto *condition : condition_stack) {
        m_reaching_conditions[block].push(condition);
    }
    if (auto *branch = ir::value_cast<ir::BranchInst>(block->terminator())) {
        if (branch->is_conditional()) {
            condition_stack.push(branch->condition());
            iterate_reaching_conditions(branch->true_target(), condition_stack, visit_stack);
            condition_stack.pop();

            // TODO: Negated.
            condition_stack.push(branch->condition());
            iterate_reaching_conditions(branch->false_target(), condition_stack, visit_stack);
            condition_stack.pop();
        } else {
            iterate_reaching_conditions(branch->target(), condition_stack, visit_stack);
        }
    }
    visit_stack.pop();
}

// Structure an acyclic region.
void Decompiler::structure_region(ir::BasicBlock *entry, ir::BasicBlock *exit) {
    std::unordered_set<ir::BasicBlock *> dfs_visited;
    dfs_visited.insert(exit);

    Vector<ir::BasicBlock *> block_order;
    dfs(block_order, dfs_visited, entry);
    std::reverse(block_order.begin(), block_order.end());

    for (auto *block : block_order) {
        if (m_ast_block_map[block] == nullptr) {
            m_ast_block_map[block] = new AstSequence;
            m_ast_block_map[block]->add_child(new AstComment(codespy::format("{}_actions", block->name)));
        }
    }

    // Build reaching conditions.
    Vector<ir::Value *> condition_stack;
    Vector<ir::BasicBlock *> visit_stack;
    visit_stack.push(exit);
    iterate_reaching_conditions(entry, condition_stack, visit_stack);

    auto *sequence = new AstSequence;
    for (auto *block : block_order) {
        const auto &conditions = m_reaching_conditions[block];
        AstNode *child_sequence = m_ast_block_map.at(block);
        if (!conditions.empty()) {
            child_sequence = new IfStatement(nullptr, child_sequence);
        }
        sequence->add_child(child_sequence);
    }
    m_ast_block_map[entry] = sequence;
}

void Decompiler::print_ast(AstSequence *sequence) {
    for (auto *child : sequence->children()) {
        if (auto *comment = dynamic_cast<AstComment *>(child)) {
            codespy::println("// {}", comment->text());
        } else if (auto *seq = dynamic_cast<AstSequence *>(child)) {
            codespy::println('{');
            print_ast(seq);
            codespy::println('}');
        } else if (auto *if_stmt = dynamic_cast<IfStatement *>(child)) {
            codespy::println("if () {");
            print_ast(dynamic_cast<AstSequence *>(if_stmt->then_body()));
            codespy::println('}');
        }
    }
}

void Decompiler::print_ast(ir::BasicBlock *function_entry) {
    auto *sequence = m_ast_block_map.at(function_entry);
    print_ast(sequence);
}

bool is_region(const ir::DominanceInfo &dom_info, const ir::DominanceInfo &post_dom_info, ir::BasicBlock *entry,
               ir::BasicBlock *exit) {
    Vector<ir::BasicBlock *> work_queue;
    work_queue.push(entry);

    std::unordered_set<ir::BasicBlock *> visited;
    while (!work_queue.empty()) {
        auto *block = work_queue.take_last();
        if (!dom_info.dominates(entry, block)) {
            return false;
        }
        if (!post_dom_info.dominates(exit, block)) {
            return false;
        }

        visited.insert(block);
        for (auto *succ : ir::succs_of(block)) {
            if (!visited.contains(succ)) {
                work_queue.push(succ);
            }
        }
    }

    std::unordered_set<ir::BasicBlock *> region_members;
    region_members.swap(visited);
    region_members.erase(exit);

    for (auto *succ : ir::succs_of(exit)) {
        work_queue.push(succ);
    }

    visited.insert(entry);
    while (!work_queue.empty()) {
        auto *block = work_queue.take_last();
        if (region_members.contains(block)) {
            return false;
        }

        visited.insert(block);
        for (auto *succ : ir::succs_of(block)) {
            if (!visited.contains(succ)) {
                work_queue.push(succ);
            }
        }
    }
    return true;
}

} // namespace

void decompile(ir::Function *function) {
    if (function->blocks().empty()) {
        return;
    }

    Decompiler decompiler;

    auto dom_info = ir::compute_dominance(function);
    auto post_dom_info = ir::compute_post_dominance(function);

    // Construct post order.
    Vector<ir::BasicBlock *> block_order;
    std::unordered_set<ir::BasicBlock *> visited_blocks;
    dfs(block_order, visited_blocks, function->entry_block());

    // Construct regions.
    for (auto *entry : block_order) {
        for (auto *post_dom = entry; post_dom != nullptr;) {
            auto *exit = post_dom_info.idoms().at(post_dom);
            if (post_dom == exit) {
                break;
            }

            if (is_region(dom_info, post_dom_info, entry, exit)) {
                codespy::println("region!! entry={} exit={}", entry->name, exit->name);
                decompiler.structure_region(entry, exit);
            }

            if (!dom_info.dominates(entry, exit)) {
                break;
            }
            post_dom = exit;
        }
    }
    decompiler.print_ast(function->entry_block());
}

} // namespace codespy
