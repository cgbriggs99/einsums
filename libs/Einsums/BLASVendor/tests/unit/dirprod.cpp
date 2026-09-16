//----------------------------------------------------------------------------------------------
// Copyright (c) The Einsums Developers. All rights reserved.
// Licensed under the MIT License. See LICENSE.txt in the project root for license information.
//----------------------------------------------------------------------------------------------

#include <Einsums/TensorUtilities/CreateRandomTensor.hpp>

#include <Einsums/Testing.hpp>
#include <Einsums/Tensor/Tensor.hpp>

TEMPLATE_TEST_CASE("direct product", "[blas]", float, double, std::complex<float>, std::complex<double>) {
    auto A = einsums::create_random_tensor<TestType>("A", 101);
    auto B = einsums::create_random_tensor<TestType>("B", 101);
    auto C = einsums::create_random_tensor<TestType>("C", 101);

    einsums::Tensor<TestType, 1> C_copy = C;

    for (size_t i = 0; i < 101; i++) {
        C_copy(i) += TestType{0.5} * A(i) * B(i);
    }

    einsums::blas::dirprod(101, TestType{0.5}, A.data(), 1, B.data(), 1, C.data(), 1);
    
    for(size_t i = 0; i < 101; i++) {
        REQUIRE_THAT(C(i), einsums::CheckWithinRel(C_copy(i), 1e-6));
    }
}