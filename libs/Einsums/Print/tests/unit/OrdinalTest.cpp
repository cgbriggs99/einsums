//----------------------------------------------------------------------------------------------
// Copyright (c) The Einsums Developers. All rights reserved.
// Licensed under the MIT License. See LICENSE.txt in the project root for license information.
//----------------------------------------------------------------------------------------------

#include <Einsums/Print.hpp>

#include <Einsums/Testing.hpp>

TEST_CASE("Formatting ordinals", "[print]") {
    using namespace einsums;

    std::string formatted;

    size_t formatted_size = fmt::formatted_size("{}", print::ordinal{1});

    formatted.resize(formatted_size);

    fmt::format_to(formatted.begin(), "{}", print::ordinal{1});

    REQUIRE(formatted == "1st");
}