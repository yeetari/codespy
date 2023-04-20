#pragma once

#include <support/String.hh>
#include <support/StringBuilder.hh>
#include <support/StringView.hh>

namespace jamf {

template <typename... Args>
String format(StringView fmt, Args &&...args) {
    StringBuilder builder;
    builder.append(fmt, forward<Args>(args)...);
    return builder.build();
}

} // namespace jamf
