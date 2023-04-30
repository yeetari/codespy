#include <codespy/bytecode/Dumper.hh>

#include <codespy/support/Print.hh>

namespace codespy::bc {
namespace {

void print_prefix_type(BaseType type) {
    switch (type) {
    case BaseType::Int:
        codespy::print('i');
        break;
    case BaseType::Long:
        codespy::print('l');
        break;
    case BaseType::Float:
        codespy::print('f');
        break;
    case BaseType::Double:
        codespy::print('d');
        break;
    case BaseType::Reference:
        codespy::print('a');
        break;
    case BaseType::Void:
        break;
    default:
        assert(false);
    }
}

} // namespace

void Dumper::visit(StringView this_name, StringView super_name) {
    // TODO: Access flags.
    // TODO: Interfaces.
    codespy::println("class {} extends {}", this_name, super_name);
}

void Dumper::visit_field(StringView name, StringView descriptor) {
    codespy::println("field {} {}", name, descriptor);
}

void Dumper::visit_method(AccessFlags, StringView name, StringView descriptor) {
    // TODO: Access flags.
    codespy::println("\nmethod {} {}", name, descriptor);
}

void Dumper::visit_code(std::uint16_t, std::uint16_t) {}

void Dumper::visit_offset(std::int32_t offset) {
    codespy::print("    {d4 }: ", offset);
}

void Dumper::visit_constant(Constant constant) {
    if (constant.has<std::int32_t>()) {
        const auto value = constant.get<std::int32_t>();
        if (value == -1) {
            codespy::println("iconst_m1");
        } else if (value >= 0 && value <= 5) {
            codespy::println("iconst_{}", value);
        } else if (value >= -128 && value <= 127) {
            codespy::println("bipush ${}", value);
        } else if (value >= -32768 && value <= 32767) {
            codespy::println("sipush ${}", value);
        } else {
            codespy::println("ldc ${}", value);
        }
    } else if (constant.has<std::int64_t>()) {
        const auto value = constant.get<std::int64_t>();
        if (value == 0) {
            codespy::println("lconst_0");
        } else if (value == 1) {
            codespy::println("lconst_1");
        } else {
            codespy::println("ldc ${}", value);
        }
    } else if (constant.has<float>()) {
        const auto value = constant.get<float>();
        if (value == 0.0f) {
            codespy::println("fconst_0");
        } else if (value == 1.0f) {
            codespy::println("fconst_1");
        } else if (value == 2.0f) {
            codespy::println("fconst_2");
        } else {
            codespy::println("ldc ${}", value);
        }
    } else if (constant.has<double>()) {
        const auto value = constant.get<double>();
        if (value == 0.0) {
            codespy::println("dconst_0");
        } else if (value == 1.0) {
            codespy::println("dconst_1");
        } else {
            codespy::println("ldc ${}", value);
        }
    } else if (constant.has<StringView>()) {
        codespy::println("ldc \"{}\"", constant.get<StringView>());
    }
}

void Dumper::visit_load(BaseType type, std::uint8_t local_index) {
    print_prefix_type(type);
    codespy::println("load {}", local_index);
}

void Dumper::visit_store(BaseType type, std::uint8_t local_index) {
    print_prefix_type(type);
    codespy::println("store {}", local_index);
}

void Dumper::visit_array_load(BaseType type) {
    print_prefix_type(type);
    codespy::println("aload");
}

void Dumper::visit_array_store(BaseType type) {
    print_prefix_type(type);
    codespy::println("astore");
}

void Dumper::visit_cast(BaseType from_type, BaseType to_type) {
    print_prefix_type(from_type);
    codespy::print('2');
    print_prefix_type(to_type);
    codespy::print('\n');
}

void Dumper::visit_new(StringView descriptor) {
    codespy::println("new {}", descriptor);
}

void Dumper::visit_get_field(StringView owner, StringView name, StringView descriptor, bool instance) {
    const char *kind_string = instance ? "field" : "static";
    codespy::println("get{} {}.{}:{}", kind_string, owner, name, descriptor);
}

void Dumper::visit_invoke(InvokeKind kind, StringView owner, StringView name, StringView descriptor) {
    Array kind_strings{"interface", "special", "static", "virtual"};
    const auto *kind_string = kind_strings[codespy::to_underlying(kind)];
    codespy::println("invoke{} {}.{}:{}", kind_string, owner, name, descriptor);
}

void Dumper::visit_math_op(BaseType type, MathOp math_op) {
    print_prefix_type(type);
    switch (math_op) {
        using enum MathOp;
    case Add:
        codespy::println("add");
        break;
    case Sub:
        codespy::println("sub");
        break;
    case Mul:
        codespy::println("mul");
        break;
    case Div:
        codespy::println("div");
        break;
    case Rem:
        codespy::println("rem");
        break;
    case Neg:
        codespy::println("neg");
        break;
    }
}

void Dumper::visit_stack_op(StackOp stack_op) {
    switch (stack_op) {
        using enum StackOp;
    case Pop:
        codespy::println("pop");
        break;
    case Pop2:
        codespy::println("pop2");
        break;
    case Dup:
        codespy::println("dup");
        break;
    case DupX1:
        codespy::println("dup_x1");
        break;
    case DupX2:
        codespy::println("dup_x2");
        break;
    case Dup2:
        codespy::println("dup2");
        break;
    case Dup2X1:
        codespy::println("dup2_x1");
        break;
    case Dup2X2:
        codespy::println("dup2_x2");
        break;
    case Swap:
        codespy::println("swap");
        break;
    }
}

void Dumper::visit_iinc(std::uint8_t local_index, std::int32_t increment) {
    codespy::println("iinc {}, {}", local_index, increment);
}

void Dumper::visit_goto(std::int32_t offset) {
    codespy::println("goto {}", offset);
}

void Dumper::visit_if_compare(CompareOp compare_op, std::int32_t true_offset, bool with_zero) {
    codespy::print("if");
    if (!with_zero) {
        codespy::print("_icmp");
    }
    switch (compare_op) {
        using enum CompareOp;
    case Equal:
        codespy::println("eq {}", true_offset);
        break;
    case NotEqual:
        codespy::println("ne {}", true_offset);
        break;
    case LessThan:
        codespy::println("lt {}", true_offset);
        break;
    case GreaterEqual:
        codespy::println("ge {}", true_offset);
        break;
    case GreaterThan:
        codespy::println("gt {}", true_offset);
        break;
    case LessEqual:
        codespy::println("le {}", true_offset);
        break;
    }
}

void Dumper::visit_return(BaseType type) {
    print_prefix_type(type);
    codespy::println("return");
}

} // namespace codespy::bc
