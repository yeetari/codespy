#include <codespy/support/Print.hh>

#include <cstdio>

namespace codespy {

void print(StringView line) {
    std::fwrite(line.data(), 1, line.length(), stdout);
}

void println(StringView line) {
    print(line);
    std::fputc('\n', stdout);
}

} // namespace codespy
