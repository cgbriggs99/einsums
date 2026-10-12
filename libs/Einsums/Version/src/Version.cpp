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

#define EINSUMS_VERSION_STRING                                                                                                             \
    EINSUMS_PP_STRINGIFY(EINSUMS_VERSION_MAJOR)                                                                                            \
    "." EINSUMS_PP_STRINGIFY(EINSUMS_VERSION_MINOR) "." EINSUMS_PP_STRINGIFY(EINSUMS_VERSION_PATCH)

#define EINSUMS_BUILD_STRING "v" EINSUMS_VERSION_STRING EINSUMS_VERSION_TAG ", Git: " EINSUMS_HAVE_GIT_COMMIT_SHORT
#define EINSUMS_DATE_TIME    __DATE__ " " __TIME__

#define EINSUMS_COMPLETE_VERSION                                                                                                           \
    "Version:\n  Einsums: " EINSUMS_BUILD_STRING                                                                                           \
    "\n\nBuild:\n  Type: " EINSUMS_PP_STRINGIFY(EINSUMS_BUILD_TYPE) "\n  Date: " EINSUMS_DATE_TIME "\n"

///////////////////////////////////////////////////////////////////////////////
namespace einsums {
namespace {
char const version_string[] = EINSUMS_VERSION_STRING;

char const build_string_[] = EINSUMS_BUILD_STRING;

char const date_string[] = EINSUMS_DATE_TIME;

char const complete_version_string[] = EINSUMS_COMPLETE_VERSION;
} // namespace

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
        std::printf("The actual string data is at %p, has length %zu.\n", static_cast<void const *>(string->data()), string->length());
    } else {
        std::printf("The actual string data is at %p, has length %zu, and contains the following:\n%s\n",
                    static_cast<void const *>(string->data()), string->length(), string->c_str());
    }
    std::fflush(stdout);
}

std::string full_version_as_string() {
    return std::string(version_string);
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

    strm << "Einsums:" << std::endl;

    char const *const *p = einsums::config_strings;
    while (*p)
        strm << "  " << *p++ << std::endl;
    strm << std::endl;

    return strm.str();
}

std::string build_string() {
    return std::string(build_string_);
}

std::string complete_version() {

    return std::string(complete_version_string);
}

std::string build_date_time() {
    return std::string(date_string);
}

} // namespace einsums
