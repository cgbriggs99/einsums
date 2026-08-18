//----------------------------------------------------------------------------------------------
// Copyright (c) The Einsums Developers. All rights reserved.
// Licensed under the MIT License. See LICENSE.txt in the project root for license information.
//----------------------------------------------------------------------------------------------

#pragma once

#include <Einsums/Errors/Error.hpp>
#include <Einsums/TypeSupport/TypeName.hpp>

#include <fmt/base.h>
#include <fmt/format.h>

#include <exception>
#include <source_location>
#include <string>
#include <system_error>

namespace einsums {
namespace errors {
namespace detail {
template <typename... Args>
std::string corrected_format(std::string_view const &format, Args &&...args) {
    std::string out;

    auto runtime_format = fmt::runtime(format);

    size_t out_size = fmt::formatted_size(runtime_format, std::forward<Args>(args)...);

    out.resize(out_size);

    fmt::format_to(out.begin(), runtime_format, std::forward<Args>(args)...);

    return out;
}
} // namespace detail
} // namespace errors
} // namespace einsums

#define EINSUMS_THROW_STD_EXCEPTION(except)                                                                                                \
    throw except(einsums::detail::make_error_message(einsums::type_name<except>(), "", std::source_location::current())) /**/

#define EINSUMS_THROW_EXCEPTION(except, ...)                                                                                               \
    throw except(einsums::detail::make_error_message(einsums::type_name<except>(), einsums::errors::detail::corrected_format(__VA_ARGS__), \
                                                     std::source_location::current())) /**/

#define EINSUMS_THROW_CODED_EXCEPTION(except, code, ...)                                                                                   \
    throw einsums::CodedError<except, code>(einsums::detail::make_error_message(                                                           \
        einsums::type_name<except>(), einsums::errors::detail::corrected_format(__VA_ARGS__), std::source_location::current())) /**/

#define EINSUMS_THROW_NOT_IMPLEMENTED                                                                                                      \
    throw not_implemented(                                                                                                                 \
        einsums::detail::make_error_message(einsums::type_name<not_implemented>(), "", std::source_location::current())) /**/
