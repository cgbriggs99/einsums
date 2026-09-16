//----------------------------------------------------------------------------------------------
// Copyright (c) The Einsums Developers. All rights reserved.
// Licensed under the MIT License. See LICENSE.txt in the project root for license information.
//----------------------------------------------------------------------------------------------

#pragma once

#include <Einsums/Assert.hpp>
#include <Einsums/BLAS.hpp>
#include <Einsums/Concepts/Complex.hpp>
#include <Einsums/Concepts/SubscriptChooser.hpp>
#include <Einsums/Concepts/TensorConcepts.hpp>
#include <Einsums/Profile/LabeledSection.hpp>
#include <Einsums/Tensor/Tensor.hpp>
#include <Einsums/TensorBase/IndexUtilities.hpp>
#include <Einsums/TensorUtilities/CreateTensorLike.hpp>

namespace einsums::linear_algebra::detail {

template <CoreBasicTensorConcept AType>
    requires(RankTensorConcept<AType, 1>)
void sum_square(AType const &a, RemoveComplexT<typename AType::ValueType> *scale, RemoveComplexT<typename AType::ValueType> *sumsq) {
    int n    = a.dim(0);
    int incx = a.stride(0);
    blas::lassq(n, a.data(), incx, scale, sumsq);
}

template <bool TransA, bool TransB, typename U, CoreBasicTensorConcept AType, CoreBasicTensorConcept BType, CoreBasicTensorConcept CType>
    requires requires {
        requires RankTensorConcept<AType, 2>;
        requires SameUnderlyingAndRank<AType, BType, CType>;
        requires(std::convertible_to<U, typename AType::ValueType>);
    }
void gemm(U const alpha, AType const &A, BType const &B, U const beta, CType *C) {
    auto m = C->dim(0), n = C->dim(1), k = TransA ? A.dim(0) : A.dim(1);
    auto lda = A.stride(0), ldb = B.stride(0), ldc = C->stride(0);

    blas::gemm(TransA ? 't' : 'n', TransB ? 't' : 'n', m, n, k, static_cast<typename AType::ValueType>(alpha), A.data(), lda, B.data(), ldb,
               static_cast<typename AType::ValueType>(beta), C->data(), ldc);
}

template <bool TransA, typename U, CoreBasicTensorConcept AType, CoreBasicTensorConcept XType, CoreBasicTensorConcept YType>
    requires requires {
        requires SameUnderlying<AType, XType, YType>;
        requires RankTensorConcept<AType, 2>;
        requires RankTensorConcept<XType, 1>;
        requires RankTensorConcept<YType, 1>;
        requires std::convertible_to<U, typename AType::ValueType>;
    }
void gemv(U const alpha, AType const &A, XType const &z, U const beta, YType *y) {
    auto m = A.dim(0), n = A.dim(1);
    auto lda  = A.stride(0);
    auto incx = z.stride(0);
    auto incy = y->stride(0);

    blas::gemv(TransA ? 't' : 'n', m, n, static_cast<typename AType::ValueType>(alpha), A.data(), lda, z.data(), incx,
               static_cast<typename AType::ValueType>(beta), y->data(), incy);
}

template <bool ComputeEigenvectors = true, CoreBasicTensorConcept AType, CoreBasicTensorConcept WType>
    requires requires {
        requires SameUnderlying<AType, WType>;
        requires RankTensorConcept<AType, 2>;
        requires RankTensorConcept<WType, 1>;
        requires NotComplex<AType>;
    }
void syev(AType *A, WType *W) {
    assert(A->dim(0) == A->dim(1));

    auto                                   n     = A->dim(0);
    auto                                   lda   = A->stride(0);
    int                                    lwork = 3 * n;
    std::vector<typename AType::ValueType> work(lwork);

    blas::syev(ComputeEigenvectors ? 'v' : 'n', 'u', n, A->data(), lda, W->data(), work.data(), lwork);
}

template <bool ComputeLeftRightEigenvectors = true, CoreBasicTensorConcept AType, CoreBasicTensorConcept WType>
    requires requires {
        requires std::is_same_v<AddComplexT<typename AType::ValueType>, typename WType::ValueType>;
        requires RankTensorConcept<AType, 2>;
        requires RankTensorConcept<WType, 1>;
    }
void geev(AType *A, WType *W, AType *lvecs, AType *rvecs) {
    EINSUMS_ASSERT(A->dim(0) == A->dim(1));
    EINSUMS_ASSERT(W->dim(0) == A->dim(0));

    using T = typename AType::ValueType;

    T          *l_data{nullptr}, *r_data{nullptr};
    blas::int_t ldvl = 1, ldvr = 1;

    char l_compute = 'N', r_compute = 'N';

    if constexpr (ComputeLeftRightEigenvectors) {
        if (lvecs != nullptr) {
            l_compute = 'V';
            l_data    = lvecs->data();
            ldvl      = lvecs->stride(0);
            EINSUMS_ASSERT(A->dim(0) == lvecs->dim(0));
            EINSUMS_ASSERT(A->dim(1) == lvecs->dim(1));
        }

        if (rvecs != nullptr) {
            r_compute = 'V';
            r_data    = rvecs->data();
            ldvr      = rvecs->stride(0);
            EINSUMS_ASSERT(A->dim(0) == rvecs->dim(0));
            EINSUMS_ASSERT(A->dim(1) == rvecs->dim(1));
        }
    }

    auto res = blas::geev(l_compute, r_compute, A->dim(0), A->data(), A->stride(0), W->data(), l_data, ldvl, r_data, ldvr);

    if (res < 0) {
        EINSUMS_THROW_EXCEPTION(std::invalid_argument, "The {} argument to geev was invalid!", print::ordinal(-res));
    } else if (res > 0) {
        EINSUMS_THROW_EXCEPTION(std::runtime_error, "The QR algorithm failed to converge for {} eigenvalues.", res);
    }
}

template <bool ComputeEigenvectors = true, CoreBasicTensorConcept AType, CoreBasicTensorConcept WType>
    requires requires {
        requires NotComplex<WType>;
        requires std::is_same_v<typename AType::ValueType, AddComplexT<typename WType::ValueType>>;
        requires MatrixConcept<AType>;
        requires VectorConcept<WType>;
    }
void heev(AType *A, WType *W) {
    EINSUMS_ASSERT(A->dim(0) == A->dim(1));

    auto                                   n     = A->dim(0);
    auto                                   lda   = A->stride(0);
    int                                    lwork = 2 * n;
    std::vector<typename AType::ValueType> work(lwork);
    std::vector<typename WType::ValueType> rwork(3 * n);

    blas::heev(ComputeEigenvectors ? 'v' : 'n', 'u', n, A->data(), lda, W->data(), work.data(), lwork, rwork.data());
}

template <CoreBasicTensorConcept AType, CoreBasicTensorConcept BType>
    requires requires {
        requires SameUnderlyingAndRank<AType, BType>;
        requires MatrixConcept<AType>;
    }
auto gesv(AType *A, BType *B) -> int {
    auto n   = A->dim(0);
    auto lda = A->dim(0);
    auto ldb = B->dim(1);

    auto nrhs = B->dim(0);

    int                      lwork = n;
    std::vector<blas::int_t> ipiv(lwork);

    int info = blas::gesv(n, nrhs, A->data(), lda, ipiv.data(), B->data(), ldb);
    return info;
}

template <CoreBasicTensorConcept AType>
void scale(typename AType::ValueType scale, AType *A) {
    blas::scal(A->dim(0) * A->stride(0), scale, A->data(), 1);
}

template <CoreBasicTensorConcept AType>
    requires(MatrixConcept<AType>)
void scale_row(size_t row, typename AType::ValueType scale, AType *A) {
    blas::scal(A->dim(1), scale, A->data(row, 0ul), A->stride(1));
}

template <CoreBasicTensorConcept AType>
void scale_column(size_t col, typename AType::ValueType scale, AType *A) {
    blas::scal(A->dim(0), scale, A->data(0ul, col), A->stride(0));
}

template <CoreBasicTensorConcept AType, CoreBasicTensorConcept BType>
    requires requires {
        requires VectorConcept<AType>;
        requires SameUnderlyingAndRank<AType, BType>;
    }
auto dot(AType const &A, BType const &B) -> typename AType::ValueType {
    EINSUMS_ASSERT(A.dim(0) == B.dim(0));

    if (A.dim(0) == 0) {
        return typename AType::ValueType{0.0};
    }

    auto result = blas::dot(A.dim(0), A.data(), A.stride(0), B.data(), B.stride(0));
    return result;
}

template <CoreBasicTensorConcept AType, CoreBasicTensorConcept BType>
    requires requires {
        requires VectorConcept<AType>;
        requires SameRank<AType, BType>;
        requires !SameUnderlying<AType, BType>;
    }
auto dot(AType const &A, BType const &B) -> BiggestTypeT<typename AType::ValueType, typename BType::ValueType> {
    EINSUMS_ASSERT(A.dim(0) == B.dim(0));

    using OutType = BiggestTypeT<typename AType::ValueType, typename BType::ValueType>;

    OutType result = OutType{0.0};

    if (A.dim(0) == 0) {
        return result;
    }

    auto const *A_data   = A.data();
    auto const *B_data   = B.data();
    auto const  A_stride = A.stride(0);
    auto const  B_stride = B.stride(0);

    EINSUMS_OMP_SIMD
    for (size_t i = 0; i < A.dim(0); i++) {
        result += A_data[A_stride * i] * B_data[B_stride * i];
    }

    return result;
}

template <CoreBasicTensorConcept AType, CoreBasicTensorConcept BType>
    requires requires {
        requires SameUnderlyingAndRank<AType, BType>;
        requires !VectorConcept<AType>;
    }
auto dot(AType const &A, BType const &B) -> BiggestTypeT<typename AType::ValueType, typename BType::ValueType> {
    using T = BiggestTypeT<typename AType::ValueType, typename BType::ValueType>;

    if (A.full_view_of_underlying() && B.full_view_of_underlying()) {
        Dim<1> dim{1};

        for (size_t i = 0; i < AType::Rank; i++) {
            assert(A.dim(i) == B.dim(i));
            dim[0] *= A.dim(i);
        }

        return blas::dot(A.size(), A.data(), A.stride(AType::Rank - 1), B.data(), B.stride(AType::Rank - 1));
    } else {
        auto dims = A.dims();

        std::array<size_t, AType::Rank> strides;
        strides[AType::Rank - 1] = 1;
        std::array<size_t, AType::Rank> index;

        for (int i = AType::Rank - 1; i > 0; i--) {
            strides[i - 1] = strides[i] * dims[i];
        }

        T out{0.0};

        for (size_t sentinel = 0; sentinel < strides[0] * dims[0]; sentinel++) {
            sentinel_to_indices(sentinel, strides, index);
            out += subscript_tensor(A, index) * subscript_tensor(B, index);
        }

        return out;
    }
}

template <CoreBasicTensorConcept AType, CoreBasicTensorConcept BType>
    requires requires {
        requires SameUnderlyingAndRank<AType, BType>;
        requires VectorConcept<AType>;
    }
auto true_dot(AType const &A, BType const &B) -> typename AType::ValueType {
    assert(A.dim(0) == B.dim(0));

    if (A.dim(0) == 0) {
        return typename AType::ValueType{0.0};
    }

    if constexpr (IsComplexV<AType>) {
        return blas::dotc(A.dim(0), A.data(), A.stride(0), B.data(), B.stride(0));
    } else {
        return blas::dot(A.dim(0), A.data(), A.stride(0), B.data(), B.stride(0));
    }
}

template <CoreBasicTensorConcept AType, CoreBasicTensorConcept BType>
    requires requires {
        requires VectorConcept<AType>;
        requires SameRank<AType, BType>;
        requires !SameUnderlying<AType, BType>;
    }
auto true_dot(AType const &A, BType const &B) -> BiggestTypeT<typename AType::ValueType, typename BType::ValueType> {
    EINSUMS_ASSERT(A.dim(0) == B.dim(0));

    using OutType = BiggestTypeT<typename AType::ValueType, typename BType::ValueType>;

    OutType result = OutType{0.0};

    if (A.dim(0) == 0) {
        return result;
    }

    auto const *A_data   = A.data();
    auto const *B_data   = B.data();
    auto const  A_stride = A.stride(0);
    auto const  B_stride = B.stride(0);

    EINSUMS_OMP_SIMD
    for (size_t i = 0; i < A.dim(0); i++) {
        if constexpr (IsComplexV<typename AType::ValueType>) {
            result += A_data[A_stride * i].conj() * B_data[B_stride * i];
        } else {
            result += A_data[A_stride * i] * B_data[B_stride * i];
        }
    }

    return result;
}

template <CoreBasicTensorConcept AType, CoreBasicTensorConcept BType>
    requires requires {
        requires SameRank<AType, BType>;
        requires !VectorConcept<AType>;
    }
auto true_dot(AType const &A, BType const &B) -> BiggestTypeT<typename AType::ValueType, typename BType::ValueType> {
    if (A.full_view_of_underlying() && B.full_view_of_underlying()) {
        if constexpr (IsComplexV<AType>) {
            return blas::dotc(A.size(), A.data(), A.stride(AType::Rank - 1), B.data(), B.stride(AType::Rank - 1));
        } else {
            return blas::dot(A.size(), A.data(), A.stride(AType::Rank - 1), B.data(), B.stride(AType::Rank - 1));
        }
    } else {
        auto dims = A.dims();

        std::array<size_t, AType::Rank> strides;
        strides[AType::Rank - 1] = 1;
        std::array<size_t, AType::Rank> index;

        for (int i = AType::Rank - 1; i > 0; i--) {
            strides[i - 1] = strides[i] * dims[i];
        }

        BiggestTypeT<typename AType::ValueType, typename BType::ValueType> out{0.0};

        for (size_t sentinel = 0; sentinel < strides[0] * dims[0]; sentinel++) {
            sentinel_to_indices(sentinel, strides, index);

            if constexpr (IsComplexV<AType>) {
                out += std::conj(subscript_tensor(A, index)) * subscript_tensor(B, index);
            } else {
                out += subscript_tensor(A, index) * subscript_tensor(B, index);
            }
        }

        return out;
    }
}

template <CoreBasicTensorConcept AType, CoreBasicTensorConcept BType, CoreBasicTensorConcept CType>
    requires SameRank<AType, BType, CType>
auto dot(AType const &A, BType const &B, CType const &C)
    -> BiggestTypeT<typename AType::ValueType, typename BType::ValueType, typename CType::ValueType> {
    Dim<1> dim{1};
    using T = BiggestTypeT<typename AType::ValueType, typename BType::ValueType, typename CType::ValueType>;

    for (size_t i = 0; i < AType::Rank; i++) {
        assert(A.dim(i) == B.dim(i) && A.dim(i) == C.dim(i));
        dim[0] *= A.dim(i);
    }

    auto vA = TensorView<T, 1>(const_cast<AType &>(A), dim);
    auto vB = TensorView<T, 1>(const_cast<BType &>(B), dim);
    auto vC = TensorView<T, 1>(const_cast<CType &>(C), dim);

    T result{0};
#pragma omp parallel for reduction(+ : result)
    for (size_t i = 0; i < dim[0]; i++) {
        result += subscript_tensor(vA, i) * subscript_tensor(vB, i) * subscript_tensor(vC, i);
    }
    return result;
}

template <CoreBasicTensorConcept XType, CoreBasicTensorConcept YType>
    requires SameUnderlyingAndRank<XType, YType>
void axpy(typename XType::ValueType alpha, XType const &X, YType *Y) {
    blas::axpy(X.dim(0) * X.stride(0), alpha, X.data(), 1, Y->data(), 1);
}

template <CoreBasicTensorConcept XType, CoreBasicTensorConcept YType>
    requires SameUnderlyingAndRank<XType, YType>
void axpby(typename XType::ValueType alpha, XType const &X, typename YType::ValueType beta, YType *Y) {
    blas::axpby(X.dim(0) * X.stride(0), alpha, X.data(), 1, beta, Y->data(), 1);
}

template <CoreBasicTensorConcept AType, CoreBasicTensorConcept XYType>
    requires requires {
        requires MatrixConcept<AType>;
        requires VectorConcept<XYType>;
        requires SameUnderlying<AType, XYType>;
    }
void ger(typename XYType::ValueType alpha, XYType const &X, XYType const &Y, AType *A) {
    blas::ger(X.dim(0), Y.dim(0), alpha, X.data(), X.stride(0), Y.data(), Y.stride(0), A->data(), A->stride(0));
}

template <bool TransA, bool TransB, CoreBasicTensorConcept AType, CoreBasicTensorConcept BType, CoreBasicTensorConcept CType>
    requires requires {
        requires SameUnderlyingAndRank<AType, BType, CType>;
        requires MatrixConcept<AType>;
    }
void symm_gemm(AType const &A, BType const &B, CType *C) {
    int temp_rows, temp_cols;
    if constexpr (TransA && TransB) {
        EINSUMS_ASSERT(B.dim(0) == A.dim(0) && A.dim(1) == B.dim(0) && C->dim(0) == B.dim(1) && C->dim(1) == B.dim(1));
    } else if constexpr (TransA && !TransB) {
        EINSUMS_ASSERT(B.dim(1) == A.dim(0) && A.dim(1) == B.dim(1) && C->dim(0) == B.dim(0) && C->dim(1) == B.dim(0));
    } else if constexpr (!TransA && TransB) {
        EINSUMS_ASSERT(B.dim(0) == A.dim(1) && A.dim(0) == B.dim(0) && C->dim(0) == B.dim(1) && C->dim(1) == B.dim(1));
    } else {
        EINSUMS_ASSERT(B.dim(1) == A.dim(1) && A.dim(0) == B.dim(1) && C->dim(0) == B.dim(0) && C->dim(1) == B.dim(0));
    }

    if constexpr (TransA) {
        temp_rows = A.dim(1);
    } else {
        temp_rows = A.dim(0);
    }

    if constexpr (TransB) {
        temp_cols = B.dim(0);
    } else {
        temp_cols = B.dim(1);
    }

    *C = typename CType::ValueType(0.0);

    Tensor<typename AType::ValueType, 2> temp{"temp", temp_rows, temp_cols};

    gemm<TransA, TransB>(typename AType::ValueType{1.0}, A, B, typename CType::ValueType{0.0}, &temp);
    gemm<!TransB, false>(typename AType::ValueType{1.0}, B, temp, typename CType::ValueType{0.0}, C);
}

template <MatrixConcept TensorType>
    requires(CoreBasicTensorConcept<TensorType>)
auto getrf(TensorType *A, std::vector<blas::int_t> *pivot) -> int {
    LabeledSection0();

    if (pivot->size() < std::min(A->dim(0), A->dim(1))) {
        // println("getrf: resizing pivot vector from {} to {}", pivot->size(), std::min(A->dim(0), A->dim(1)));
        pivot->resize(std::min(A->dim(0), A->dim(1)));
    }
    int result = blas::getrf(A->dim(0), A->dim(1), A->data(), A->stride(0), pivot->data());

    if (result < 0) {
        EINSUMS_LOG_WARN("getrf: argument {} has an invalid value", -result);
        abort();
    }

    return result;
}

template <MatrixConcept TensorType>
    requires(CoreBasicTensorConcept<TensorType>)
auto getri(TensorType *A, std::vector<blas::int_t> const &pivot) -> int {
    LabeledSection0();

    int result = blas::getri(A->dim(0), A->data(), A->stride(0), pivot.data());

    if (result < 0) {
        EINSUMS_LOG_WARN("getri: argument {} has an invalid value", -result);
    }
    return result;
}

template <CoreBasicTensorConcept AType, CoreBasicTensorConcept BType, CoreBasicTensorConcept CType>
    requires SameUnderlyingAndRank<AType, BType, CType>
void direct_product(typename AType::ValueType alpha, AType const &A, BType const &B, typename CType::ValueType beta, CType *C) {
    LabeledSection0();

    using T = typename AType::ValueType;

    // Ensure the various tensors passed in are the same dimensionality
    if (((C->dims() != A.dims()) || C->dims() != B.dims())) {
        EINSUMS_THROW_EXCEPTION(dimension_error, "direct_product: at least one tensor does not have same dimensionality as destination");
    }

    // Horrible hack. For some reason, in the for loop below, the result could be
    // NAN if the target_value is initially a trash value.
    if constexpr (IsComplexV<typename CType::ValueType>) {
        if (beta == typename CType::ValueType{0.0, 0.0}) {
            C->zero();
        }
    } else {
        if (beta == T(0)) {
            C->zero();
        }
    }

    std::array<size_t, AType::Rank> index_strides;

    size_t elements = dims_to_strides(A.dims(), index_strides);

    if (!A.full_view_of_underlying() || !B.full_view_of_underlying() || !C->full_view_of_underlying()) {
        EINSUMS_OMP_PARALLEL_FOR
        for (size_t item = 0; item < elements; item++) {

            size_t A_ord, B_ord, C_ord;

            sentinel_to_sentinels(item, index_strides, A.strides(), A_ord, B.strides(), B_ord, C->strides(), C_ord);

            C->data()[C_ord] = beta * C->data()[C_ord] + alpha * (A.data()[A_ord] * B.data()[B_ord]);
        }
    } else {
        EINSUMS_OMP_PARALLEL_FOR_SIMD
        for (size_t item = 0; item < elements; item++) {
            C->data()[item] = beta * C->data()[item] + alpha * (A.data()[item] * B.data()[item]);
        }
    }
}

// template <CoreBasicTensorConcept AType>
// auto sqrt(AType const &a, typename AType::ValueType cutoff = std::numeric_limits<typename AType::ValueType>::epsilon())
//     -> Tensor<typename AType::ValueType, 2> {
//
//     assert(a.dim(0) == a.dim(1));
//
//     using T = typename AType::ValueType;
//
//     if constexpr (IsComplexV<typename AType::ValueType>) {
//         // Special algorithm for a complex matrix. See Björk and Hammerling, 1983
//
//         // First, compute the Schur canonical form of A.
//         Tensor<T, 2> X{a.dims()}, Q{a.dims()}, U{a.dims()};
//         Tensor<T, 1> eigvals{"eigenvalues", a.dim(0)};
//
//         size_t const n = a.dim(0);
//
//         // Transpose the tensor.
//         for (size_t i = 0; i < n; i++) {
//             for (size_t j = 0; j < n; j++) {
//                 X(i, j) = a(j, i);
//             }
//         }
//
//         blas::int_t sdim;
//
//         auto res = blas::gees('V', n, X.data(), X.stride(0), &sdim, eigvals.data(), Q.data(), Q.stride(0));
//
//         if (res < 0) {
//             EINSUMS_THROW_EXCEPTION(std::runtime_error, "The {} argument to [c,z]gees had an invalid value!", print::ordinal(-res));
//         } else if (res > 0 && res <= n) {
//             EINSUMS_THROW_EXCEPTION(std::runtime_error, "The QR algorithm failed to converge!");
//         } else if (res == n + 1) {
//             EINSUMS_THROW_EXCEPTION(std::runtime_error, "Could not reorder eigenvalues! This should not be thrown!");
//         } else if (res == n + 2) {
//             EINSUMS_LOG_WARN("Roundoff errors have changed eigenvalues after reordering in gees. Results may be unstable!");
//         }
//
//         // Sort the matrix so that all of the zero eigenvalues are at the back.
//         size_t num_zeros = 0;
//
//         for (size_t i = 0; i < n; i++) {
//             if (std::abs(eigvals(i)) < cutoff) {
//                 size_t const end = n - 1 - num_zeros;
//                 // Swap the eigenvalue to the end of the list.
//                 std::swap(eigvals(i), eigvals(end));
//
//                 // Now, perform the swap of the Schur vectors.
//                 T const a = X(i, i), b = X(i, end), c = X(end, end);
//                 T const R   = std::sqrt(std::abs(a - c) * std::abs(a - c) + std::abs(b) * std::abs(b));
//                 T const cos = b / R, sin = (a - c) / R;
//
//                 T const p = Q(i, i), q = Q(end, i), r = Q(i, end), s = Q(end, end);
//
//                 Q(i, i)     = p * cos - q * sin;
//                 Q(end, i)   = p * sin + q * cos;
//                 Q(i, end)   = r * cos - s * sin;
//                 Q(end, end) = r * sin + s * cos;
//
//                 // X(end, i) and X(i, end) are not affected by design.
//                 std::swap(X(i, i), X(end, end));
//                 num_zeros++;
//             }
//         }
//
//         // Then, check to see if the matrix does in fact have a square root.
//         if (num_zeros > 1) {
//             for (size_t i = n - num_zeros; i < n; i++) {
//                 for (size_t j = i + 1; j < n; j++) {
//                     if (std::abs(X(j, i)) > cutoff) {
//                         EINSUMS_THROW_EXCEPTION(std::domain_error, "Matrix does not have a square root due to having too many zero "
//                                                                    "eigenvalues that are off-balanced by non-zero off-diagonal
//                                                                    elements!");
//                     }
//                 }
//             }
//         }
//
//         // Now, iterate until convergence.
//         bool converged = false;
//
//         U.zero();
//
//         // Initial guess is a diagonal matrix whose entries are the square roots of the eigenvalues.
//         // The diagonal entries never change.
//         for (size_t i = 0; i < n; i++) {
//             U(i, i) = std::sqrt(X(i, i));
//         }
//
//         size_t const zero_tail = n - num_zeros;
//
//         while (!converged) {
//             converged = true;
//             // Convert the equations to column-major form.
//             for (size_t j = 0; j < n; j++) {
//                 for (size_t i = 0; i < j; i++) {
//                     if (i >= zero_tail) {
//                         // Special rule if there are close eigenvalues.
//                         U(j, i) = T{0.0};
//                     } else {
//                         // Compute the new value.
//                         T new_value{X(j, i)};
//                         for (size_t k = i + 1; k < j; k++) {
//                             new_value -= U(k, i) * U(j, k);
//                         }
//                         new_value /= U(i, i) + U(j, j);
//
//                         // Check convergence.
//                         if (std::abs(new_value - U(j, i)) > cutoff) {
//                             converged = false;
//                         }
//                     }
//                 }
//             }
//         }
//
//         // Back-transform to the solution.
//         blas::gemm('N', 'N', n, n, n, T{1.0}, Q.data(), Q.stride(0), U.data(), U.stride(0), T{0.0}, X.data(), X.stride(0));
//         blas::gemm('N', 'C', n, n, n, T{1.0}, X.data(), X.stride(0), Q.data(), Q.stride(0), T{0.0}, U.data(), U.stride(0));
//
//         // Finally, transpose.
//         for (size_t i = 0; i < n; i++) {
//             for (size_t j = 0; j < n; j++) {
//                 X(i, j) = U(j, i);
//             }
//         }
//
//         return X;
//     } else {
//         // Otherwise, for real values, use Sylvester decomposition.
//
//         // Start with a guess.
//         Tensor<T, 2> X{a.dims()}, Q{a.dims()}, S{a.dims()}, temp{a.dims()}, C{a.dims()};
//         Tensor<T, 1> real_eig{"real eigenvalue components", a.dim(0)}, imag_eig{"imaginary eigenvalue components", a.dim(0)};
//
//         X.zero();
//
//         size_t const n = a.dim(0);
//
//         for (size_t i = 0; i < n; i++) {
//             X(i, i) = T{1.0};
//         }
//
//         bool converged = false;
//
//         std::vector<RemoveComplexT<typename AType::ValueType>> work(4 * a.dim(0), 0.0);
//         size_t                                                 iter = 0;
//
//         while (!converged) {
//             // Decompose X.
//             blas::int_t sdim;
//             S        = X;
//             auto res = blas::gees('V', n, S.data(), S.stride(0), &sdim, real_eig.data(), imag_eig.data(), Q.data(), Q.stride(0));
//
//             if (res < 0) {
//                 EINSUMS_THROW_EXCEPTION(std::runtime_error, "The {} argument to [s,d]gees had an invalid value!", print::ordinal(-res));
//             } else if (res > 0 && res <= n) {
//                 EINSUMS_THROW_EXCEPTION(std::runtime_error, "The QR algorithm failed to converge!");
//             } else if (res == n + 1) {
//                 EINSUMS_THROW_EXCEPTION(std::runtime_error, "Could not reorder eigenvalues! This should not be thrown!");
//             } else if (res == n + 2) {
//                 EINSUMS_LOG_WARN("Roundoff errors have changed eigenvalues after reordering in gees. Results may be unstable!");
//             }
//
//             // Compute the C matrix.
//             gemm<true, false>(T{1.0}, Q, a, T{0.0}, &temp); // Do this in case A has non-unit stride.
//
//             // Everything has unit stride here, so use the raw BLAS calls.
//             blas::gemm('N', 'N', n, n, n, T{1.0}, S.data(), S.stride(0), S.data(), S.stride(0), T{0.0}, C.data(), C.stride(0));
//             blas::gemm('T', 'T', n, n, n, T{1.0}, temp.data(), temp.stride(0), Q.data(), Q.stride(0), T{-1.0}, C.data(), C.stride(0));
//
//             // Solve for the transformed error matrix.
//             T scale;
//             res = blas::trsyl('N', 'N', 1, n, n, S.data(), S.stride(0), S.data(), S.stride(0), C.data(), C.stride(0), &scale);
//
//             if (res < 0) {
//                 EINSUMS_THROW_EXCEPTION(std::runtime_error, "The {} argument to trsyl had an invalid value!", print::ordinal(-res));
//             } else if (res == 1) {
//                 EINSUMS_LOG_INFO("The matrices passed to trsyl had very close eigenvalues. The matrices were perturbed. This does not "
//                                  "affect the matrices themselves. Loss of precision may have occurred.");
//             }
//
//             // Back-transform to get the error matrix.
//             blas::gemm('N', 'N', n, n, n, T{1.0}, Q.data(), Q.stride(0), C.data(), C.stride(0), T{0.0}, temp.data(), temp.stride(0));
//             blas::gemm('N', 'T', n, n, n, T{1.0}, temp.data(), temp.stride(0), Q.data(), Q.stride(0), T{0.0}, C.data(), C.stride(0));
//
//             // Check for convergence.
//             auto conv_check = blas::lange('M', n, n, C.data(), C.stride(0), work.data()) / scale;
//             iter++;
//             EINSUMS_LOG_DEBUG("{} Convergence criterion: {}", iter, conv_check);
//             if (conv_check < cutoff) {
//                 converged = true;
//             }
//
//             axpy(T{1.0} / scale, C, &X);
//         }
//
//         // Transpose the output.
//         for (size_t i = 0; i < n; i++) {
//             for (size_t j = 0; j < n; j++) {
//                 C(j, i) = X(i, j);
//             }
//         }
//         return C;
//     }
// }

template <CoreBasicTensorConcept AType>
auto sqrt(AType const                              &a,
          RemoveComplexT<typename AType::ValueType> cutoff = std::numeric_limits<RemoveComplexT<typename AType::ValueType>>::epsilon())
    -> Tensor<typename AType::ValueType, 2>;

extern template auto EINSUMS_EXPORT sqrt(Tensor<float, 2> const &a, float cutoff = std::numeric_limits<float>::epsilon())
    -> Tensor<float, 2>;

extern template auto EINSUMS_EXPORT sqrt(Tensor<double, 2> const &a, double cutoff = std::numeric_limits<double>::epsilon())
    -> Tensor<double, 2>;

extern template auto EINSUMS_EXPORT sqrt(Tensor<std::complex<float>, 2> const &a, float cutoff = std::numeric_limits<float>::epsilon())
    -> Tensor<std::complex<float>, 2>;

extern template auto EINSUMS_EXPORT sqrt(Tensor<std::complex<double>, 2> const &a, double cutoff = std::numeric_limits<double>::epsilon())
    -> Tensor<std::complex<double>, 2>;

template <CoreBasicTensorConcept AType>
    requires MatrixConcept<AType>
auto real_pow(AType const &a, RemoveComplexT<typename AType::ValueType> alpha,
              typename AType::ValueType cutoff = std::numeric_limits<typename AType::ValueType>::epsilon())
    -> Tensor<typename AType::ValueType, 2> {

    using T = typename AType::ValueType;

    // Start by extracting the components of alpha.

    RemoveComplexT<T> mantissa;
    int               exponent;

    mantissa = frexp(alpha, &exponent);

    EINSUMS_LOG_DEBUG("Mantissa: {}, exponent: {}", mantissa, exponent);

    Tensor<T, 2> out{a.dims()}, temp{a.dims()}, mult{a.dims()};

    for (size_t i = 0; i < a.dim(0); i++) {
        for (size_t j = 0; j < a.dim(1); j++) {
            if (i == j) {
                out(i, j) = T{1.0};
            } else {
                out(i, j) = T{0.0};
            }
        }
    }
    if (alpha == T{0.0}) {
        return out;
    }

    mult = a;

    if (mantissa < T{0.0}) {
        std::vector<blas::int_t> pivot(a.dim(0));
        int                      result = getrf(&mult, &pivot);
        if (result > 0) {
            EINSUMS_THROW_EXCEPTION(std::runtime_error,
                                    "getrf: the ({}, {}) element of the factor U or L is zero, and the inverse could not be computed",
                                    result, result);
        }

        result = getri(&mult, pivot);
        if (result > 0) {
            EINSUMS_THROW_EXCEPTION(std::runtime_error,
                                    "getri: the ({}, {}) element of the factor U or L i zero, and the inverse could not be computed",
                                    result, result);
        }

        mantissa = std::abs(mantissa);
    }

    // Next, as long as the mantissa is not zero, we use a recurrence relation.
    while (mantissa != T{0.0}) {
        EINSUMS_LOG_DEBUG("Mantissa: {}, exponent: {}", mantissa, exponent);
        if (mantissa >= T{1.0}) {
            gemm<false, false>(T{0.5}, mult, out, T{0.0}, &temp);
            gemm<false, false>(T{0.5}, out, mult, T{1.0}, &temp); // For stability.
            out = temp;
            mantissa -= T{1.0};
        }

        if (exponent >= 0) {
            gemm<false, false>(T{1.0}, mult, mult, T{0.0}, &temp);
            mult = temp;
            exponent--;
        } else if (exponent < 0) {
            mult = sqrt(mult, cutoff);
            exponent += 1;
        }

        mantissa = std::ldexp(mantissa, 1);
        exponent--;
    }
    EINSUMS_LOG_DEBUG("Mantissa: {}, exponent: {}", mantissa, exponent);

    // Then, handle the 2^expo. For positive exponents, this is repeated squaring.
    while (exponent > 0) {
        EINSUMS_LOG_DEBUG("Mantissa: {}, exponent: {}", mantissa, exponent);
        gemm<false, false>(T{1.0}, out, out, T{0.0}, &temp);
        out = temp;
        exponent--;
    }
    EINSUMS_LOG_DEBUG("Mantissa: {}, exponent: {}", mantissa, exponent);

    // For negative arguments, it's square roots.
    while (exponent < 0) {
        EINSUMS_LOG_DEBUG("Mantissa: {}, exponent: {}", mantissa, exponent);
        out = sqrt(out, cutoff);
        exponent++;
    }
    EINSUMS_LOG_DEBUG("Mantissa: {}, exponent: {}", mantissa, exponent);
    return out;
}

template <CoreBasicTensorConcept AType>
    requires MatrixConcept<AType>
auto pow(AType const &a, typename AType::ValueType alpha,
         typename AType::ValueType cutoff = std::numeric_limits<typename AType::ValueType>::epsilon())
    -> Tensor<typename AType::ValueType, 2> {
    assert(a.dim(0) == a.dim(1));

    using T = typename AType::ValueType;

    if constexpr (IsComplexV<T>) {
        // If we have an imaginary part, deal with it.
        if (std::imag(alpha) == RemoveComplexT<T>{0.0}) {
            return real_pow(a, alpha, cutoff);
        } else {
            // If we have a complex power, there's nothing else we can really do other than diagonalize the matrix.

            Tensor<T, 2> A_temp = a, L{a.dims()}, R{a.dims()};
            Tensor<T, 1> eigvals{"eigenvalues", a.dim(0)};

            geev<true>(&A_temp, &eigvals, &L, &R);

            // Compute the actual eigenvectors that don't have the weird scaling applied.
            for (size_t i = 0; i < a.dim(1); i++) {
                T scale = true_dot(L(All, i), R(All, i));

                T lscale, rscale;

                lscale = std::sqrt(scale);
                rscale = lscale;

                for (size_t j = 0; j < a.dim(0); j++) {
                    L(j, i) /= lscale;
                    R(j, i) /= rscale;
                }
            }

            // Compute the output.
            for (size_t i = 0; i < a.dim(0); i++) {
                scale_row(i, std::pow(eigvals(i), alpha), &L);
            }

            gemm<true, false>(T{1.0}, L, R, T{0.0}, &A_temp);
            return A_temp;
        }
    } else {
        return real_pow(a, alpha, cutoff);
    }
}

template <CoreBasicTensorConcept AType, std::integral Int>
    requires MatrixConcept<AType>
auto pow(AType const &a, Int alpha, typename AType::ValueType cutoff = std::numeric_limits<typename AType::ValueType>::epsilon())
    -> Tensor<typename AType::ValueType, 2> {
    assert(a.dim(0) == a.dim(1));

    using T    = typename AType::ValueType;
    using UInt = std::make_unsigned_t<Int>;

    if (alpha < 0) {
        Tensor<T, 2> inv = a;

        std::vector<blas::int_t> pivot(a.dim(0));
        int                      result = getrf(&inv, &pivot);
        if (result > 0) {
            EINSUMS_THROW_EXCEPTION(std::runtime_error,
                                    "getrf: the ({}, {}) element of the factor U or L is zero, and the inverse could not be computed",
                                    result, result);
        }

        result = getri(&inv, pivot);
        if (result > 0) {
            EINSUMS_THROW_EXCEPTION(std::runtime_error,
                                    "getri: the ({}, {}) element of the factor U or L i zero, and the inverse could not be computed",
                                    result, result);
        }

        return pow(inv, static_cast<UInt>(-alpha), cutoff);
    } else {
        Tensor<T, 2> out{a.dims()}, temp{a.dims()}, pow_a{a.dims()};

        // Create the initial value, the identity matrix.
        out.zero();

        size_t const stride = out.stride(0) + out.stride(1);

        for (size_t i = 0; i < a.dim(0); i++) {
            out.data()[i * stride] = T{1.0};
        }

        pow_a = a;

        // Now, work through the binary representation of alpha.
        UInt cast_alpha = static_cast<UInt>(alpha);

        // Create the bit mask to find whether to square.
        constexpr UInt bit_mask   = 0x1;
        constexpr UInt max_cycles = 8 * sizeof(UInt);

        for (unsigned int cycle = 0; cycle < max_cycles && cast_alpha != 0; cycle++) {
            if ((cast_alpha & bit_mask) != 0) {
                gemm<false, false>(T{0.5}, out, pow_a, T{0.0}, &temp);
                gemm<false, false>(T{0.5}, pow_a, out, T{1.0}, &temp); // For stability
                out = temp;
            }

            cast_alpha >>= 1;

            gemm<false, false>(T{1.0}, pow_a, pow_a, T{0.0}, &temp);
            pow_a = temp;
        }

        return out;
    }
}

} // namespace einsums::linear_algebra::detail
