#include <support/Print.hh>

#include <stdio.h>

namespace jamf {

void print(StringView line) {
    fwrite(line.data(), 1, line.length(), stdout);
}

void println(StringView line) {
    print(line);
    fputc('\n', stdout);
}

} // namespace jamf
