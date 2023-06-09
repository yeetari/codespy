#include <codespy/ir/Dumper.hh>

#include <codespy/container/Vector.hh>
#include <codespy/ir/BasicBlock.hh>
#include <codespy/ir/Cfg.hh>
#include <codespy/ir/Constant.hh>
#include <codespy/ir/Context.hh>
#include <codespy/ir/Function.hh>
#include <codespy/ir/Instructions.hh>
#include <codespy/ir/Java.hh>
#include <codespy/ir/Type.hh>
#include <codespy/ir/Visitor.hh>
#include <codespy/support/Format.hh>
#include <codespy/support/String.hh>
#include <codespy/support/StringBuilder.hh>

#include <unordered_map>
#include <unordered_set>

namespace codespy::ir {
namespace {

class Dumper final : public Visitor {
    StringBuilder m_sb;
    std::unordered_map<BasicBlock *, std::size_t> m_block_map;
    std::unordered_map<Value *, std::size_t> m_value_map;

    String value_string(Value *value);

public:
    void run_on(Function *function);
    String build() { return m_sb.build(); }

#define INST(opcode, Class) void visit(Class &) override;
#include <codespy/ir/Instructions.in>
};

String type_string(Type *type) {
    // TODO: as<T> function on Type.
    switch (type->kind()) {
    case TypeKind::Any:
        return "any";
    case TypeKind::Float:
        return "f32";
    case TypeKind::Double:
        return "f64";
    case TypeKind::Void:
        return "void";
    case TypeKind::Array:
        return codespy::format("{}[]", type_string(static_cast<ArrayType *>(type)->element_type()));
    case TypeKind::Integer:
        return codespy::format("i{}", static_cast<IntType *>(type)->bit_width());
    case TypeKind::Reference:
        return codespy::format("#{}", static_cast<ReferenceType *>(type)->class_name());
    default:
        assert(false);
    }
}

String Dumper::value_string(Value *value) {
    if (auto *argument = value_cast<Argument>(value)) {
        return codespy::format("{} %a{}", type_string(argument->type()), argument->index());
    }
    if (auto *block = value_cast<BasicBlock>(value)) {
        return block->name;
        return codespy::format("L{}", m_block_map.at(block));
    }
    if (value->kind() == ValueKind::ConstantNull) {
        return codespy::format("{} null", type_string(value->type()));
    }
    if (value->kind() == ValueKind::Poison) {
        return codespy::format("{} poison", type_string(value->type()));
    }
    if (auto *constant = value_cast<ConstantDouble>(value)) {
        return codespy::format("{} ${}", type_string(constant->type()), constant->value());
    }
    if (auto *constant = value_cast<ConstantFloat>(value)) {
        return codespy::format("{} ${}", type_string(constant->type()), constant->value());
    }
    if (auto *constant = value_cast<ConstantInt>(value)) {
        return codespy::format("{} ${}", type_string(constant->type()), constant->value());
    }
    if (auto *constant = value_cast<ConstantString>(value)) {
        return codespy::format("{} \"{}\"", type_string(constant->type()), constant->value());
    }
    if (auto *function = value_cast<Function>(value)) {
        return codespy::format("{} @{}", type_string(function->function_type()->return_type()),
                               function->display_name());
    }
    if (auto *field = value_cast<JavaField>(value)) {
        return codespy::format("{} {}.{}", type_string(field->type()), field->parent()->name(), field->name());
    }
    if (auto *local = value_cast<Local>(value)) {
        return codespy::format("{} %l{}", type_string(local->type()), local->index());
    }
    return codespy::format("{} %v{}", type_string(value->type()), m_value_map.at(value));
}

void dfs(Vector<BasicBlock *> &post_order, std::unordered_set<BasicBlock *> &visited, BasicBlock *block) {
    for (auto *succ : ir::succs_of(block)) {
        if (visited.insert(succ).second) {
            dfs(post_order, visited, succ);
        }
    }
    post_order.push(block);
}

void Dumper::run_on(Function *function) {
    m_sb.append("{} @{}(", type_string(function->function_type()->return_type()), function->display_name());
    for (auto *argument : function->arguments()) {
        if (argument->index() != 0) {
            m_sb.append(", ");
        }
        m_sb.append("%a{}: {}", argument->index(), type_string(argument->type()));
    }

    if (function->blocks().empty()) {
        m_sb.append(");\n");
        return;
    }

    // Construct reverse post order.
    Vector<BasicBlock *> block_order;
    std::unordered_set<BasicBlock *> visited_blocks;
    dfs(block_order, visited_blocks, function->entry_block());
    std::reverse(block_order.begin(), block_order.end());

    // Materialise unique value identifiers now.
    for (auto *block : block_order) {
        m_block_map.emplace(block, m_block_map.size());
        for (auto *inst : *block) {
            if (inst->type() != function->context().void_type()) {
                m_value_map.emplace(inst, m_value_map.size());
            }
        }
    }

    m_sb.append(") {\n");
    for (auto *local : function->locals()) {
        m_sb.append("  %l{}: {}\n", local->index(), type_string(local->type()));
    }
    for (auto *block : block_order) {
        m_sb.append("  {}", value_string(block));
        m_sb.append(" {\n");
        for (auto *handler : block->handlers()) {
            m_sb.append("    @handler {} -> L{}\n", type_string(handler->type()), m_block_map.at(handler->target()));
        }
        for (auto *inst : *block) {
            m_sb.append("    ");
            if (inst->type() != function->context().void_type()) {
                m_sb.append("%v{} = ", m_value_map.at(inst));
            }
            inst->accept(*this);
            m_sb.append('\n');
        }
        m_sb.append("  }\n");
    }
    m_sb.append("}\n");
}

void Dumper::visit(ArrayLengthInst &array_length) {
    m_sb.append("array_length {}", value_string(array_length.array_ref()));
}

void Dumper::visit(BinaryInst &binary) {
    switch (binary.op()) {
    case BinaryOp::Add:
        m_sb.append("add ");
        break;
    case BinaryOp::Sub:
        m_sb.append("sub ");
        break;
    case BinaryOp::Mul:
        m_sb.append("mul ");
        break;
    case BinaryOp::Div:
        m_sb.append("div ");
        break;
    case BinaryOp::Rem:
        m_sb.append("rem ");
        break;
    case BinaryOp::Shl:
        m_sb.append("shl ");
        break;
    case BinaryOp::Shr:
        m_sb.append("shr ");
        break;
    case BinaryOp::UShr:
        m_sb.append("ushr ");
        break;
    case BinaryOp::And:
        m_sb.append("and ");
        break;
    case BinaryOp::Or:
        m_sb.append("or ");
        break;
    case BinaryOp::Xor:
        m_sb.append("xor ");
        break;
    }
    m_sb.append("{}, {}", value_string(binary.lhs()), value_string(binary.rhs()));
}

void Dumper::visit(BranchInst &branch) {
    if (branch.is_conditional()) {
        m_sb.append("br {}, {}, {}", value_string(branch.condition()), value_string(branch.true_target()),
                    value_string(branch.false_target()));
    } else {
        m_sb.append("br {}", value_string(branch.target()));
    }
}

void Dumper::visit(CallInst &call) {
    m_sb.append("call ");
    if (call.is_invoke_special()) {
        m_sb.append("special ");
    }
    m_sb.append("{}(", value_string(call.callee()));
    for (bool first = true; auto *argument : call.arguments()) {
        if (!first) {
            m_sb.append(", ");
        }
        first = false;
        m_sb.append(value_string(argument));
    }
    m_sb.append(')');
}

void Dumper::visit(CastInst &cast) {
    m_sb.append("cast {} to {}", value_string(cast.value()), type_string(cast.type()));
}

void Dumper::visit(CatchInst &catch_inst) {
    m_sb.append("catch {}", type_string(catch_inst.type()));
}

void Dumper::visit(CompareInst &compare) {
    switch (compare.op()) {
    case CompareOp::Equal:
        m_sb.append("cmp_eq ");
        break;
    case CompareOp::NotEqual:
        m_sb.append("cmp_ne ");
        break;
    case CompareOp::LessThan:
        m_sb.append("cmp_lt ");
        break;
    case CompareOp::GreaterThan:
        m_sb.append("cmp_gt ");
        break;
    case CompareOp::LessEqual:
        m_sb.append("cmp_le ");
        break;
    case CompareOp::GreaterEqual:
        m_sb.append("cmp_ge ");
        break;
    }
    m_sb.append("{}, {}", value_string(compare.lhs()), value_string(compare.rhs()));
}

void Dumper::visit(ExceptionHandler &) {
    assert(false);
}

void Dumper::visit(InstanceOfInst &instance_of) {
    m_sb.append("instance_of {}, {}", value_string(instance_of.value()), type_string(instance_of.check_type()));
}

void Dumper::visit(JavaCompareInst &java_compare) {
    const auto type_kind = java_compare.operand_type()->kind();
    switch (type_kind) {
    case TypeKind::Integer:
        assert(static_cast<IntType *>(java_compare.operand_type())->bit_width() == 64);
        m_sb.append('l');
        break;
    case TypeKind::Float:
        m_sb.append('f');
        break;
    case TypeKind::Double:
        m_sb.append('d');
        break;
    default:
        assert(false);
    }

    m_sb.append("cmp");
    if (type_kind == TypeKind::Float || type_kind == TypeKind::Double) {
        if (java_compare.greater_on_nan()) {
            m_sb.append('g');
        } else {
            m_sb.append('l');
        }
    }
    m_sb.append(" {}, {}", value_string(java_compare.lhs()), value_string(java_compare.rhs()));
}

void Dumper::visit(LoadInst &load) {
    m_sb.append("load {}", value_string(load.pointer()));
}

void Dumper::visit(LoadArrayInst &load_array) {
    m_sb.append("load_array {}[{}]", value_string(load_array.array_ref()), value_string(load_array.index()));
}

void Dumper::visit(LoadFieldInst &load_field) {
    m_sb.append("load_field {}, {}", value_string(load_field.object_ref()), value_string(load_field.field()));
}

void Dumper::visit(MonitorInst &monitor) {
    if (monitor.op() == MonitorOp::Enter) {
        m_sb.append("monitor_enter {}", value_string(monitor.object_ref()));
    } else {
        m_sb.append("monitor_exit {}", value_string(monitor.object_ref()));
    }
}

void Dumper::visit(NegateInst &negate) {
    m_sb.append("neg {}", value_string(negate.value()));
}

void Dumper::visit(NewInst &new_inst) {
    m_sb.append("new {}", type_string(new_inst.type()));
}

void Dumper::visit(NewArrayInst &new_array) {
    m_sb.append("new_array {} [", type_string(new_array.type()));
    for (std::uint8_t i = 0; i < new_array.dimensions(); i++) {
        if (i != 0) {
            m_sb.append(", ");
        }
        m_sb.append(value_string(new_array.count(i)));
    }
    m_sb.append(']');
}

void Dumper::visit(PhiInst &phi) {
    m_sb.append("phi (");
    for (unsigned i = 0; i < phi.incoming_count(); i++) {
        if (i != 0) {
            m_sb.append(", ");
        }
        m_sb.append("{}: {}", value_string(phi.incoming_block(i)), value_string(phi.incoming_value(i)));
    }
    m_sb.append(')');
}

void Dumper::visit(ReturnInst &return_inst) {
    if (return_inst.is_void()) {
        m_sb.append("ret void");
        return;
    }
    m_sb.append("ret {}", value_string(return_inst.value()));
}

void Dumper::visit(StoreInst &store) {
    m_sb.append("store {}, {}", value_string(store.pointer()), value_string(store.value()));
}

void Dumper::visit(StoreArrayInst &store_array) {
    m_sb.append("store_array {}[{}], {}", value_string(store_array.array_ref()), value_string(store_array.index()),
                value_string(store_array.value()));
}

void Dumper::visit(StoreFieldInst &store_field) {
    m_sb.append("store_field {}, {}, {}", value_string(store_field.object_ref()), value_string(store_field.field()),
                value_string(store_field.value()));
}

void Dumper::visit(SwitchInst &switch_inst) {
    m_sb.append("switch {}, {}, [\n", value_string(switch_inst.value()), value_string(switch_inst.default_target()));
    for (unsigned i = 0; i < switch_inst.case_count(); i++) {
        m_sb.append("      {}, {}\n", value_string(switch_inst.case_value(i)),
                    value_string(switch_inst.case_target(i)));
    }
    m_sb.append("    ]");
}

void Dumper::visit(ThrowInst &throw_inst) {
    m_sb.append("throw {}", value_string(throw_inst.exception_ref()));
}

} // namespace

String dump_code(Function *function) {
    Dumper dumper;
    dumper.run_on(function);
    return dumper.build();
}

} // namespace codespy::ir
