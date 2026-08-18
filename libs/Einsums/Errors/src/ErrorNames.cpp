//----------------------------------------------------------------------------------------------
// Copyright (c) The Einsums Developers. All rights reserved.
// Licensed under the MIT License. See LICENSE.txt in the project root for license information.
//----------------------------------------------------------------------------------------------

#include <Einsums/Errors/Error.hpp>

#include <fmt/base.h>
#include <fmt/format.h>

#include <cstdio>

namespace einsums::detail {

std::string make_error_message(std::string_view const &type_name, char const *str, std::source_location const &location) {
    std::string out;
    auto        runtime_fmt = fmt::runtime("{}:{}:{}:\nIn {}\n{}: {}");

    auto pass_args = fmt::vargs{{}};

    size_t out_size = fmt::formatted_size(runtime_fmt, location.file_name(), location.line(), location.column(), location.function_name(),
                                          type_name, str);

    out.resize(out_size);

    fmt::format_to(out.begin(), runtime_fmt, location.file_name(), location.line(), location.column(), location.function_name(), type_name,
                   str);
    return out;
}

std::string make_error_message(std::string_view const &type_name, std::string const &str, std::source_location const &location) {
    return make_error_message(type_name, str.c_str(), location);
}
} // namespace einsums::detail