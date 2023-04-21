#pragma once

#include <codespy/support/String.hh>
#include <codespy/support/StringBuilder.hh>
#include <codespy/support/StringView.hh>

namespace codespy {

template <typename... Args>
String format(StringView fmt, Args &&...args) {
    StringBuilder builder;
    builder.append(fmt, std::forward<Args>(args)...);
    return builder.build();
}

} // namespace codespy
