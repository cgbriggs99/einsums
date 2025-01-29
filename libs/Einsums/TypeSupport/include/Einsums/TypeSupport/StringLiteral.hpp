//--------------------------------------------------------------------------------------------
// Copyright (c) The Einsums Developers. All rights reserved.
// Licensed under the MIT License. See LICENSE.txt in the project root for license information.
//--------------------------------------------------------------------------------------------

#pragma once

#include <fmt/format.h>

#include <algorithm>
#include <array>
#include <string>
#include <string_view>

namespace einsums {

/// Normal strings cannot be used as template parameters, but this can.
/// This is needed for the parameters names in the NamedTuples.
template <size_t N>
struct StringLiteral {
    constexpr StringLiteral(auto const... chars) : _arr{chars..., '\0'} {}

    constexpr StringLiteral(std::array<char, N> const &arr) : _arr(arr) {}

    constexpr StringLiteral(char const (&str)[N]) { std::copy_n(str, N, std::data(_arr)); }

    /// Returns the value as a string
    std::string str() const { return std::string(string_view()); }

    /// Returns the value as a string
    constexpr std::string_view string_view() const { return std::string_view(std::data(_arr), N - 1); }

    std::array<char, N> _arr{};
};

template <size_t N1, size_t N2>
constexpr bool operator==(StringLiteral<N1> const &_first, StringLiteral<N2> const &_second) {
    if constexpr (N1 != N2) {
        return false;
    }
    return _first.string_view() == _second.string_view();
}

template <size_t N1, size_t N2>
constexpr bool operator!=(StringLiteral<N1> const &_first, StringLiteral<N2> const &_second) {
    return !(_first == _second);
}

} // namespace einsums

template <size_t N>
struct fmt::formatter<einsums::StringLiteral<N>> {
    // Parse the format specification, if needed.
    template <typename ParseContext>
    constexpr auto parse(ParseContext &ctx) {
        return ctx.begin();
    }

    // Format the StringLiteral object.
    template <typename FormatContext>
    auto format(einsums::StringLiteral<N> const &sl, FormatContext &ctx) const {
        // Write the string representation into the output iterator.
        return fmt::format_to(ctx.out(), "{}", sl.string_view());
    }
};
