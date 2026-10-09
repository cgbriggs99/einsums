//----------------------------------------------------------------------------------------------
// Copyright (c) The Einsums Developers. All rights reserved.
// Licensed under the MIT License. See LICENSE.txt in the project root for license information.
//----------------------------------------------------------------------------------------------

#include <Einsums/Config.hpp>

#include <Einsums/Config/ConfigStrings.hpp>
#include <Einsums/Config/Version.hpp>
#include <Einsums/Version.hpp>

#include <fmt/format.h>

#include <sstream>
#include <string>

///////////////////////////////////////////////////////////////////////////////
namespace einsums {

std::string full_version_as_string() {
    return fmt::format("{}.{}.{}", EINSUMS_VERSION_MAJOR, EINSUMS_VERSION_MINOR, EINSUMS_VERSION_PATCH);
}

std::string full_build_string() {
    std::ostringstream strm;
    strm << "{config}:\n"
         << configuration_string() << "{version}: " << build_string() << "\n"
         << "{build-type}: " << build_type() << "\n"
         << "{date}: " << build_date_time() << "\n";

    return strm.str();
}

///////////////////////////////////////////////////////////////////////////
std::string configuration_string() {
    std::ostringstream strm;

    strm << "Einsums:\n";

    char const *const *p = einsums::config_strings;
    while (*p)
        strm << "  " << *p++ << "\n";
    strm << "\n";

    return strm.str();
}

static void print_actual_string(std::string const *string) {
    std::printf("Printing %zu bytes of string data structure at %p\n", sizeof(std::string), static_cast<void const *>(string));

    // I am not turning a C++ string into a C string. I am turning the string data structure into its underlying bytes.
    uint8_t const *string_data = reinterpret_cast<uint8_t const *>(string);

    for (size_t i = 0; i < sizeof(std::string); i++) {
        std::printf("%02hhx", string_data[i]);
        if (i % 4 == 3) {
            std::putchar(' ');
        }
    }

    std::printf("\n");

    if (string->data() == nullptr) {
        std::printf("The actual string data is at %p, has length %zu.\n",
                    static_cast<void const *>(string->data()), string->length());
    } else {
        std::printf("The actual string data is at %p, has length %zu, and contains the following:\n%s\n",
                    static_cast<void const *>(string->data()), string->length(), string->c_str());
    }
    std::fflush(stdout);
}

std::string build_string() {
    std::string out;

    {
        std::printf("Copying the format string to a temporary variable.\n");
        std::fflush(stdout);
        std::string temp =
            fmt::format("v{}{}, Git: {:.10}", full_version_as_string().c_str(), EINSUMS_VERSION_TAG, EINSUMS_HAVE_GIT_COMMIT);

        // Print out the actual data structure, see if something is wrong.
        print_actual_string(&out);
        print_actual_string(&temp);

        std::printf("Copying the temporary string to an output variable.\n");
        std::fflush(stdout);
        out = std::string(temp);

        print_actual_string(&out);
        print_actual_string(&temp);

        std::printf("Calling the temporary destructor.\n");
        std::fflush(stdout);
    }

    std::printf("Destructor worked.\n");
    std::fflush(stdout);

    return out;
}

std::string complete_version() {
    std::string version = fmt::format("Version:\n"
                                      "  Einsums: {}\n"
                                      "\n"
                                      "Build:\n"
                                      "  Type: {}\n"
                                      "  Date: {}\n",
                                      build_string().c_str(), build_type(), build_date_time().c_str());

    return version;
}

std::string build_date_time() {
    return std::string(__DATE__) + " " + __TIME__;
}

} // namespace einsums
