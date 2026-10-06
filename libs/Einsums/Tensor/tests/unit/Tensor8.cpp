//----------------------------------------------------------------------------------------------
// Copyright (c) The Einsums Developers. All rights reserved.
// Licensed under the MIT License. See LICENSE.txt in the project root for license information.
//----------------------------------------------------------------------------------------------

#include <Einsums/Tensor/Tensor.hpp>
#include <Einsums/TensorUtilities/CreateIncrementedTensor.hpp>

#include <Einsums/Testing.hpp>

TEST_CASE("reshape") {
    SECTION("1") {
        auto C = einsums::create_incremented_tensor("C", 10, 10, 10);
        REQUIRE_NOTHROW(einsums::Tensor{std::move(C), "D", 10, -1});
        // NOTE: At this point tensor C is no longer valid.
    }

    SECTION("2") {
        auto C = einsums::create_incremented_tensor("C", 10, 10, 10);
        auto D = einsums::Tensor{std::move(C), "D", 100, 10};
        // NOTE: At this point tensor C is no longer valid.

        // println(C); // <- This will cause a segfault when println tries to print the tensor elements
        // println(D); // <- This succeeds.
    }

    SECTION("3") {
        auto C = einsums::create_incremented_tensor("C", 10, 10, 10);
        REQUIRE_THROWS(einsums::Tensor{std::move(C), "D", -1, -1});
        // NOTE: At this point tensor C is no longer valid.
    }

    SECTION("4") {
        auto C = einsums::create_incremented_tensor("C", 10, 10, 10);
        REQUIRE_THROWS(einsums::Tensor{std::move(C), "D", 9, 9});
        // NOTE: At this point tensor C is no longer valid.
    }
}