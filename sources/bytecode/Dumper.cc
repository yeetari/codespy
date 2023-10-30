#include <codespy/bytecode/Dumper.hh>

#include <codespy/bytecode/ClassFile.hh>

namespace codespy::bc {

void Dumper::print_prefix_type(BaseType type) {
    switch (type) {
    case BaseType::Int:
        m_sb.append('i');
        break;
    case BaseType::Long:
        m_sb.append('l');
        break;
    case BaseType::Float:
        m_sb.append('f');
        break;
    case BaseType::Double:
        m_sb.append('d');
        break;
    case BaseType::Reference:
        m_sb.append('a');
        break;
    case BaseType::Byte:
        m_sb.append('b');
        break;
    case BaseType::Char:
        m_sb.append('c');
        break;
    case BaseType::Short:
        m_sb.append('s');
        break;
    case BaseType::Void:
        break;
    default:
        codespy::unreachable();
    }
}

void Dumper::visit(StringView this_name, StringView super_name) {
    // TODO: Access flags.
    // TODO: Interfaces.
    m_this_name = this_name;
    m_sb.append("class {} extends {}\n", this_name, super_name);
}

void Dumper::visit_field(StringView name, StringView descriptor) {
    m_sb.append("field {} {}\n", name, descriptor);
}

void Dumper::visit_method(AccessFlags, StringView name, StringView descriptor) {
    // TODO: Access flags.
    m_sb.append("\nmethod {} {}\n", name, descriptor);
}

void Dumper::visit_code(CodeAttribute &code) {
    for (std::int32_t pc = 0; pc < code.code_end();) {
        m_sb.append("    {d4 }: ", pc);
        pc += CODESPY_EXPECT(code.parse_inst(pc, *this));
    }
}

void Dumper::visit_exception_range(std::int32_t start_pc, std::int32_t end_pc, std::int32_t handler_pc,
                                   StringView type_name) {
    m_sb.append("{}: {} -> {} handled by {}\n", type_name, start_pc, end_pc, handler_pc);
}

void Dumper::visit_constant(Constant constant) {
    if (constant.has<NullReference>()) {
        m_sb.append("aconst_null\n");
    } else if (constant.has<std::int32_t>()) {
        const auto value = constant.get<std::int32_t>();
        if (value == -1) {
            m_sb.append("iconst_m1\n");
        } else if (value >= 0 && value <= 5) {
            m_sb.append("iconst_{}\n", value);
        } else if (value >= -128 && value <= 127) {
            m_sb.append("bipush ${}\n", value);
        } else if (value >= -32768 && value <= 32767) {
            m_sb.append("sipush ${}\n", value);
        } else {
            m_sb.append("ldc ${}\n", value);
        }
    } else if (constant.has<std::int64_t>()) {
        const auto value = constant.get<std::int64_t>();
        if (value == 0) {
            m_sb.append("lconst_0\n");
        } else if (value == 1) {
            m_sb.append("lconst_1\n");
        } else {
            m_sb.append("ldc ${}\n", value);
        }
    } else if (constant.has<float>()) {
        const auto value = constant.get<float>();
        if (value == 0.0f) {
            m_sb.append("fconst_0\n");
        } else if (value == 1.0f) {
            m_sb.append("fconst_1\n");
        } else if (value == 2.0f) {
            m_sb.append("fconst_2\n");
        } else {
            m_sb.append("ldc ${}\n", value);
        }
    } else if (constant.has<double>()) {
        const auto value = constant.get<double>();
        if (value == 0.0) {
            m_sb.append("dconst_0\n");
        } else if (value == 1.0) {
            m_sb.append("dconst_1\n");
        } else {
            m_sb.append("ldc ${}\n", value);
        }
    } else if (constant.has<StringView>()) {
        m_sb.append("ldc \"{}\"\n", constant.get<StringView>());
    }
}

void Dumper::visit_load(BaseType type, std::uint8_t local_index) {
    print_prefix_type(type);
    m_sb.append("load {}\n", local_index);
}

void Dumper::visit_store(BaseType type, std::uint8_t local_index) {
    print_prefix_type(type);
    m_sb.append("store {}\n", local_index);
}

void Dumper::visit_array_load(BaseType type) {
    print_prefix_type(type);
    m_sb.append("aload\n");
}

void Dumper::visit_array_store(BaseType type) {
    print_prefix_type(type);
    m_sb.append("astore\n");
}

void Dumper::visit_cast(BaseType from_type, BaseType to_type) {
    print_prefix_type(from_type);
    m_sb.append('2');
    print_prefix_type(to_type);
    m_sb.append('\n');
}

void Dumper::visit_compare(BaseType type, bool greater_on_nan) {
    print_prefix_type(type);
    m_sb.append("cmp");
    if (type == BaseType::Float || type == BaseType::Double) {
        if (greater_on_nan) {
            m_sb.append('g');
        } else {
            m_sb.append('l');
        }
    }
    m_sb.append('\n');
}

void Dumper::visit_new(StringView descriptor, std::uint8_t dimensions) {
    m_sb.append("new {}", descriptor);
    if (dimensions != 1) {
        m_sb.append(", {}\n", dimensions);
    } else {
        m_sb.append('\n');
    }
}

void Dumper::visit_get_field(StringView owner, StringView name, StringView descriptor, bool instance) {
    const char *kind_string = instance ? "field" : "static";
    m_sb.append("get{} {}.{}:{}\n", kind_string, owner, name, descriptor);
}

void Dumper::visit_put_field(StringView owner, StringView name, StringView descriptor, bool instance) {
    const char *kind_string = instance ? "field" : "static";
    m_sb.append("put{} {}.{}:{}\n", kind_string, owner, name, descriptor);
}

void Dumper::visit_invoke(InvokeKind kind, StringView owner, StringView name, StringView descriptor) {
    Array kind_strings{"interface", "special", "static", "virtual"};
    const auto *kind_string = kind_strings[codespy::to_underlying(kind)];
    m_sb.append("invoke{} {}.{}:{}\n", kind_string, owner, name, descriptor);
}

void Dumper::visit_math_op(BaseType type, MathOp math_op) {
    print_prefix_type(type);
    switch (math_op) {
        using enum MathOp;
    case Add:
        m_sb.append("add\n");
        break;
    case Sub:
        m_sb.append("sub\n");
        break;
    case Mul:
        m_sb.append("mul\n");
        break;
    case Div:
        m_sb.append("div\n");
        break;
    case Rem:
        m_sb.append("rem\n");
        break;
    case Neg:
        m_sb.append("neg\n");
        break;
    case Shl:
        m_sb.append("shl\n");
        break;
    case Shr:
        m_sb.append("shr\n");
        break;
    case UShr:
        m_sb.append("ushr\n");
        break;
    case And:
        m_sb.append("and\n");
        break;
    case Or:
        m_sb.append("or\n");
        break;
    case Xor:
        m_sb.append("xor\n");
        break;
    }
}

void Dumper::visit_monitor_op(MonitorOp monitor_op) {
    switch (monitor_op) {
        using enum MonitorOp;
    case Enter:
        m_sb.append("monitorenter\n");
        break;
    case Exit:
        m_sb.append("monitorexit\n");
        break;
    }
}

void Dumper::visit_reference_op(ReferenceOp reference_op) {
    switch (reference_op) {
        using enum ReferenceOp;
    case ArrayLength:
        m_sb.append("arraylength\n");
        break;
    case Throw:
        m_sb.append("athrow\n");
        break;
    }
}

void Dumper::visit_stack_op(StackOp stack_op) {
    switch (stack_op) {
        using enum StackOp;
    case Pop:
        m_sb.append("pop\n");
        break;
    case Pop2:
        m_sb.append("pop2\n");
        break;
    case Dup:
        m_sb.append("dup\n");
        break;
    case DupX1:
        m_sb.append("dup_x1\n");
        break;
    case DupX2:
        m_sb.append("dup_x2\n");
        break;
    case Dup2:
        m_sb.append("dup2\n");
        break;
    case Dup2X1:
        m_sb.append("dup2_x1\n");
        break;
    case Dup2X2:
        m_sb.append("dup2_x2\n");
        break;
    case Swap:
        m_sb.append("swap\n");
        break;
    }
}

void Dumper::visit_type_op(TypeOp type_op, StringView descriptor) {
    switch (type_op) {
        using enum TypeOp;
    case CheckCast:
        m_sb.append("checkcast {}\n", descriptor);
        break;
    case InstanceOf:
        m_sb.append("instanceof {}\n", descriptor);
        break;
    }
}

void Dumper::visit_iinc(std::uint8_t local_index, std::int32_t increment) {
    m_sb.append("iinc {}, {}\n", local_index, increment);
}

void Dumper::visit_goto(std::int32_t offset) {
    m_sb.append("goto {}\n", offset);
}

void Dumper::visit_if_compare(CompareOp compare_op, std::int32_t true_offset, CompareRhs compare_rhs) {
    m_sb.append("if");
    if (compare_rhs == CompareRhs::Null) {
        if (compare_op == CompareOp::ReferenceNotEqual) {
            m_sb.append("non");
        }
        m_sb.append("null {}\n", true_offset);
        return;
    }

    if (compare_op == CompareOp::ReferenceEqual || compare_op == CompareOp::ReferenceNotEqual) {
        m_sb.append("_acmp");
    } else if (compare_rhs != CompareRhs::Zero) {
        m_sb.append("_icmp");
    }
    switch (compare_op) {
        using enum CompareOp;
    case Equal:
    case ReferenceEqual:
        m_sb.append("eq {}\n", true_offset);
        break;
    case NotEqual:
    case ReferenceNotEqual:
        m_sb.append("ne {}\n", true_offset);
        break;
    case LessThan:
        m_sb.append("lt {}\n", true_offset);
        break;
    case GreaterEqual:
        m_sb.append("ge {}\n", true_offset);
        break;
    case GreaterThan:
        m_sb.append("gt {}\n", true_offset);
        break;
    case LessEqual:
        m_sb.append("le {}\n", true_offset);
        break;
    }
}

void Dumper::visit_table_switch(std::int32_t low, std::int32_t high, std::int32_t default_pc,
                                Span<std::int32_t> table) {
    m_sb.append("tableswitch\n");
    for (std::int32_t i = low; i <= high; i++) {
        m_sb.append("            {}: {}\n", i, table[i - low]);
    }
    m_sb.append("            default: {}\n", default_pc);
}

void Dumper::visit_lookup_switch(std::int32_t default_pc, Span<std::pair<std::int32_t, std::int32_t>> table) {
    m_sb.append("lookupswitch\n");
    for (auto &entry : table) {
        m_sb.append("            {}: {}\n", entry.first, entry.second);
    }
    m_sb.append("            default: {}\n", default_pc);
}

void Dumper::visit_return(BaseType type) {
    print_prefix_type(type);
    m_sb.append("return\n");
}

} // namespace codespy::bc
