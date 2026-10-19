//----------------------------------------------------------------------------------------------
// Copyright (c) The Einsums Developers. All rights reserved.
// Licensed under the MIT License. See LICENSE.txt in the project root for license information.
//----------------------------------------------------------------------------------------------

#include <Einsums/CommandLine/CommandLine.hpp>

namespace einsums::cl {

ExclusiveCategory::ExclusiveCategory() {
    Registry::get_singleton().add_exclusion(this);
}

ExclusiveCategory::~ExclusiveCategory() {
    std::printf("Finalizing ExclusiveCategory.\n");
}

bool ExclusiveCategory::verify_exclusions() const {
    bool found_one = false;

    for (auto const *opt : options) {
        std::printf("Checking %s\n", opt->get_long_name().c_str());
        std::fflush(stdout);
        if (opt->is_from_cli() || opt->is_from_config() || opt->num_occurrences() > 0) {
            std::printf("Found %s\n", opt->get_long_name().c_str());
            std::fflush(stdout);
            if (found_one) {
                std::printf("Already found an argument in this category!\n");
                std::fflush(stdout);
                return false;
            }
            found_one = true;
        }
    }

    return true;
}

void ExclusiveCategory::add_option(OptionBase *option) {
    options.push_back(option);
}

} // namespace einsums::cl