//----------------------------------------------------------------------------------------------
// Copyright (c) The Einsums Developers. All rights reserved.
// Licensed under the MIT License. See LICENSE.txt in the project root for license information.
//----------------------------------------------------------------------------------------------

#include "einsums/LinearAlgebra.hpp"

#include "einsums/Print.hpp"
#include "einsums/STL.hpp"
#include "einsums/Sort.hpp"
#include "einsums/Tensor.hpp"
#include "einsums/TensorAlgebra.hpp"
#include "einsums/Utilities.hpp"
#include "einsums/utility/ComplexTraits.hpp"

#include <catch2/catch_all.hpp>
#include <complex>
#include <filesystem>
#include <random>

using namespace einsums;
namespace {
template <typename T>
auto reconstruct(const Tensor<T, 2> &u, const Tensor<T, 1> &s, const Tensor<T, 2> &vt, size_t N, size_t LDA) -> Tensor<T, 2> {
    using namespace einsums::tensor_algebra;

    auto half = create_tensor<T>("sigma", N, LDA);
    zero(half);
    auto new_a = create_tensor<T>("new a", N, LDA);
    zero(new_a);
    auto sigma = diagonal_like(s, new_a);

    einsum(Indices{index::i, index::j}, &half, Indices{index::i, index::k}, u, Indices{index::k, index::j}, sigma);
    einsum(Indices{index::i, index::j}, &new_a, Indices{index::i, index::k}, half, Indices{index::k, index::j}, vt);

    return new_a;
}
} // namespace

template <typename T>
void gesvd_test() {
    /*
       DGESVD Example.
       ==============

       Program computes the singular value decomposition of a general
       rectangular matrix A:

         8.79   9.93   9.83   5.45   3.16
         6.11   6.91   5.04  -0.27   7.98
        -9.15  -7.93   4.86   4.85   3.01
         9.57   1.64   8.83   0.74   5.80
        -3.49   4.02   9.80  10.00   4.27
         9.84   0.15  -8.99  -6.02  -5.31

       Description.
       ============

       The routine computes the singular value decomposition (SVD) of a real
       m-by-n matrix A, optionally computing the left and/or right singular
       vectors. The SVD is written as

       A = U*SIGMA*VT

       where SIGMA is an m-by-n matrix which is zero except for its min(m,n)
       diagonal elements, U is an m-by-m orthogonal matrix and VT (V transposed)
       is an n-by-n orthogonal matrix. The diagonal elements of SIGMA
       are the singular values of A; they are real and non-negative, and are
       returned in descending order. The first min(m, n) columns of U and V are
       the left and right singular vectors of A.

       Note that the routine returns VT, not V.

       Example Program Results.
       ========================

     DGESVD Example Program Results

     Singular values
      27.47  22.64   8.56   5.99   2.01

     Left singular vectors (stored columnwise)
      -0.59   0.26   0.36   0.31   0.23
      -0.40   0.24  -0.22  -0.75  -0.36
      -0.03  -0.60  -0.45   0.23  -0.31
      -0.43   0.24  -0.69   0.33   0.16
      -0.47  -0.35   0.39   0.16  -0.52
       0.29   0.58  -0.02   0.38  -0.65

     Right singular vectors (stored rowwise)
      -0.25  -0.40  -0.69  -0.37  -0.41
       0.81   0.36  -0.25  -0.37  -0.10
      -0.26   0.70  -0.22   0.39  -0.49
       0.40  -0.45   0.25   0.43  -0.62
      -0.22   0.14   0.59  -0.63  -0.44
    */
    constexpr int M{6};
    constexpr int N{5};
    constexpr int LDA{M};
    constexpr int LDU{M};
    constexpr int LDVT{N};

    using namespace einsums;

    auto a = create_tensor<T>("a", N, LDA);

    a.vector_data() = einsums::VectorData<T>{8.79, 6.11, -9.15, 9.57, -3.49, 9.84, 9.93, 6.91,  -7.93, 1.64, 4.02, 0.15, 9.83, 5.04, 4.86,
                                             8.83, 9.80, -8.99, 5.45, -0.27, 4.85, 0.74, 10.00, -6.02, 3.16, 7.98, 3.01, 5.80, 4.27, -5.31};

    auto [u, s, vt] = linear_algebra::svd(a);

    // Using u, s, and vt reconstruct a and test a against the reconstructed a
    auto new_a = reconstruct(u, s, vt, N, LDA);

    CHECK_THAT(new_a.vector_data(), Catch::Matchers::Approx(a.vector_data()).margin(0.0001));
}

TEST_CASE("gesvd") {
    SECTION("float") {
        gesvd_test<float>();
    }
    SECTION("double") {
        gesvd_test<double>();
    }
}

template <typename T>
void gesdd_test() {
    /*
   DGESDD Example.
   ==============

   Program computes the singular value decomposition of a general
   rectangular matrix A using a divide and conquer method, where A is:

     7.52  -1.10  -7.95   1.08
    -0.76   0.62   9.34  -7.10
     5.13   6.62  -5.66   0.87
    -4.75   8.52   5.75   5.30
     1.33   4.91  -5.49  -3.52
    -2.40  -6.77   2.34   3.95

   Description.
   ============

   The routine computes the singular value decomposition (SVD) of a real
   m-by-n matrix A, optionally computing the left and/or right singular
   vectors. If singular vectors are desired, it uses a divide and conquer
   algorithm. The SVD is written as

   A = U*SIGMA*VT

   where SIGMA is an m-by-n matrix which is zero except for its min(m,n)
   diagonal elements, U is an m-by-m orthogonal matrix and VT (V transposed)
   is an n-by-n orthogonal matrix. The diagonal elements of SIGMA
   are the singular values of A; they are real and non-negative, and are
   returned in descending order. The first min(m, n) columns of U and V are
   the left and right singular vectors of A.

   Note that the routine returns VT, not V.

   Example Program Results.
   ========================

 DGESDD Example Program Results

 Singular values
  18.37  13.63  10.85   4.49

 Left singular vectors (stored columnwise)
  -0.57   0.18   0.01   0.53
   0.46  -0.11  -0.72   0.42
  -0.45  -0.41   0.00   0.36
   0.33  -0.69   0.49   0.19
  -0.32  -0.31  -0.28  -0.61
   0.21   0.46   0.39   0.09

 Right singular vectors (stored rowwise)
  -0.52  -0.12   0.85  -0.03
   0.08  -0.99  -0.09  -0.01
  -0.28  -0.02  -0.14   0.95
   0.81   0.01   0.50   0.31
*/
    constexpr int M{6};
    constexpr int N{4};
    constexpr int LDA{M};
    constexpr int LDU{M};
    constexpr int LDVT{N};

    using namespace einsums;

    auto a = create_tensor<T>("a", N, LDA);

    a.vector_data() = einsums::VectorData<T>{7.52,  -0.76, 5.13,  -4.75, 1.33,  -2.40, -1.10, 0.62,  6.62, 8.52, 4.91,  -6.77,
                                             -7.95, 9.34,  -5.66, 5.75,  -5.49, 2.34,  1.08,  -7.10, 0.87, 5.30, -3.52, 3.95};

    auto [u, s, vt] = linear_algebra::svd_dd(a);

    // Using u, s, and vt reconstruct a and test a against the reconstructed a
    auto new_a = reconstruct(u, s, vt, N, LDA);

    CHECK_THAT(new_a.vector_data(), Catch::Matchers::Approx(a.vector_data()).margin(0.0001));
}

TEST_CASE("gesdd") {
    SECTION("double") {
        gesdd_test<double>();
    }
    SECTION("float") {
        gesdd_test<float>();
    }
}

template <typename T>
void gesv_test() {
    /*
       LAPACKE_dgesv Example.
       ======================

       The program computes the solution to the system of linear
       equations with a square matrix A and multiple
       right-hand sides B, where A is the coefficient matrix:

         6.80  -6.05  -0.45   8.32  -9.67
        -2.11  -3.30   2.58   2.71  -5.14
         5.66   5.36  -2.70   4.35  -7.26
         5.97  -4.44   0.27  -7.17   6.08
         8.23   1.08   9.04   2.14  -6.87

       and B is the right-hand side matrix:

         4.02  -1.56   9.81
         6.19   4.00  -4.09
        -8.22  -8.67  -4.57
        -7.57   1.75  -8.61
        -3.03   2.86   8.99

       Description.
       ============

       The routine solves for X the system of linear equations A*X = B,
       where A is an n-by-n matrix, the columns of matrix B are individual
       right-hand sides, and the columns of X are the corresponding
       solutions.

       The LU decomposition with partial pivoting and row interchanges is
       used to factor A as A = P*L*U, where P is a permutation matrix, L
       is unit lower triangular, and U is upper triangular. The factored
       form of A is then used to solve the system of equations A*X = B.

       Example Program Results.
       ========================

     LAPACKE_dgesv (column-major, high-level) Example Program Results

     Solution
      -0.80  -0.39   0.96
      -0.70  -0.55   0.22
       0.59   0.84   1.90
       1.32  -0.10   5.36
       0.57   0.11   4.04

     Details of LU factorization
       8.23   1.08   9.04   2.14  -6.87
       0.83  -6.94  -7.92   6.55  -3.99
       0.69  -0.67 -14.18   7.24  -5.19
       0.73   0.75   0.02 -13.82  14.19
      -0.26   0.44  -0.59  -0.34  -3.43

     Pivot indices
          5      5      3      4      5
        */

    constexpr int N{5};
    constexpr int NRHS{3};
    constexpr int LDA{N};
    constexpr int LDB{N};

    using namespace einsums;

    auto a = create_tensor<T>("a", N, LDA);
    auto b = create_tensor<T>("b", NRHS, LDB);

    a.vector_data() = einsums::VectorData<T>{6.80, -2.11, 5.66, 5.97, 8.23, -6.05, -3.30, 5.36,  -4.44, 1.08,  -0.45, 2.58, -2.70,
                                             0.27, 9.04,  8.32, 2.71, 4.35, -7.17, 2.14,  -9.67, -5.14, -7.26, 6.08,  -6.87};
    b.vector_data() =
        einsums::VectorData<T>{4.02, 6.19, -8.22, -7.57, -3.03, -1.56, 4.00, -8.67, 1.75, 2.86, 9.81, -4.09, -4.57, -8.61, 8.99};

    linear_algebra::gesv(&a, &b);

    CHECK_THAT(a.vector_data(),
               Catch::Matchers::Approx(einsums::VectorData<T>{8.23000000,  0.82624544,  0.68772783,   0.72539490,   -0.25637910,
                                                              1.08000000,  -6.94234508, -0.66508563,  0.75240087,   0.43545957,
                                                              9.04000000,  -7.91925881, -14.18404477, 0.02320302,   -0.58842060,
                                                              2.14000000,  6.55183475,  7.23579360,   -13.81984350, -0.33743379,
                                                              -6.87000000, -3.99369380, -5.19145820,  14.18877913,  -3.42921969})
                   .margin(0.00001));
    CHECK_THAT(b.vector_data(),
               Catch::Matchers::Approx(einsums::VectorData<T>{-0.80071403, -0.69524338, 0.59391499, 1.32172561, 0.56575620, -0.38962139,
                                                              -0.55442713, 0.84222739, -0.10380185, 0.10571095, 0.95546491, 0.22065963,
                                                              1.90063673, 5.35766149, 4.04060266})
                   .margin(0.00001));
}

TEST_CASE("gesv") {
    SECTION("float") {
        gesv_test<float>();
    }
    SECTION("double") {
        gesv_test<double>();
    }
    SECTION("complex<float>") {
        // gesv_test<std::complex<float>>();
    }
}

template <einsums::NotComplex T>
void gemv_test() {
    using namespace einsums;
    using namespace einsums::linear_algebra;

    auto A  = create_incremented_tensor<T>("A", 3, 3);
    auto b  = create_incremented_tensor<T>("b", 3);
    auto fy = create_incremented_tensor<T>("y", 3);
    auto ty = create_incremented_tensor<T>("y", 3);

    // Perform basic matrix-vector
    einsums::linear_algebra::gemv<false>(1.0, A, b, 0.0, &fy);
    einsums::linear_algebra::gemv<true>(1.0, A, b, 0.0, &ty);

    REQUIRE(fy(0) == 5.0);
    REQUIRE(fy(1) == 14.0);
    REQUIRE(fy(2) == 23.0);

    REQUIRE(ty(0) == 15.0);
    REQUIRE(ty(1) == 18.0);
    REQUIRE(ty(2) == 21.0);
}

template <einsums::Complex T>
void gemv_test() {
    using namespace einsums;
    using namespace einsums::linear_algebra;

    auto A  = create_incremented_tensor<T>("A", 3, 3);
    auto b  = create_incremented_tensor<T>("b", 3);
    auto fy = create_incremented_tensor<T>("y", 3);
    auto ty = create_incremented_tensor<T>("y", 3);

    // Perform basic matrix-vector
    einsums::linear_algebra::gemv<false>(T{1.0, 1.0}, A, b, T{0.0, 0.0}, &fy);
    einsums::linear_algebra::gemv<true>(T{1.0, 1.0}, A, b, T{0.0, 0.0}, &ty);

    REQUIRE(fy(0) == T{-10, 10});
    REQUIRE(fy(1) == T{-28, 28});
    REQUIRE(fy(2) == T{-46, 46});

    REQUIRE(ty(0) == T{-30, 30});
    REQUIRE(ty(1) == T{-36, 36});
    REQUIRE(ty(2) == T{-42, 42});
}

TEST_CASE("gemv") {
    SECTION("double") {
        gemv_test<double>();
    }
    SECTION("float") {
        gemv_test<float>();
    }
    SECTION("complex<double>") {
        gemv_test<std::complex<double>>();
    }
    SECTION("complex<float>") {
        gemv_test<std::complex<float>>();
    }
}

template <typename T>
void gemm_test_1() {
    using namespace einsums;
    using namespace einsums::linear_algebra;

    auto A    = create_incremented_tensor<T>("a", 3, 3);
    auto B    = create_incremented_tensor<T>("b", 3, 3);
    auto Cff  = create_tensor<T>("c", 3, 3);
    auto C0ff = create_tensor<T>("C0", 3, 3);
    zero(Cff);
    zero(C0ff);
    auto Cft  = create_tensor<T>("c", 3, 3);
    auto C0ft = create_tensor<T>("C0", 3, 3);
    zero(Cft);
    zero(C0ft);
    auto Ctf  = create_tensor<T>("c", 3, 3);
    auto C0tf = create_tensor<T>("C0", 3, 3);
    zero(Ctf);
    zero(C0tf);
    auto Ctt  = create_tensor<T>("c", 3, 3);
    auto C0tt = create_tensor<T>("C0", 3, 3);
    zero(Ctt);
    zero(C0tt);

    // Perform basic matrix multiplication
    einsums::linear_algebra::gemm<false, false>(T{1.0}, A, B, T{0.0}, &Cff);
    einsums::linear_algebra::gemm<false, true>(T{1.0}, A, B, T{0.0}, &Cft);
    einsums::linear_algebra::gemm<true, false>(T{1.0}, A, B, T{0.0}, &Ctf);
    einsums::linear_algebra::gemm<true, true>(T{1.0}, A, B, T{0.0}, &Ctt);

    for (size_t i = 0; i < 3; i++) {
        for (size_t j = 0; j < 3; j++) {
            for (size_t k = 0; k < 3; k++) {
                C0ff(i, j) += A(i, k) * B(k, j);
                C0ft(i, j) += A(i, k) * B(j, k);
                C0tf(i, j) += A(k, i) * B(k, j);
                C0tt(i, j) += A(k, i) * B(j, k);
            }
        }
    }

    for (size_t i = 0; i < 3; i++) {
        for (size_t j = 0; j < 3; j++) {
            REQUIRE(C0ff(i, j) == Cff(i, j));
            REQUIRE(C0ft(i, j) == Cft(i, j));
            REQUIRE(C0tf(i, j) == Ctf(i, j));
            REQUIRE(C0tt(i, j) == Ctt(i, j));
        }
    }
}

TEST_CASE("gemm_1") {
    SECTION("float") {
        gemm_test_1<float>();
    }

    SECTION("double") {
        gemm_test_1<double>();
    }

    SECTION("complex float") {
        gemm_test_1<std::complex<float>>();
    }

    SECTION("complex double") {
        gemm_test_1<std::complex<double>>();
    }
}

template <einsums::NotComplex T>
void gemm_test_2() {
    using namespace einsums;
    using namespace einsums::linear_algebra;

    auto A    = create_incremented_tensor<T>("a", 3, 3);
    auto B    = create_incremented_tensor<T>("b", 3, 3);
    auto C0ff = create_tensor<T>("C0", 3, 3);
    auto C0ft = create_tensor<T>("C0", 3, 3);
    auto C0tf = create_tensor<T>("C0", 3, 3);
    auto C0tt = create_tensor<T>("C0", 3, 3);
    zero(C0ff);
    zero(C0tf);
    zero(C0ft);
    zero(C0tt);

    // Perform basic matrix multiplication
    auto Cff = einsums::linear_algebra::gemm<false, false>(1.0, A, B);
    auto Cft = einsums::linear_algebra::gemm<false, true>(1.0, A, B);
    auto Ctf = einsums::linear_algebra::gemm<true, false>(1.0, A, B);
    auto Ctt = einsums::linear_algebra::gemm<true, true>(1.0, A, B);

    for (size_t i = 0; i < 3; i++) {
        for (size_t j = 0; j < 3; j++) {
            for (size_t k = 0; k < 3; k++) {
                C0ff(i, j) += A(i, k) * B(k, j);
                C0ft(i, j) += A(i, k) * B(j, k);
                C0tf(i, j) += A(k, i) * B(k, j);
                C0tt(i, j) += A(k, i) * B(j, k);
            }
        }
    }

    for (size_t i = 0; i < 3; i++) {
        for (size_t j = 0; j < 3; j++) {
            REQUIRE(C0ff(i, j) == Cff(i, j));
            REQUIRE(C0ft(i, j) == Cft(i, j));
            REQUIRE(C0tf(i, j) == Ctf(i, j));
            REQUIRE(C0tt(i, j) == Ctt(i, j));
        }
    }
}

template <einsums::Complex T>
void gemm_test_2() {
    using namespace einsums;
    using namespace einsums::linear_algebra;

    auto A    = create_incremented_tensor<T>("a", 3, 3);
    auto B    = create_incremented_tensor<T>("b", 3, 3);
    auto C0ff = create_tensor<T>("C0", 3, 3);
    auto C0ft = create_tensor<T>("C0", 3, 3);
    auto C0tf = create_tensor<T>("C0", 3, 3);
    auto C0tt = create_tensor<T>("C0", 3, 3);
    zero(C0ff);
    zero(C0tf);
    zero(C0ft);
    zero(C0tt);

    // Perform basic matrix multiplication
    auto Cff = einsums::linear_algebra::gemm<false, false>(T{1.0, 1.0}, A, B);
    auto Cft = einsums::linear_algebra::gemm<false, true>(T{1.0, 1.0}, A, B);
    auto Ctf = einsums::linear_algebra::gemm<true, false>(T{1.0, 1.0}, A, B);
    auto Ctt = einsums::linear_algebra::gemm<true, true>(T{1.0, 1.0}, A, B);

    for (size_t i = 0; i < 3; i++) {
        for (size_t j = 0; j < 3; j++) {
            for (size_t k = 0; k < 3; k++) {
                C0ff(i, j) += T{1.0, 1.0} * A(i, k) * B(k, j);
                C0ft(i, j) += T{1.0, 1.0} * A(i, k) * B(j, k);
                C0tf(i, j) += T{1.0, 1.0} * A(k, i) * B(k, j);
                C0tt(i, j) += T{1.0, 1.0} * A(k, i) * B(j, k);
            }
        }
    }

    for (size_t i = 0; i < 3; i++) {
        for (size_t j = 0; j < 3; j++) {
            REQUIRE(C0ff(i, j) == Cff(i, j));
            REQUIRE(C0ft(i, j) == Cft(i, j));
            REQUIRE(C0tf(i, j) == Ctf(i, j));
            REQUIRE(C0tt(i, j) == Ctt(i, j));
        }
    }
}

TEST_CASE("gemm_2") {
    SECTION("double") {
        gemm_test_2<double>();
    }
    SECTION("float") {
        gemm_test_2<float>();
    }
    SECTION("complex<float>") {
        gemm_test_2<std::complex<float>>();
    }
    SECTION("complex<double>") {
        gemm_test_2<std::complex<double>>();
    }
}

template <typename T>
void syev_test() {
    using namespace einsums;
    using namespace einsums::linear_algebra;

    auto A = create_tensor<T>("a", 3, 3);
    auto b = create_tensor<T>("b", 3);

    A.vector_data() = einsums::VectorData<T>{1.0, 2.0, 3.0, 2.0, 4.0, 5.0, 3.0, 5.0, 6.0};

    // Perform basic matrix multiplication
    einsums::linear_algebra::syev(&A, &b);

    CHECK_THAT(b(0), Catch::Matchers::WithinRel(-0.515729, 0.00001));
    CHECK_THAT(b(1), Catch::Matchers::WithinRel(+0.170915, 0.00001));
    CHECK_THAT(b(2), Catch::Matchers::WithinRel(+11.344814, 0.00001));
}

TEST_CASE("syev") {
    SECTION("float") {
        syev_test<float>();
    }

    SECTION("double") {
        syev_test<double>();
    }
}

TEMPLATE_TEST_CASE("definite and semidefinite", "[linear-algebra]", float, double) {
    using namespace einsums;
    using namespace einsums::linear_algebra;

    constexpr int size = 10;

    SECTION("positive definite") {
        auto A = create_random_definite<TestType>("A", size, size);

        auto B = create_tensor<TestType>("B", size);

        syev(&A, &B);

        for (int i = 0; i < size; i++) {
            REQUIRE(B(i) >= -einsums::WithinStrict(TestType(0.0), TestType{10000.0}).get_error());
        }
    }

    SECTION("negative definite") {
        auto A = create_random_definite<TestType>("A", size, size, TestType{-1.0});

        auto B = create_tensor<TestType>("B", size);

        syev(&A, &B);

        for (int i = 0; i < size; i++) {
            REQUIRE(B(i) <= einsums::WithinStrict(TestType(0.0), TestType{10000.0}).get_error());
        }
    }

    SECTION("positive semi-definite") {
        auto A = create_random_semidefinite<TestType>("A", size, size);

        auto B = create_tensor<TestType>("B", size);

        syev(&A, &B);

        int zeros = 0;
        for (int i = 0; i < size; i++) {
            if (einsums::WithinStrict(TestType(0.0), TestType{10000.0}).match(B(i))) {
                zeros++;
            }
            REQUIRE(B(i) >= -einsums::WithinStrict(TestType(0.0), TestType{10000.0}).get_error());
        }
        REQUIRE(zeros >= 1);
    }

    SECTION("negative semi-definite") {
        auto A = create_random_semidefinite<TestType>("A", size, size, TestType{-1.0});

        auto B = create_tensor<TestType>("B", size);

        syev(&A, &B);

        int zeros = 0;
        for (int i = 0; i < size; i++) {
            if (einsums::WithinStrict(TestType(0.0), TestType{10000.0}).match(B(i))) {
                zeros++;
            }
            REQUIRE(B(i) <= einsums::WithinStrict(TestType(0.0), TestType{10000.0}).get_error());
        }
        REQUIRE(zeros >= 1);
    }
}

template <NotComplex T>
void geev_test() {
    using namespace einsums;
    using namespace einsums::linear_algebra;

    auto a  = create_tensor<T>("a", 5, 5);
    auto w  = create_tensor<AddComplexT<T>>("w", 5);
    auto vl = create_tensor<T>("vl", 5, 5);
    auto vr = create_tensor<T>("vr", 5, 5);

    a.vector_data() = VectorData<T>{-1.01f, 0.86f,  -4.60f, 3.31f, -4.81f, 3.98f,  0.53f, -7.04f, 5.29f,  3.55f,  3.30f, 8.26f, -3.89f,
                                    8.20f,  -1.51f, 4.43f,  4.96f, -7.66f, -7.33f, 6.18f, 7.31f,  -6.43f, -6.16f, 2.47f, 5.58f};

    einsums::linear_algebra::geev(&a, &w, &vl, &vr);

    //    (0): (    2.85813284  +    10.76274967i)
    //    (1): (    2.85813284  +   -10.76274967i)
    //    (2): (   -0.68667454  +     4.70426130i)
    //    (3): (   -0.68667454  +    -4.70426130i)
    //    (4): (  -10.46291637  +     0.00000000i)

    CHECK_THAT(w(0).real(), Catch::Matchers::WithinRel(2.85813284, 0.001));
    CHECK_THAT(w(1).real(), Catch::Matchers::WithinRel(2.85813284, 0.001));
    CHECK_THAT(w(2).real(), Catch::Matchers::WithinRel(-0.68667454, 0.001));
    CHECK_THAT(w(3).real(), Catch::Matchers::WithinRel(-0.68667454, 0.001));
    CHECK_THAT(w(4).real(), Catch::Matchers::WithinRel(-10.46291637, 0.001));

    CHECK_THAT(w(0).imag(), Catch::Matchers::WithinRel(10.76274967, 0.001));
    CHECK_THAT(w(1).imag(), Catch::Matchers::WithinRel(-10.76274967, 0.001));
    CHECK_THAT(w(2).imag(), Catch::Matchers::WithinRel(4.70426130, 0.001));
    CHECK_THAT(w(3).imag(), Catch::Matchers::WithinRel(-4.70426130, 0.001));
    CHECK_THAT(w(4).imag(), Catch::Matchers::WithinRel(0.00, 0.001));
}

template <Complex T>
void geev_test() {
    using namespace einsums;
    using namespace einsums::linear_algebra;

    auto a  = create_tensor<T>("a", 4, 4);
    auto w  = create_tensor<T>("w", 4);
    auto vl = create_tensor<T>("vl", 4, 4);
    auto vr = create_tensor<T>("vr", 4, 4);

    a.vector_data() =
        einsums::VectorData<T>{{-3.84f, 2.25f},  {-8.94f, -4.75f}, {8.95f, -6.53f},  {-9.87f, 4.82f},  {-0.66f, 0.83f},  {-4.40f, -3.82f},
                               {-3.50f, -4.26f}, {-3.15f, 7.36f},  {-3.99f, -4.73f}, {-5.88f, -6.60f}, {-3.36f, -0.40f}, {-0.75f, 5.23f},
                               {7.74f, 4.18f},   {3.66f, -7.53f},  {2.58f, 3.60f},   {4.59f, 5.41f}};

    einsums::linear_algebra::geev(&a, &w, &vl, &vr);

    CHECK_THAT(w(0).real(), Catch::Matchers::WithinRel(-9.4298, 0.001));
    CHECK_THAT(w(1).real(), Catch::Matchers::WithinRel(-3.4419, 0.001));
    CHECK_THAT(w(2).real(), Catch::Matchers::WithinRel(0.1056, 0.001));
    CHECK_THAT(w(3).real(), Catch::Matchers::WithinRel(5.7562, 0.001));

    CHECK_THAT(w(0).imag(), Catch::Matchers::WithinRel(-12.9833, 0.001));
    CHECK_THAT(w(1).imag(), Catch::Matchers::WithinRel(12.6897, 0.001));
    CHECK_THAT(w(2).imag(), Catch::Matchers::WithinRel(-3.3950, 0.001));
    CHECK_THAT(w(3).imag(), Catch::Matchers::WithinRel(7.1286, 0.001));
}

TEST_CASE("geev") {
    geev_test<std::complex<float>>();
    geev_test<std::complex<double>>();
    geev_test<double>();
    geev_test<float>();
}

template <typename T>
void heev_test() {
    using namespace einsums;
    using namespace einsums::linear_algebra;

    auto A = create_tensor<T>("a", 3, 3);
    auto b = create_tensor<typename ComplexType<T>::Type>("b", 3);

    A.vector_data() = einsums::VectorData<T>{{0.199889, 0.0},         {-0.330816, -0.127778}, {-0.0546237, 0.176589},
                                             {-0.330816, 0.127778},   {0.629179, 0.0},        {-0.224813, 0.327171},
                                             {-0.0546237, -0.176589}, {-0.0224813, 0.327171}, {0.170931, 0.0}};

    einsums::linear_algebra::heev(&A, &b);

    // Sometimes 0.0 will be reported as -0.0 therefore we test the Abs of the first two
    CHECK_THAT(b(0), Catch::Matchers::WithinAbs(0.0, 0.00001));
    CHECK_THAT(b(1), Catch::Matchers::WithinAbs(0.0, 0.00001));
    CHECK_THAT(b(2), Catch::Matchers::WithinRel(1.0, 0.00001));
}

TEST_CASE("heev") {
    SECTION("float") {
        heev_test<std::complex<float>>();
    }
    SECTION("double") {
        heev_test<std::complex<double>>();
    }
}

template <typename T>
void lassq_test() {
    using namespace einsums;
    using namespace einsums::linear_algebra;

    auto A       = create_random_tensor<T>("a", 10);
    auto scale   = RemoveComplexT<T>{1.0};
    auto result  = RemoveComplexT<T>{0.0};
    auto result0 = RemoveComplexT<T>{0.0};

    einsums::linear_algebra::sum_square(A, &scale, &result);

    for (int i = 0; i < 10; i++) {
        if constexpr (IsComplexV<T>) {
            result0 += A(i).real() * A(i).real() + A(i).imag() * A(i).imag();
        } else {
            result0 += A(i) * A(i);
        }
    }

    CHECK_THAT(result, Catch::Matchers::WithinAbs(result0, 0.00001));
}

TEST_CASE("sum_square") {
    SECTION("float") {
        lassq_test<float>();
    }
    SECTION("double") {
        lassq_test<double>();
    }
    SECTION("complex_float") {
        lassq_test<std::complex<float>>();
    }
    SECTION("complex_double") {
        lassq_test<std::complex<double>>();
    }
}

template <NotComplex T>
void norm_test() {
    using namespace einsums;
    using namespace einsums::linear_algebra;
    using namespace einsums::tensor_algebra;
    using namespace einsums::tensor_algebra::index;

    auto A = create_tensor<T>("a", 9, 9);

    A.vector_data() =
        VectorData<T>{1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0,  0.0,  0.0, 4.0, 1.0, 1.0,
                      1.0, 1.0, 1.0, 0.0, 0.0, 0.0, 0.0, 5.0, 1.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 0.0,  0.0,  6.0, 1.0, 1.0, 1.0,
                      1.0, 1.0, 0.0, 0.0, 0.0, 0.0, 7.0, 1.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 0.0, 0.0,  8.0,  1.0, 1.0, 1.0, 1.0,
                      0.0, 0.0, 0.0, 0.0, 0.0, 9.0, 1.0, 1.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 10.0, 11.0, 12.0};

    T result = einsums::linear_algebra::norm(Norm::One, A);

    CHECK_THAT(result, Catch::Matchers::WithinRel(33.0, 0.1));
}

TEST_CASE("norm") {
    norm_test<float>();
    norm_test<double>();
}

template <typename T>
void getrf_and_getri_test() {
    using namespace einsums;
    using namespace einsums::linear_algebra;
    using namespace einsums::tensor_algebra;
    using namespace einsums::tensor_algebra::index;

    auto                  A = create_tensor<T>("A", 4, 4);
    std::vector<blas_int> pivot(4);

    A.vector_data() =
        VectorData<T>{1.80, 2.88, 2.05, -0.89, 5.25, -2.95, -0.95, -3.80, 1.58, -2.69, -2.90, -1.04, -1.11, -0.66, -0.59, 0.80};

    einsums::linear_algebra::getrf(&A, &pivot);
    einsums::linear_algebra::getri(&A, pivot);

    CHECK_THAT(A.vector_data(),
               Catch::Matchers::Approx(VectorData<T>{1.77199817, 0.57569082, 0.08432537, 4.81550236, -0.11746607, -0.44561501, 0.41136261,
                                                     -1.71258093, 0.17985639, 0.45266204, -0.66756530, 1.48240005, 2.49438204, 0.76497689,
                                                     -0.03595380, 7.61190029})
                   .margin(0.01));
}

TEST_CASE("getrf_getri") {
    getrf_and_getri_test<float>();
    getrf_and_getri_test<double>();
}

TEMPLATE_TEST_CASE("dot", "[linear-algebra]", float, double) {
    using namespace einsums;
    using namespace einsums::linear_algebra;

    constexpr int size = 10;

    SECTION("Rank 1 tensors") {
        Tensor<TestType, 1> A = create_random_tensor<TestType>("A", size);
        Tensor<TestType, 1> B = create_random_tensor<TestType>("B", size);

        TestType test{0.0};

        for (int i = 0; i < size; i++) {
            test += A(i) * B(i);
        }

        auto dot_res = dot(A, B);

        REQUIRE_THAT(dot_res, einsums::WithinStrict(test, TestType{100000.0}));
    }

    SECTION("Rank 1 tensor views") {
        Tensor<TestType, 2> A = create_random_tensor<TestType>("A", size, size);
        Tensor<TestType, 2> B = create_random_tensor<TestType>("B", size, size);

        for (int i = 0; i < size; i++) {
            auto A_view = A(AllT(), i);
            auto B_view = B(i, AllT());

            TestType test{0.0};

            for (int j = 0; j < size; j++) {
                test += A(j, i) * B(i, j);
            }

            auto dot_res = dot(A_view, B_view);

            REQUIRE_THAT(dot_res, einsums::WithinStrict(test, TestType{100000.0}));
        }
    }

    SECTION("Rank 2 tensors") {
        Tensor<TestType, 2> A = create_random_tensor<TestType>("A", size, size);
        Tensor<TestType, 2> B = create_random_tensor<TestType>("B", size, size);

        TestType test{0.0};

        for (int i = 0; i < size; i++) {
            for (int j = 0; j < size; j++) {
                test += A(i, j) * B(i, j);
            }
        }

        auto dot_res = dot(A, B);

        REQUIRE_THAT(dot_res, einsums::WithinStrict(test, TestType{100000.0}));
    }

    // SECTION("Rank 2 tensor views") {
    //     Tensor<TestType, 2> A = create_random_tensor<TestType>("A", size, size);
    //     Tensor<TestType, 2> B = create_random_tensor<TestType>("B", size, size);

    //     for(int i = 0; i < size; i++) {
    //         auto A_view = A(AllT(), Range{i, i + 1});
    //         auto B_view = B(AllT(), Range{i, i + 1});

    //         TestType test{0.0};

    //         for(int j = 0; j < size; j++) {
    //             test += A(j, i) * B(i, j);
    //         }

    //         auto dot_res = dot(A_view, B_view);

    //         REQUIRE(std::abs(dot_res - test) < static_cast<TestType>(EINSUMS_ZERO));
    //     }
    // }
}

TEST_CASE("pow") {
    using namespace einsums;
    using namespace einsums::linear_algebra;

    using TestType = double;

    SECTION("Integer power") {

        Tensor<TestType, 2> A = create_random_tensor<TestType>("A", 10, 10);
        // Can only handle symmetric matrices.
        for (int i = 0; i < A.dim(0); i++) {
            for (int j = 0; j < i; j++) {
                A(i, j) = A(j, i);
            }
        }

        Tensor<TestType, 2> B = einsums::linear_algebra::pow(A, TestType{2.0});
        Tensor<TestType, 2> C = create_tensor_like(A);

        gemm<false, false>(1.0, A, A, 0.0, &C);

        for (int i = 0; i < A.dim(0); i++) {
            for (int j = 0; j < A.dim(1); j++) {
                CHECK_THAT(B(i, j), Catch::Matchers::WithinAbs(C(i, j), TestType{1e-6}));
            }
        }
    }

    SECTION("Non-integer power") {
        constexpr int       size = 3;
        Tensor<TestType, 2> A{"A", size, size};
        Tensor<TestType, 2> B{"B", size, size};
        Tensor<TestType, 2> C{"C", size, size};

        std::default_random_engine engine(Catch::rngSeed());

        // Generate a positive definite matrix for A.
        auto normal = std::normal_distribution<TestType>(1, 3);

        // Set up the diagonal.
        Tensor<TestType, 1> Evals{"Evals", size};

        for (int i = 0; i < size; i++) {
            TestType val;

            // If we get zero, reroll until not zero.
            do {
                val = normal(engine);
            } while (val == TestType{0.0});

            Evals(i) = std::abs(val); // Can't have negatives in a positive definite matrix.
        }

        // Generate the eigenvectors.
        Tensor<TestType, 2> Evecs = create_random_tensor<TestType>("Evecs", size, size);

        // Set up the first vector.
        auto v1 = Evecs(AllT(), 0);

        auto norm = vec_norm(v1);
        v1 /= norm;

        // Orthogonalize.
        for (int i = 1; i < size; i++) {
            auto qi = Evecs(AllT(), i);
            for (int j = 0; j < i; j++) {
                auto qj = Evecs(AllT(), j);

                auto proj = true_dot(qi, qj);

                // axpy(-proj, qj, &qi);
                for (int k = 0; k < size; k++) {
                    qi(k) -= proj * qj(k);
                }
            }

            while (vec_norm(qi) < TestType{1e-6}) {
                qi = create_random_tensor<TestType>("new vec", size);
                for (int j = 0; j < i; j++) {
                    auto qj = Evecs(AllT(), j);

                    auto proj = true_dot(qi, qj);

                    // axpy(-proj, qj, &qi);
                    for (int k = 0; k < size; k++) {
                        qi(k) -= proj * qj(k);
                    }
                }
            }
            qi /= vec_norm(qi);
        }

        // Create the test tensors.
        auto Diag1 = diagonal(Evals);

        for (int i = 0; i < size; i++) {
            Evals(i) = std::pow(Evals(i), TestType{0.5});
        }
        auto Diag2 = diagonal(Evals);

        gemm<false, true>(TestType{1.0}, Diag1, Evecs, TestType{0.0}, &C);
        gemm<false, false>(TestType{1.0}, Evecs, C, TestType{0.0}, &A);
        gemm<false, true>(TestType{1.0}, Diag2, Evecs, TestType{0.0}, &C);
        gemm<false, false>(TestType{1.0}, Evecs, C, TestType{0.0}, &B);

        C.zero();
        C = pow(A, TestType{0.5});

        for (int i = 0; i < size; i++) {
            for (int j = 0; j < size; j++) {
                CHECK_THAT(B(i, j), Catch::Matchers::WithinAbs(C(i, j), TestType{1e-6}));
                CHECK_THAT(C(j, i), Catch::Matchers::WithinAbs(C(i, j), TestType{1e-6}));
                CHECK_THAT(B(j, i), Catch::Matchers::WithinAbs(B(i, j), TestType{1e-6}));
            }
        }
    }

    SECTION("Integer power blocks") {

        BlockTensor<TestType, 2> A("A", 4, 0, 1, 2);

        for (int i = 0; i < A.num_blocks(); i++) {
            if (A.block_dim(i) == 0) {
                continue;
            }
            A[i] = create_random_tensor("block", A.block_dim(i), A.block_dim(i));
            // Can only handle symmetric matrices.
            for (int j = 0; j < A.block_dim(i); j++) {
                for (int k = 0; k < j; k++) {
                    A[i](j, k) = A[i](k, j);
                }
            }
        }

        BlockTensor<TestType, 2> B = einsums::linear_algebra::pow(A, TestType{2.0});
        BlockTensor<TestType, 2> C = create_tensor_like(A);
        C.zero();

        gemm<false, false>(1.0, A, A, 0.0, &C);

        for (int i = 0; i < A.dim(0); i++) {
            for (int j = 0; j < A.dim(1); j++) {
                CHECK_THAT(B(i, j), Catch::Matchers::WithinAbs(C(i, j), TestType{1e-6}));
            }
        }
    }

    SECTION("Non-integer power blocks") {

        BlockTensor<TestType, 2> A("A", 4, 0, 1, 2);

        for (int i = 0; i < A.num_blocks(); i++) {
            if (A.block_dim(i) == 0) {
                continue;
            }
            A[i] = create_random_definite("block", A.block_dim(i), A.block_dim(i));
        }

        BlockTensor<TestType, 2> B      = einsums::linear_algebra::pow(A, TestType{0.5});
        Tensor<TestType, 2>      A_base = (Tensor<TestType, 2>)A;
        Tensor<TestType, 2>      C      = einsums::linear_algebra::pow(A_base, TestType{0.5});

        for (int i = 0; i < A.dim(0); i++) {
            for (int j = 0; j < A.dim(1); j++) {
                CHECK_THAT(B(i, j), Catch::Matchers::WithinAbs(C(i, j), TestType{1e-6}));
            }
        }
    }
}