#include <codespy/bytecode/Frontend.hh>

#include <codespy/ir/BasicBlock.hh>
#include <codespy/ir/Constant.hh>
#include <codespy/ir/Context.hh>
#include <codespy/ir/Function.hh>
#include <codespy/ir/Instructions.hh>
#include <codespy/ir/Type.hh>
#include <codespy/support/Format.hh>
#include <codespy/support/StringBuilder.hh>

namespace codespy::bc {

Frontend::Frontend() {
    m_context = codespy::make_unique<ir::Context>();
}

Frontend::~Frontend() = default;

ir::Type *Frontend::lower_base_type(BaseType base_type) {
    switch (base_type) {
    case BaseType::Int:
        return m_context->int_type(32);
    case BaseType::Long:
        return m_context->int_type(64);
    case BaseType::Float:
        return m_context->float_type();
    case BaseType::Double:
        return m_context->double_type();
    case BaseType::Reference:
        return m_context->reference_type("java/lang/Object");
    case BaseType::Void:
        return m_context->void_type();
    }
}

ir::Type *Frontend::parse_type(StringView descriptor, std::size_t *length) {
    assert(!descriptor.empty());
    if (length != nullptr) {
        *length = 1;
    }

    switch (descriptor[0]) {
    case 'B':
        return m_context->int_type(8);
    case 'C':
    case 'S':
        return m_context->int_type(16);
    case 'D':
        return m_context->double_type();
    case 'F':
        return m_context->float_type();
    case 'I':
        return m_context->int_type(32);
    case 'J':
        return m_context->int_type(64);
    case 'V':
        return m_context->void_type();
    case 'Z':
        return m_context->int_type(1);
    case '[': {
        std::size_t sub_length = 0;
        auto *element_type = parse_type(descriptor.substr(1), &sub_length);
        if (length != nullptr) {
            *length = sub_length + 1;
        }
        return m_context->array_type(element_type);
    }
    case 'L': {
        StringBuilder sb;
        for (std::size_t i = 1; i < descriptor.length(); i++) {
            if (descriptor[i] == ';') {
                break;
            }
            sb.append(descriptor[i]);
        }
        if (length != nullptr) {
            *length = sb.length() + 2;
        }
        return m_context->reference_type(sb.build());
    }
    }
    assert(false);
}

ir::FunctionType *Frontend::parse_function_type(StringView descriptor, ir::Type *this_type) {
    assert(!descriptor.empty() && descriptor[0] == '(');
    descriptor = descriptor.substr(1);

    Vector<ir::Type *> parameter_types;
    if (this_type != nullptr) {
        parameter_types.push(this_type);
    }
    while (!descriptor.empty() && descriptor[0] != ')') {
        std::size_t length;
        parameter_types.push(parse_type(descriptor, &length));
        descriptor = descriptor.substr(length);
    }
    descriptor = descriptor.substr(1);

    auto *return_type = parse_type(descriptor);
    return m_context->function_type(return_type, std::move(parameter_types));
}

ir::Function *Frontend::materialise_function(StringView owner, StringView name, StringView descriptor,
                                             ir::Type *this_type) {
    auto full_name = codespy::format("{}.{}", owner, name);
    auto *&slot = m_function_map[full_name];
    if (slot == nullptr) {
        auto *type = parse_function_type(descriptor, this_type);
        slot = new ir::Function(*m_context, std::move(full_name), type);
    }
    return slot;
}

ir::Value *Frontend::materialise_local(std::uint16_t index) {
    auto *&slot = m_local_map[index];
    if (slot == nullptr) {
        // Use any type as we don't know what type(s) the local may hold.
        // TODO: If SSA can be generated from the beginning (in this frontend pass), we wouldn't need to do this.
        slot = m_function->append_local(m_context->any_type());
    }
    return slot;
}

void Frontend::visit(StringView this_name, StringView) {
    m_this_name = this_name;
}

void Frontend::visit_field(StringView, StringView) {}

void Frontend::visit_method(AccessFlags access_flags, StringView name, StringView descriptor) {
    ir::Type *this_type = nullptr;
    if ((access_flags & AccessFlags::Static) != AccessFlags::Static) {
        this_type = m_context->reference_type(m_this_name);
    }
    m_function = materialise_function(m_this_name, name, descriptor, this_type);

    m_block = m_function->append_block();
}

void Frontend::visit_code(std::uint16_t max_stack, std::uint16_t max_locals) {
    m_stack.ensure_capacity(max_stack);
    m_local_map.reserve(max_locals);
}

void Frontend::visit_constant(Constant constant) {
    if (constant.has<std::int32_t>()) {
        m_stack.push(m_context->constant_int(m_context->int_type(32), constant.get<std::int32_t>()));
    } else if (constant.has<std::int64_t>()) {
        m_stack.push(m_context->constant_int(m_context->int_type(64), constant.get<std::int64_t>()));
    } else if (constant.has<StringView>()) {
        m_stack.push(m_context->constant_string(constant.get<StringView>()));
    } else {
        assert(false);
    }
}

void Frontend::visit_load(BaseType base_type, std::uint8_t local_index) {
    if (local_index < m_function->parameter_count()) {
        m_stack.push(m_function->argument(local_index));
        return;
    }

    ir::Type *type = lower_base_type(base_type);
    m_stack.push(m_block->append<ir::LoadInst>(type, materialise_local(local_index)));
}

void Frontend::visit_store(BaseType, std::uint8_t local_index) {
    if (local_index < m_function->parameter_count()) {
        // TODO: Handle assigns to arguments.
        assert(false);
    }

    ir::Value *value = m_stack.take_last();
    m_block->append<ir::StoreInst>(materialise_local(local_index), value);
}

void Frontend::visit_cast(BaseType, BaseType) {
    assert(false);
}

void Frontend::visit_get_field(StringView owner, StringView name, StringView descriptor, bool instance) {
    ir::Value *object_ref = nullptr;
    if (instance) {
        object_ref = m_stack.take_last();
    }
    m_stack.push(m_block->append<ir::LoadFieldInst>(parse_type(descriptor), owner, name, object_ref));
}

void Frontend::visit_invoke(InvokeKind kind, StringView owner, StringView name, StringView descriptor) {
    ir::Type *this_type = nullptr;
    if (kind != InvokeKind::Static) {
        this_type = m_context->reference_type(owner);
    }
    auto *function = materialise_function(owner, name, descriptor, this_type);

    Vector<ir::Value *> arguments(function->function_type()->parameter_types().size());
    for (std::uint32_t i = arguments.size(); i > 0; i--) {
        arguments[i - 1] = m_stack.take_last();
    }

    auto *call = m_block->append<ir::CallInst>(function, arguments.span());
    call->set_is_invoke_special(kind == InvokeKind::Special);
    if (call->type() != m_context->void_type()) {
        m_stack.push(call);
    }
}

void Frontend::visit_stack_op(StackOp stack_op) {
    switch (stack_op) {
    case StackOp::Pop2:
        m_stack.pop();
        if (m_stack.empty()) {
            break;
        }
        [[fallthrough]];
    case StackOp::Pop:
        m_stack.pop();
        break;
    default:
        assert(false);
    }
}

void Frontend::visit_if_cmp(CompareOp, std::int32_t, bool) {
    assert(false);
}

void Frontend::visit_return(BaseType type) {
    switch (type) {
    case BaseType::Void:
        m_block->append<ir::ReturnInst>();
        break;
    default:
        assert(false);
    }
}

Vector<ir::Function *> Frontend::functions() {
    Vector<ir::Function *> functions;
    functions.ensure_capacity(m_function_map.size());
    for (const auto &[name, function] : m_function_map) {
        functions.push(function);
    }
    return functions;
}

} // namespace codespy::bc
