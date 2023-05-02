#include <codespy/ir/Dumper.hh>

#include <codespy/ir/BasicBlock.hh>
#include <codespy/ir/Constant.hh>
#include <codespy/ir/Context.hh>
#include <codespy/ir/Function.hh>
#include <codespy/ir/Instructions.hh>
#include <codespy/ir/Type.hh>
#include <codespy/ir/Visitor.hh>
#include <codespy/support/Print.hh>

#include <unordered_map>

namespace codespy::ir {
namespace {

class Dumper final : public Visitor {
    Function *m_function;
    std::unordered_map<BasicBlock *, std::size_t> m_block_map;
    std::unordered_map<Local *, std::size_t> m_local_map;
    std::unordered_map<Value *, std::size_t> m_value_map;

    String value_string(Value *value);

public:
    void run_on(Function *function);
    void visit(BinaryInst &) override;
    void visit(BranchInst &) override;
    void visit(CallInst &) override;
    void visit(CompareInst &) override;
    void visit(LoadInst &) override;
    void visit(LoadArrayInst &) override;
    void visit(LoadFieldInst &) override;
    void visit(NewInst &) override;
    void visit(NewArrayInst &) override;
    void visit(PhiInst &) override;
    void visit(ReturnInst &) override;
    void visit(StoreInst &) override;
    void visit(StoreArrayInst &) override;
    void visit(SwitchInst &) override;
    void visit(ThrowInst &) override;
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
        return static_cast<ReferenceType *>(type)->class_name();
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
    if (auto *constant = value->as<ConstantInt>()) {
        return codespy::format("{} ${}", type_string(constant->type()), constant->value());
    }
    if (auto *constant = value->as<ConstantString>()) {
        return codespy::format("{} \"{}\"", type_string(constant->type()), constant->value());
    }
    if (auto *function = value->as<Function>()) {
        return codespy::format("{} @{}", type_string(function->function_type()->return_type()), function->name());
    }
    if (auto *local = value->as<Local>()) {
        return codespy::format("{} %l{}", type_string(local->type()), m_local_map.at(local));
    }
    return codespy::format("{} %v{}", type_string(value->type()), m_value_map.at(value));
}

void Dumper::run_on(Function *function) {
    m_function = function;
    codespy::print("\n{} @{}(", type_string(function->function_type()->return_type()), function->name());
    for (std::size_t i = 0; auto *argument : function->arguments()) {
        if (i != 0) {
            codespy::print(", ");
        }
        codespy::print("%a{}: {}", i, type_string(argument->type()));
        i++;
    }

    if (function->blocks().empty()) {
        codespy::println(");");
        return;
    }

    codespy::println(") {");
    for (auto *local : function->locals()) {
        codespy::println("  %l{}: {}", m_local_map.size(), type_string(local->type()));
        m_local_map.emplace(local, m_local_map.size());
    }
    for (auto *block : function->blocks()) {
        codespy::print("  {}", value_string(block));
        codespy::println(" {");
        for (auto *inst : block->insts()) {
            codespy::print("    ");
            if (inst->type() != function->context().void_type()) {
                codespy::print("%v{} = ", m_value_map.size());
                m_value_map.emplace(inst, m_value_map.size());
            }
            inst->accept(*this);
            codespy::print('\n');
        }
        codespy::println("  }");
    }
    codespy::println("}");
}

void Dumper::visit(BinaryInst &binary) {
    switch (binary.op()) {
    case BinaryOp::Add:
        codespy::print("add ");
        break;
    case BinaryOp::Sub:
        codespy::print("sub ");
        break;
    case BinaryOp::Mul:
        codespy::print("mul ");
        break;
    case BinaryOp::Div:
        codespy::print("div ");
        break;
    case BinaryOp::Rem:
        codespy::print("rem ");
        break;
    }
    codespy::print("{}, {}", value_string(binary.lhs()), value_string(binary.rhs()));
}

void Dumper::visit(BranchInst &branch) {
    if (branch.is_conditional()) {
        codespy::print("br {}, {}, {}", value_string(branch.condition()), value_string(branch.true_target()),
                       value_string(branch.false_target()));
    } else {
        codespy::print("br {}", value_string(branch.target()));
    }
}

void Dumper::visit(CallInst &call) {
    codespy::print("call ");
    if (call.is_invoke_special()) {
        codespy::print("special ");
    }
    codespy::print("{}(", value_string(call.callee()));
    for (bool first = true; auto *argument : call.arguments()) {
        if (!first) {
            codespy::print(", ");
        }
        first = false;
        codespy::print(value_string(argument));
    }
    codespy::print(")");
}

void Dumper::visit(CompareInst &compare) {
    switch (compare.op()) {
    case CompareOp::Equal:
        codespy::print("cmp_eq ");
        break;
    case CompareOp::NotEqual:
        codespy::print("cmp_ne ");
        break;
    case CompareOp::LessThan:
        codespy::print("cmp_lt ");
        break;
    case CompareOp::GreaterThan:
        codespy::print("cmp_gt ");
        break;
    case CompareOp::LessEqual:
        codespy::print("cmp_le ");
        break;
    case CompareOp::GreaterEqual:
        codespy::print("cmp_ge ");
        break;
    }
    codespy::print("{}, {}", value_string(compare.lhs()), value_string(compare.rhs()));
}

void Dumper::visit(LoadInst &load) {
    codespy::print("load {}", value_string(load.pointer()));
}

void Dumper::visit(LoadArrayInst &load_array) {
    codespy::print("load_array {}[{}]", value_string(load_array.array_ref()), value_string(load_array.index()));
}

void Dumper::visit(LoadFieldInst &load_field) {
    codespy::print("load_field {} {}.{}", type_string(load_field.type()), load_field.owner(), load_field.name());
    if (load_field.has_object_ref()) {
        codespy::print("on {}", value_string(load_field.object_ref()));
    }
}

void Dumper::visit(NewInst &new_inst) {
    codespy::print("new {}", type_string(new_inst.type()));
}

void Dumper::visit(NewArrayInst &new_array) {
    codespy::print("new_array {} [", type_string(new_array.type()));
    for (std::uint8_t i = 0; i < new_array.dimensions(); i++) {
        if (i != 0) {
            codespy::print(", ");
        }
        codespy::print(value_string(new_array.count(i)));
    }
    codespy::print(']');
}

void Dumper::visit(PhiInst &) {
    assert(false);
}

void Dumper::visit(ReturnInst &return_inst) {
    if (return_inst.is_void()) {
        codespy::print("ret void");
        return;
    }
    codespy::print("ret {}", value_string(return_inst.value()));
}

void Dumper::visit(StoreInst &store) {
    codespy::print("store {}, {}", value_string(store.pointer()), value_string(store.value()));
}

void Dumper::visit(StoreArrayInst &store_array) {
    codespy::print("store_array {}[{}], {}", value_string(store_array.array_ref()), value_string(store_array.index()),
                   value_string(store_array.value()));
}

void Dumper::visit(SwitchInst &switch_inst) {
    codespy::println("switch {}, {}, [", value_string(switch_inst.value()), value_string(switch_inst.default_target()));
    for (unsigned i = 0; i < switch_inst.case_count(); i++) {
        codespy::println("      {}, {}", value_string(switch_inst.case_value(i)),
                         value_string(switch_inst.case_target(i)));
    }
    codespy::print("    ]");
}

void Dumper::visit(ThrowInst &throw_inst) {
    codespy::print("throw {}", value_string(throw_inst.exception_ref()));
}

} // namespace

void dump_code(Function *function) {
    Dumper dumper;
    dumper.run_on(function);
}

} // namespace codespy::ir
