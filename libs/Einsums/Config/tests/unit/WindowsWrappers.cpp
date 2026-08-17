//----------------------------------------------------------------------------------------------
// Copyright (c) The Einsums Developers. All rights reserved.
// Licensed under the MIT License. See LICENSE.txt in the project root for license information.
//----------------------------------------------------------------------------------------------

#include <Einsums/Config/StdAlternatives.hpp>

#include <cstdlib>
#include <random>

#include <Einsums/Testing.hpp>

#ifndef EINSUMS_WINDOWS
using einsums::errno_t;
#    define FAIL_TAG "[!nonportable]"
#else
#    define FAIL_TAG "[!shouldfail][!nonportable]"
#endif

//TEST_CASE("asctime_s null buffer", "[windows-overrides][asctime_s]") {
//    std::time_t    curr = std::time(nullptr);
//    struct std::tm time_struct;
//
//    REQUIRE(einsums::localtime_s(&time_struct, &curr) == 0);
//    REQUIRE(einsums::asctime_s(nullptr, 0, &time_struct) == EINVAL);
//}
//
//TEST_CASE("asctime_s no data", "[windows-overrides][asctime_s]") {
//    std::array<char, 256> buffer;
//    std::time_t           curr = std::time(nullptr);
//    struct std::tm        time_struct;
//
//    REQUIRE(einsums::localtime_s(&time_struct, &curr) == 0);
//    REQUIRE(einsums::asctime_s(buffer.data(), 0, &time_struct) == EINVAL);
//}

//TEST_CASE("asctime_s buffer too small", "[windows-overrides][asctime_s]") {
//    std::array<char, 256> buffer;
//    buffer[0]           = 'A';
//    buffer[5]           = 'A';
//    std::time_t    curr = std::time(nullptr);
//    struct std::tm time_struct;
//
//    REQUIRE(einsums::localtime_s(&time_struct, &curr) == 0);
//    REQUIRE(einsums::asctime_s(buffer.data(), 5, &time_struct) == EINVAL);
//    REQUIRE(buffer[0] == 0);
//    REQUIRE(buffer[5] == 'A');
//}

//TEST_CASE("asctime_s no time pointer", "[windows-overrides][asctime_s][!shouldfail]") {
//    std::array<char, 256> buffer;
//    buffer[0] = 'A';
//    REQUIRE(einsums::asctime_s(buffer.data(), buffer.size(), nullptr) == EINVAL);
//    REQUIRE(buffer[0] == 0);
//}

TEST_CASE("asctime_s proper inputs", "[windows-override][asctime_s]") {
    std::array<char, 256> buffer;
    std::time_t           curr = std::time(nullptr);
    struct std::tm        time_struct;

    REQUIRE(einsums::localtime_s(&time_struct, &curr) == 0);

    REQUIRE(einsums::asctime_s(buffer.data(), buffer.size(), &time_struct) == 0);
    INFO(buffer.data());
}

static int int_compare(void *context, void const *left, void const *right) {
    int const *const int_left = reinterpret_cast<int const *>(left), *const int_right = reinterpret_cast<int const *>(right);

    if (*int_left < *int_right) {
        return -1;
    } else if (*int_left == *int_right) {
        return 0;
    } else {
        return 1;
    }
}

TEST_CASE("qsort_s", "[windows-overrides][qsort_s]") {
    std::default_random_engine         engine;
    std::uniform_int_distribution<int> random_dist(std::numeric_limits<int>::min(), std::numeric_limits<int>::max());

    SECTION("small") {
        std::vector<int> random_data(16);

        // Fill with random data.
        for (size_t i = 0; i < random_data.size(); i++) {
            random_data[i] = random_dist(engine);
        }

        // Sort.
        REQUIRE_NOTHROW(
            einsums::qsort_s(reinterpret_cast<void *>(random_data.data()), random_data.size(), sizeof(int), int_compare, nullptr));

        // Make sure the values are increasing.
        for (size_t i = 0; i < random_data.size() - 1; i++) {
            CHECK(random_data[i] <= random_data[i + 1]);
        }
    }

    SECTION("large") {
        std::vector<int> random_data(256);

        // Fill with random data.
        for (size_t i = 0; i < random_data.size(); i++) {
            random_data[i] = random_dist(engine);
        }

        // Sort.
        REQUIRE_NOTHROW(
            einsums::qsort_s(reinterpret_cast<void *>(random_data.data()), random_data.size(), sizeof(int), int_compare, nullptr));

        // Make sure the values are increasing.
        for (size_t i = 0; i < random_data.size() - 1; i++) {
            CHECK(random_data[i] <= random_data[i + 1]);
        }
    }
}

TEST_CASE("bsearch_s", "[windows-overrides][qsort_s][bsearch_s]") {
    std::default_random_engine         engine;
    std::uniform_int_distribution<int> random_dist(std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
    std::vector<int>                   random_data(256);

    // Fill with random data.
    for (size_t i = 0; i < random_data.size(); i++) {
        random_data[i] = random_dist(engine);
    }

    // Sort.
    REQUIRE_NOTHROW(einsums::qsort_s(reinterpret_cast<void *>(random_data.data()), random_data.size(), sizeof(int), int_compare, nullptr));

    // Make sure the values are increasing.
    for (size_t i = 0; i < random_data.size() - 1; i++) {
        CHECK(random_data[i] <= random_data[i + 1]);
    }

    for (size_t i = 0; i < random_data.size(); i++) {
        int *found;

        REQUIRE_NOTHROW(found = reinterpret_cast<int *>(einsums::bsearch_s(reinterpret_cast<void *>(&(random_data[i])),
                                                                           reinterpret_cast<void *>(random_data.data()), random_data.size(),
                                                                           sizeof(int), int_compare, nullptr)));
        REQUIRE(found != nullptr);
        REQUIRE(*found == random_data[i]);
    }

    // Find the smallest value not in the array.
    // We need this in case 0xffffffff is in the array.
    int  smallest = std::numeric_limits<int>::min();
    bool found    = true;
    do {
        found = true;
        for (size_t i = 0; i < random_data.size(); i++) {
            if (random_data[i] == smallest) {
                found = false;
                smallest++;
                break;
            }
        }
    } while (!found);

    // Make sure the value is not in the array.
    // We need this in case 0 is in the array.
    REQUIRE(einsums::bsearch_s(reinterpret_cast<void *>(&smallest), reinterpret_cast<void *>(random_data.data()), random_data.size(),
                               sizeof(int), int_compare, nullptr) == nullptr);

    // Find the middlemost value not in the array.
    int near_zero = 0;
    do {
        found = true;
        for (size_t i = 0; i < random_data.size(); i++) {
            if (random_data[i] == near_zero) {
                found = false;
                near_zero++;
                break;
            }
        }

    } while (!found);

    // Make sure the value is not in the array.
    REQUIRE(einsums::bsearch_s(reinterpret_cast<void *>(&near_zero), reinterpret_cast<void *>(random_data.data()), random_data.size(),
                               sizeof(int), int_compare, nullptr) == nullptr);

    // Find the largest value not in the array.
    int biggest = std::numeric_limits<int>::max();
    do {
        found = true;
        for (size_t i = 0; i < random_data.size(); i++) {
            if (random_data[i] == biggest) {
                found = false;
                biggest--;
                break;
            }
        }
    } while (!found);

    // Make sure the value is not in the array
    REQUIRE(einsums::bsearch_s(reinterpret_cast<void *>(&biggest), reinterpret_cast<void *>(random_data.data()), random_data.size(),
                               sizeof(int), int_compare, nullptr) == nullptr);
}

// This one fails extra bad
// TEST_CASE("clearerr_s", "[windows-override][clearerr_s][!shouldfail]") {
//    // Not much we can do here. Just test nullptr.
//    REQUIRE(einsums::clearerr_s(nullptr) == EINVAL);
//}

//TEST_CASE("fopen_s null output", "[windows-override][fopen_s][!shouldfail]") {
//    REQUIRE(einsums::fopen_s(nullptr, nullptr, nullptr) == EINVAL);
//}

// this causes a segfault.
//TEST_CASE("fopen_s null file name", "[windows-override][fopen_s][!shouldfail]") {
//    std::FILE *fp = nullptr;
//    // Test opening.
//    REQUIRE(einsums::fopen_s(&fp, nullptr, nullptr) == EINVAL);
//    REQUIRE(fp == nullptr);
//}
//
//TEST_CASE("fopen_s null mode", "[windows-override][fopen_s][!shouldfail]") {
//    std::FILE *fp = nullptr;
//    // Test opening.
//    REQUIRE(einsums::fopen_s(&fp, "test.txt", nullptr) == EINVAL);
//    REQUIRE(fp == nullptr);
//}

TEST_CASE("fopen_s, fprintf_s, freopen_s, fread_s", "[windows-override][fopen_s]") {
    std::FILE *fp = nullptr;
    // Test opening.
    REQUIRE(einsums::fopen_s(&fp, "test.txt", "w+") == 0);
    REQUIRE(fp != nullptr);

    // Test printing.
    REQUIRE(einsums::fprintf_s(fp, "Hello, World! Test %d\n", 123) >= 23);

    // Test reopening.
    REQUIRE(einsums::freopen_s(&fp, "test.txt", "r", fp) == 0);

    // Test reading.
    std::array<char, 256> buffer;
    REQUIRE(einsums::fread_s(reinterpret_cast<void *>(buffer.data()), buffer.size(), sizeof(char), 23, fp) == 23);

    REQUIRE_NOTHROW(std::fclose(fp));
}