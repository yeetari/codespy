#pragma once

#include <codespy/support/Format.hh>
#include <codespy/support/StringView.hh>

#include <utility>

namespace codespy {

void print(StringView);
void println(StringView);

template <typename... Args>
void print(StringView fmt, Args &&...args) {
    print(codespy::format(fmt, std::forward<Args>(args)...));
}

template <typename... Args>
void println(StringView fmt, Args &&...args) {
    println(codespy::format(fmt, std::forward<Args>(args)...));
}

} // namespace codespy
