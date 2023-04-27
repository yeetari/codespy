#include <codespy/bytecode/ClassFile.hh>
#include <codespy/bytecode/Frontend.hh>
#include <codespy/bytecode/Visitor.hh>
#include <codespy/container/Vector.hh>
#include <codespy/ir/Dumper.hh>
#include <codespy/support/Enum.hh>
#include <codespy/support/Print.hh>
#include <codespy/support/SpanStream.hh>

#include <fstream>
#include <iostream>
#include <miniz/miniz.h>

using namespace codespy;

struct DumpVisitor : public bc::Visitor {
    void visit(StringView this_name, StringView super_name) override {
        codespy::println("{} extends {}", this_name, super_name);
    }

    void visit_field(StringView name, StringView descriptor) override {
        codespy::println("field {} {}", name, descriptor);
    }

    void visit_method(bc::AccessFlags, StringView name, StringView descriptor) override {
        codespy::println("method {} {}", name, descriptor);
    }

    void visit_code(std::uint16_t, std::uint16_t) override {}

    void visit_constant(bc::Constant constant) override {
        if (constant.has<std::int32_t>()) {
            codespy::println("    ldc ${}", constant.get<std::int32_t>());
        } else if (constant.has<std::int64_t>()) {
            codespy::println("    ldc ${}", constant.get<std::int64_t>());
        } else if (constant.has<float>()) {
            codespy::println("    ldc ${}", constant.get<float>());
        } else if (constant.has<double>()) {
            codespy::println("    ldc ${}", constant.get<double>());
        } else if (constant.has<StringView>()) {
            codespy::println("    ldc \"{}\"", constant.get<StringView>());
        }
    }

    void visit_load(bc::BaseType type, std::uint8_t local_index) override {
        codespy::println("    load {} ({})", local_index, codespy::enum_name<1>(type));
    }

    void visit_store(bc::BaseType type, std::uint8_t local_index) override {
        codespy::println("    store {} ({})", local_index, codespy::enum_name<1>(type));
    }

    void visit_cast(bc::BaseType from_type, bc::BaseType to_type) override {
        codespy::println("    cast {} -> {}", codespy::enum_name<1>(from_type), codespy::enum_name<1>(to_type));
    }

    void visit_get_field(StringView owner, StringView name, StringView descriptor, bool) override {
        codespy::println("    getfield {}.{}:{}", owner, name, descriptor);
    }

    void visit_invoke(bc::InvokeKind kind, StringView owner, StringView name, StringView descriptor) override {
        const auto *kind_string = kind == bc::InvokeKind::Special ? "special" : "virtual";
        codespy::println("    invoke{} {}.{}:{}", kind_string, owner, name, descriptor);
    }

    void visit_stack_op(bc::StackOp stack_op) override {
        switch (stack_op) {
            using enum bc::StackOp;
        case Pop:
            codespy::println("    pop");
            break;
        case Pop2:
            codespy::println("    pop2");
            break;
        case Dup:
            codespy::println("    dup");
            break;
        case DupX1:
            codespy::println("    dup_x1");
            break;
        case DupX2:
            codespy::println("    dup_x2");
            break;
        case Dup2:
            codespy::println("    dup2");
            break;
        case Dup2X1:
            codespy::println("    dup2_x1");
            break;
        case Dup2X2:
            codespy::println("    dup2_x2");
            break;
        case Swap:
            codespy::println("    swap");
            break;
        }
    }

    void visit_return(bc::BaseType type) override { codespy::println("    return ({})", codespy::enum_name<1>(type)); }
};

int main(int, char **argv) {
    mz_zip_archive zip_archive{};
    mz_zip_reader_init_file(&zip_archive, argv[1], 0);
    const auto zip_entry_count = mz_zip_reader_get_num_files(&zip_archive);
    for (mz_uint i = 0; i < zip_entry_count; i++) {
        Array<char, 256> name_chars{};
        mz_zip_reader_get_filename(&zip_archive, i, name_chars.data(), name_chars.size());
        String name(name_chars.data());
        if (name.ends_with(".class")) {
            std::size_t size;
            void *data = mz_zip_reader_extract_to_heap(&zip_archive, i, &size, 0);
            {
                SpanStream stream(codespy::make_span(static_cast<std::uint8_t *>(data), size));
                DumpVisitor visitor;
                CODESPY_EXPECT(bc::parse_class(stream, visitor));
            }
            codespy::print('\n');
            {
                SpanStream stream(codespy::make_span(static_cast<std::uint8_t *>(data), size));
                bc::Frontend frontend;
                CODESPY_EXPECT(bc::parse_class(stream, frontend));
                for (auto *function : frontend.functions()) {
                    ir::dump_code(function);
                }
            }
        }
    }
    mz_zip_reader_end(&zip_archive);
}
