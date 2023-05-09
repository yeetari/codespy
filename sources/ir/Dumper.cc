#include <codespy/ir/Dumper.hh>

#include <codespy/ir/BasicBlock.hh>
#include <codespy/ir/Constant.hh>
#include <codespy/ir/Context.hh>
#include <codespy/ir/Function.hh>
#include <codespy/ir/Instructions.hh>
#include <codespy/ir/Type.hh>
#include <codespy/ir/Visitor.hh>
#include <codespy/support/Format.hh>
#include <codespy/support/String.hh>
#include <codespy/support/StringBuilder.hh>

#include <unordered_map>

namespace codespy::ir {
namespace {

class Dumper final : public Visitor {
    Function *m_function;
    StringBuilder m_sb;
    std::unordered_map<BasicBlock *, std::size_t> m_block_map;
    std::unordered_map<Local *, std::size_t> m_local_map;
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
    if (auto *argument = value->as<Argument>()) {
        auto it = std::find_if(m_function->arguments().begin(), m_function->arguments().end(),
                               [argument](const Argument *arg) {
                                   return arg == argument;
                               });
        auto index = std::distance(m_function->arguments().begin(), it);
        return codespy::format("{} %a{}", type_string(argument->type()), index);
    }
    if (auto *block = value->as<BasicBlock>()) {
        if (!m_block_map.contains(block)) {
            m_block_map.emplace(block, m_block_map.size());
        }
        return codespy::format("L{}", m_block_map.at(block));
    }
    if (value->kind() == ValueKind::ConstantNull) {
        return codespy::format("{} null", type_string(value->type()));
    }
    if (auto *constant = value->as<ConstantDouble>()) {
        return codespy::format("{} ${}", type_string(constant->type()), constant->value());
    }
    if (auto *constant = value->as<ConstantFloat>()) {
        return codespy::format("{} ${}", type_string(constant->type()), constant->value());
    }
    if (auto *constant = value->as<ConstantInt>()) {
        return codespy::format("{} ${}", type_string(constant->type()), constant->value());
    }
    if (auto *constant = value->as<ConstantString>()) {
        return codespy::format("{} \"{}\"", type_string(constant->type()), constant->value());
    }
    if (auto *function = value->as<Function>()) {
        return codespy::format("{} @{}", type_string(function->function_type()->return_type()),
                               function->display_name());
    }
    if (auto *local = value->as<Local>()) {
        return codespy::format("{} %l{}", type_string(local->type()), m_local_map.at(local));
    }
    return codespy::format("{} %v{}", type_string(value->type()), m_value_map.at(value));
}

void Dumper::run_on(Function *function) {
    m_function = function;
    m_sb.append("{} @{}(", type_string(function->function_type()->return_type()), function->display_name());
    for (std::size_t i = 0; auto *argument : function->arguments()) {
        if (i != 0) {
            m_sb.append(", ");
        }
        m_sb.append("%a{}: {}", i, type_string(argument->type()));
        i++;
    }

    if (function->blocks().empty()) {
        m_sb.append(");\n");
        return;
    }

    m_sb.append(") {\n");
    for (auto *local : function->locals()) {
        m_sb.append("  %l{}: {}\n", m_local_map.size(), type_string(local->type()));
        m_local_map.emplace(local, m_local_map.size());
    }
    for (auto *block : function->blocks()) {
        m_sb.append("  {}", value_string(block));
        m_sb.append(" {\n");
        for (auto *inst : block->insts()) {
            m_sb.append("    ");
            if (inst->type() != function->context().void_type()) {
                m_sb.append("%v{} = ", m_value_map.size());
                m_value_map.emplace(inst, m_value_map.size());
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
    m_sb.append("load_field {} {}.{}", type_string(load_field.type()), load_field.owner(), load_field.name());
    if (load_field.has_object_ref()) {
        m_sb.append(", on {}", value_string(load_field.object_ref()));
    }
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

void Dumper::visit(PhiInst &) {
    assert(false);
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
    m_sb.append("store_field {}.{}, {}", store_field.owner(), store_field.name(), value_string(store_field.value()));
    if (store_field.has_object_ref()) {
        m_sb.append(", on {}", value_string(store_field.object_ref()));
    }
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
