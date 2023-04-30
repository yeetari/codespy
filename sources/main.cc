#include <codespy/bytecode/ClassFile.hh>
#include <codespy/bytecode/Dumper.hh>
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
                bc::Dumper dumper;
                CODESPY_EXPECT(bc::parse_class(stream, dumper));
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
