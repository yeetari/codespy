#pragma once

#include <support/Format.hh>
#include <support/StringView.hh>

namespace jamf {

void print(StringView);
void println(StringView);
template <typename... Args>
void print(StringView fmt, Args &&...args) {
    print(jamf::format(fmt, jamf::forward<Args>(args)...));
}
template <typename... Args>
void println(StringView fmt, Args &&...args) {
    println(jamf::format(fmt, jamf::forward<Args>(args)...));
}

} // namespace jamf
