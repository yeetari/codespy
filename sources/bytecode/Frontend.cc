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

void Frontend::visit_code(std::uint16_t max_stack, std::uint16_t) {
    m_stack.ensure_capacity(max_stack);
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

void Frontend::visit_load(BaseType, std::uint8_t local_index) {
    const auto parameter_count = m_function->function_type()->parameter_types().size();
    if (local_index < parameter_count) {
        m_stack.push(m_function->argument(local_index));
        return;
    }
    assert(false);
}

void Frontend::visit_store(BaseType, std::uint8_t) {
    m_stack.pop();
    //    assert(false);
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
