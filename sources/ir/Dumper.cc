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
    std::unordered_map<Local *, std::size_t> m_local_map;
    std::unordered_map<Value *, std::size_t> m_value_map;

    String value_string(Value *value);

public:
    void run_on(Function *function);
    void visit(CallInst &) override;
    void visit(LoadInst &) override;
    void visit(LoadFieldInst &) override;
    void visit(ReturnInst &) override;
    void visit(StoreInst &) override;
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
        for (auto *inst : block->insts()) {
            codespy::print("  ");
            if (inst->type() != function->context().void_type()) {
                codespy::print("%v{} = ", m_value_map.size());
                m_value_map.emplace(inst, m_value_map.size());
            }
            inst->accept(*this);
            codespy::print('\n');
        }
    }
    codespy::println("}");
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
        codespy::print("{}", value_string(argument));
    }
    codespy::print(")");
}

void Dumper::visit(LoadInst &load) {
    codespy::print("load {}", value_string(load.pointer()));
}

void Dumper::visit(LoadFieldInst &load_field) {
    codespy::print("load_field {} {}.{}", type_string(load_field.type()), load_field.owner(), load_field.name());
    if (load_field.has_object_ref()) {
        codespy::print("on {}", value_string(load_field.object_ref()));
    }
}

void Dumper::visit(ReturnInst &) {
    codespy::print("ret void");
}

void Dumper::visit(StoreInst &store) {
    codespy::print("store {}, {}", value_string(store.pointer()), value_string(store.value()));
}

} // namespace

void dump_code(Function *function) {
    Dumper dumper;
    dumper.run_on(function);
}

} // namespace codespy::ir
