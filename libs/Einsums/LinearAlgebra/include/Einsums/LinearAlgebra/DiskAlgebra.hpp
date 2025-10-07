//----------------------------------------------------------------------------------------------
// Copyright (c) The Einsums Developers. All rights reserved.
// Licensed under the MIT License. See LICENSE.txt in the project root for license information.
//----------------------------------------------------------------------------------------------

#pragma once

#include <Einsums/Concepts/TensorConcepts.hpp>
#include <Einsums/Errors/Error.hpp>
#include <Einsums/Errors/ThrowException.hpp>
#include <Einsums/LinearAlgebra/Base.hpp>
#include <Einsums/LinearAlgebra/Bases/high_precision.hpp>
#include <Einsums/Tensor/DiskTensor.hpp>

#include <thread>

namespace einsums::linear_algebra::detail {

template <DiskTensorConcept CType, typename U>
void scale(U alpha, CType *C) {
    constexpr size_t Rank = CType::Rank;

    BufferAllocator<typename CType::ValueType> C_alloc;

    size_t buffer_size = C_alloc.work_buffer_size();

    if (C->size() <= buffer_size) {
        scale(alpha, &C->get());
        C->put();
        return;
    }

    // Calculate the things needed to loop over the tensors.
    size_t    loop_step = 1, loop_skip = 0;
    ptrdiff_t rank_step = -1, rank_skip = -1;
    size_t    view1_size = 1, remaining_size = 0, step_size = 1;
    bool      found_max = false;

    for (int i = Rank - 1; i >= 0; i--) {
        if (buffer_size > C->dim(i) * view1_size && !found_max) {
            view1_size *= C->dim(i);
        } else if (buffer_size <= C->dim(i) * view1_size && view1_size < buffer_size && !found_max) {
            size_t max_dim = buffer_size / view1_size;
            rank_skip      = i;
            view1_size *= max_dim;
            step_size      = max_dim;
            remaining_size = C->dim(i) % max_dim;
            loop_skip      = C->dim(i) / max_dim;
            found_max      = true;
        } else {
            loop_step *= C->dim(i);
            rank_step = std::max(rank_step, (ptrdiff_t)i);
        }
    }

    if (rank_skip < rank_step) {
        rank_skip = rank_step;
    }

    // Set up the indices for the view.
    std::array<Range, Rank> view_indices;

    for (int i = rank_skip; i < Rank; i++) {
        view_indices[i] = Range{0, C->dim(i)};
    }

    for (size_t i = 0; i < loop_step; i++) {
        size_t temp = i;
        for (int k = rank_step; k >= 0; k--) {
            view_indices[k] = Range{temp % C->dim(k), temp % C->dim(k) + 1};
            temp /= C->dim(k);
        }
        for (size_t j = 0; j < loop_skip; j++) {
            // Generate the view.
            view_indices[rank_skip] = Range{j * step_size, (j + 1) * step_size};

            // Find the view.
            auto C_view = std::apply(*C, view_indices);

            scale(alpha, &C_view.get());
            C_view.put();
        }

        // Handle the remainder.
        if (remaining_size != 0) {
            view_indices[rank_skip] = Range{loop_skip * step_size, loop_skip * step_size + remaining_size};
            // Find the view.
            auto C_view = std::apply(*C, view_indices);

            scale(alpha, &C_view.get());
            C_view.put();
        }
    }
}

template <bool Conjugate, BufferableTensorConcept AType, BufferableTensorConcept BType>
    requires requires { requires SameRank<AType, BType>; }
auto dot_base(AType const &A, BType const &B) -> BiggestTypeT<typename AType::ValueType, typename BType::ValueType> {
    constexpr size_t Rank = AType::Rank;

    for (int i = 0; i < Rank; i++) {
        if (A.dim(i) != B.dim(i)) {
            EINSUMS_THROW_EXCEPTION(einsums::tensor_compat_error, "The tensors passed into the dot product must have the same dimensions!");
        }
    }

    BufferAllocator<typename AType::ValueType> A_alloc;
    BufferAllocator<typename BType::ValueType> B_alloc;

    size_t buffer_size = std::min(A_alloc.work_buffer_size(), B_alloc.work_buffer_size());

    if (A.size() <= buffer_size) {
        BiggestTypeT<typename AType::ValueType, typename BType::ValueType> out;

        if constexpr (Conjugate && IsComplexV<typename AType::ValueType>) {
            out = true_dot(A.get(), B.get());
        } else {
            out = dot(A.get(), B.get());
        }

        return out;
    }

    // Calculate the things needed to loop over the tensors.
    size_t    loop_step = 1, loop_skip = 0;
    ptrdiff_t rank_step = -1, rank_skip = -1;
    size_t    view1_size = 1, remaining_size = 0, step_size = 1;
    bool      found_max = false;

    for (int i = Rank - 1; i >= 0; i--) {
        if (buffer_size > A.dim(i) * view1_size && !found_max) {
            view1_size *= A.dim(i);
        } else if (buffer_size <= A.dim(i) * view1_size && view1_size < buffer_size && !found_max) {
            size_t max_dim = buffer_size / view1_size;
            rank_skip      = i;
            view1_size *= max_dim;
            step_size      = max_dim;
            remaining_size = A.dim(i) % max_dim;
            loop_skip      = A.dim(i) / max_dim;
            found_max      = true;
        } else {
            loop_step *= A.dim(i);
            rank_step = std::max(rank_step, (ptrdiff_t)i);
        }
    }

    if (rank_skip < rank_step) {
        rank_skip = rank_step;
    }

    // Loop over and add.
    einsums::BiggestTypeT<typename AType::ValueType, typename BType::ValueType> big_sum{0.0}, medium_sum{0.0}, small_sum{0.0};

    bool not_big_re = true, not_big_im = true;

    // Set up the indices for the view.
    std::array<Range, Rank> view_indices;

    for (int i = rank_skip; i < Rank; i++) {
        view_indices[i] = Range{0, A.dim(i)};
    }

    for (size_t i = 0; i < loop_step; i++) {
        size_t temp = i;
        for (int k = rank_step; k >= 0; k--) {
            view_indices[k] = Range{temp % A.dim(k), temp % A.dim(k) + 1};
            temp /= A.dim(k);
        }
        for (size_t j = 0; j < loop_skip; j++) {
            // Generate the view.
            view_indices[rank_skip] = Range{j * step_size, (j + 1) * step_size};

            // Find the view.
            auto A_view = std::apply(A, view_indices);
            auto B_view = std::apply(B, view_indices);

            if constexpr (Conjugate && IsComplexV<typename AType::ValueType>) {
                add_scale(true_dot(A_view.get(), B_view.get()), big_sum, medium_sum, small_sum, not_big_re, not_big_im);
            } else {
                add_scale(dot(A_view.get(), B_view.get()), big_sum, medium_sum, small_sum, not_big_re, not_big_im);
            }
        }

        // Handle the remainder.
        if (remaining_size != 0) {
            view_indices[rank_skip] = Range{loop_skip * step_size, loop_skip * step_size + remaining_size};
            auto A_view             = std::apply(A, view_indices);
            auto B_view             = std::apply(B, view_indices);

            if constexpr (Conjugate && IsComplexV<typename AType::ValueType>) {
                add_scale(true_dot(A_view.get(), B_view.get()), big_sum, medium_sum, small_sum, not_big_re, not_big_im);
            } else {
                add_scale(dot(A_view.get(), B_view.get()), big_sum, medium_sum, small_sum, not_big_re, not_big_im);
            }
        }
    }

    return combine_accum(big_sum, medium_sum, small_sum);
}

template <BufferableTensorConcept AType, BufferableTensorConcept BType>
    requires requires { requires SameRank<AType, BType>; }
auto dot(AType const &A, BType const &B) -> BiggestTypeT<typename AType::ValueType, typename BType::ValueType> {
    return dot_base<false>(A, B);
}

template <BufferableTensorConcept AType, BufferableTensorConcept BType>
    requires requires { requires SameRank<AType, BType>; }
auto true_dot(AType const &A, BType const &B) -> BiggestTypeT<typename AType::ValueType, typename BType::ValueType> {
    return dot_base<true>(A, B);
}

template <BufferableTensorConcept AType, BufferableTensorConcept BType, BufferableTensorConcept CType, typename U>
    requires requires { requires SameRank<AType, BType>; }
void direct_product(U alpha, AType const &A, BType const &B, U beta, CType *C) {
    constexpr size_t Rank = AType::Rank;

    for (int i = 0; i < Rank; i++) {
        if (A.dim(i) != B.dim(i) || A.dim(i) != C->dim(i)) {
            EINSUMS_THROW_EXCEPTION(einsums::tensor_compat_error,
                                    "The tensors passed into the direct product must have the same dimensions!");
        }
    }

    BufferAllocator<typename AType::ValueType> A_alloc;
    BufferAllocator<typename BType::ValueType> B_alloc;
    BufferAllocator<typename CType::ValueType> C_alloc;

    size_t buffer_size = std::min(A_alloc.work_buffer_size(), std::min(B_alloc.work_buffer_size(), C_alloc.work_buffer_size()));

    if (A.size() <= buffer_size) {
        direct_product(alpha, A.get(), B.get(), beta, &C->get());

        C->put();
        return;
    }

    // Calculate the things needed to loop over the tensors.
    size_t    loop_step = 1, loop_skip = 0;
    ptrdiff_t rank_step = -1, rank_skip = -1;
    size_t    view1_size = 1, remaining_size = 0, step_size = 1;
    bool      found_max = false;

    for (int i = Rank - 1; i >= 0; i--) {
        if (buffer_size > A.dim(i) * view1_size && !found_max) {
            view1_size *= A.dim(i);
        } else if (buffer_size <= A.dim(i) * view1_size && view1_size < buffer_size && !found_max) {
            size_t max_dim = buffer_size / view1_size;
            rank_skip      = i;
            view1_size *= max_dim;
            step_size      = max_dim;
            remaining_size = A.dim(i) % max_dim;
            loop_skip      = A.dim(i) / max_dim;
            found_max      = true;
        } else {
            loop_step *= A.dim(i);
            rank_step = std::max(rank_step, (ptrdiff_t)i);
        }
    }

    if (rank_skip < rank_step) {
        rank_skip = rank_step;
    }

    // Set up the indices for the view.
    std::array<Range, Rank> view_indices;

    for (int i = rank_skip; i < Rank; i++) {
        view_indices[i] = Range{0, A.dim(i)};
    }

    for (size_t i = 0; i < loop_step; i++) {
        size_t temp = i;
        for (int k = rank_step; k >= 0; k--) {
            view_indices[k] = Range{temp % A.dim(k), temp % A.dim(k) + 1};
            temp /= A.dim(k);
        }
        for (size_t j = 0; j < loop_skip; j++) {
            // Generate the view.
            view_indices[rank_skip] = Range{j * step_size, (j + 1) * step_size};

            // Find the view.
            auto A_view = std::apply(A, view_indices);
            auto B_view = std::apply(B, view_indices);
            auto C_view = std::apply(*C, view_indices);

            direct_product(alpha, A_view.get(), B_view.get(), beta, &C_view.get());
            C_view.put();
        }

        // Handle the remainder.
        if (remaining_size != 0) {
            view_indices[rank_skip] = Range{loop_skip * step_size, loop_skip * step_size + remaining_size};
            // Find the view.
            auto A_view = std::apply(A, view_indices);
            auto B_view = std::apply(B, view_indices);
            auto C_view = std::apply(*C, view_indices);

            direct_product(alpha, A_view.get(), B_view.get(), beta, &C_view.get());
            C_view.put();
        }
    }
}

template <TensorConcept AType, TensorConcept BType, TensorConcept CType, typename U>
    requires requires {
        requires BufferableTensorConcept<AType> || BufferableTensorConcept<BType> || BufferableTensorConcept<CType>;
        requires SameUnderlyingAndRank<AType, BType, CType>;
        requires MatrixConcept<AType>;
    }
void gemm(char transA, char transB, U alpha, AType const &A, BType const &B, U beta, CType *C) {
    bool tA = (std::tolower(transA) == 'n') ? false : true;
    bool tB = (std::tolower(transB) == 'n') ? false : true;

    // Calculate the parameters.
    size_t C_m = C->dim(0);
    size_t C_n = C->dim(1);

    size_t A_m = (tA) ? A.dim(1) : A.dim(0);
    size_t A_k = (tA) ? A.dim(0) : A.dim(1);

    size_t B_k = (tB) ? B.dim(1) : B.dim(0);
    size_t B_n = (tB) ? B.dim(0) : B.dim(1);

    if (A_m != C_m || A_k != B_k || C_n != B_n) {
        EINSUMS_THROW_EXCEPTION(einsums::dimension_error, "The tensors passed to gemm need to be the same size!");
    }

    // We are assuming that we have done some Strassen iterations before, so we need to find the least of these.
    size_t m = A_m, n = B_n, k = A_k;

    // If all parameters are less than 500, then perform the normal matrix multiplication.
    if (m < 500 && n < 500 && k < 500) {
        if constexpr (BufferableTensorConcept<AType> && BufferableTensorConcept<BType> && BufferableTensorConcept<CType>) {
            detail::gemm(transA, transB, alpha, A.get(), B.get(), beta, &C->get());
            C->put();
        } else if constexpr (BufferableTensorConcept<AType> && BufferableTensorConcept<BType>) {
            detail::gemm(transA, transB, alpha, A.get(), B.get(), beta, C);
        } else if constexpr (BufferableTensorConcept<AType> && BufferableTensorConcept<CType>) {
            detail::gemm(transA, transB, alpha, A.get(), B, beta, &C->get());
            C->put();
        } else if constexpr (BufferableTensorConcept<BType> && BufferableTensorConcept<CType>) {
            detail::gemm(transA, transB, alpha, A, B.get(), beta, &C->get());
            C->put();
        } else if constexpr (BufferableTensorConcept<AType>) {
            detail::gemm(transA, transB, alpha, A.get(), B, beta, C);
        } else if constexpr (BufferableTensorConcept<BType>) {
            detail::gemm(transA, transB, alpha, A, B.get(), beta, C);
        } else if constexpr (BufferableTensorConcept<CType>) {
            detail::gemm(transA, transB, alpha, A, B, beta, &C->get());
            C->put();
        } else {
            detail::gemm(transA, transB, alpha, A, B, beta, C);
        }
    } else {
        // Start by scaling C.
        if (beta != U{1.0}) {
            scale(beta, C);
        }

        // Next, we are going to loop over the indices in blocks of 500.
        int min_dim = 500;

        int m_loops = m / min_dim;
        int n_loops = n / min_dim;
        int k_loops = k / min_dim;

        for (int i = 0; i < m_loops; i++) {
            for (int j = 0; j < n_loops; j++) {
                auto C_view = (*C)(Range{i * min_dim, (i + 1) * min_dim}, Range{j * min_dim, (j + 1) * min_dim});
                for (int l = 0; l < k_loops; l++) {

                    if (tA && tB) {
                        gemm(transA, transB, alpha, A(Range{l * min_dim, (l + 1) * min_dim}, Range{i * min_dim, (i + 1) * min_dim}),
                             B(Range{j * min_dim, (j + 1) * min_dim}, Range{l * min_dim, (l + 1) * min_dim}), U{1.0}, &C_view);
                    } else if (tA) {
                        gemm(transA, transB, alpha, A(Range{l * min_dim, (l + 1) * min_dim}, Range{i * min_dim, (i + 1) * min_dim}),
                             B(Range{l * min_dim, (l + 1) * min_dim}, Range{j * min_dim, (j + 1) * min_dim}), U{1.0}, &C_view);
                    } else if (tB) {
                        gemm(transA, transB, alpha, A(Range{i * min_dim, (i + 1) * min_dim}, Range{l * min_dim, (l + 1) * min_dim}),
                             B(Range{j * min_dim, (j + 1) * min_dim}, Range{l * min_dim, (l + 1) * min_dim}), U{1.0}, &C_view);
                    } else {
                        gemm(transA, transB, alpha, A(Range{i * min_dim, (i + 1) * min_dim}, Range{l * min_dim, (l + 1) * min_dim}),
                             B(Range{l * min_dim, (l + 1) * min_dim}, Range{j * min_dim, (j + 1) * min_dim}), U{1.0}, &C_view);
                    }
                }

                if (k - k_loops * min_dim != 0) {
                    if (tA && tB) {
                        gemm(transA, transB, alpha, A(Range{k_loops * min_dim, k}, Range{i * min_dim, (i + 1) * min_dim}),
                             B(Range{j * min_dim, (j + 1) * min_dim}, Range{k_loops * min_dim, k}), U{1.0}, &C_view);
                    } else if (tA) {
                        gemm(transA, transB, alpha, A(Range{k_loops * min_dim, k}, Range{i * min_dim, (i + 1) * min_dim}),
                             B(Range{k_loops * min_dim, k}, Range{j * min_dim, (j + 1) * min_dim}), U{1.0}, &C_view);
                    } else if (tB) {
                        gemm(transA, transB, alpha, A(Range{i * min_dim, (i + 1) * min_dim}, Range{k_loops * min_dim, k}),
                             B(Range{j * min_dim, (j + 1) * min_dim}, Range{k_loops * min_dim, k}), U{1.0}, &C_view);
                    } else {
                        gemm(transA, transB, alpha, A(Range{i * min_dim, (i + 1) * min_dim}, Range{k_loops * min_dim, k}),
                             B(Range{k_loops * min_dim, k}, Range{j * min_dim, (j + 1) * min_dim}), U{1.0}, &C_view);
                    }
                }
            }
            if (n - n_loops * min_dim != 0) {
                auto C_view = (*C)(Range{i * min_dim, (i + 1) * min_dim}, Range{n_loops * min_dim, n});
                for (int l = 0; l < k_loops; l++) {
                    if (tA && tB) {
                        gemm(transA, transB, alpha, A(Range{l * min_dim, (l + 1) * min_dim}, Range{i * min_dim, (i + 1) * min_dim}),
                             B(Range{n_loops * min_dim, n}, Range{l * min_dim, (l + 1) * min_dim}), U{1.0}, &C_view);
                    } else if (tA) {
                        gemm(transA, transB, alpha, A(Range{l * min_dim, (l + 1) * min_dim}, Range{i * min_dim, (i + 1) * min_dim}),
                             B(Range{l * min_dim, (l + 1) * min_dim}, Range{n_loops * min_dim, n}), U{1.0}, &C_view);
                    } else if (tB) {
                        gemm(transA, transB, alpha, A(Range{i * min_dim, (i + 1) * min_dim}, Range{l * min_dim, (l + 1) * min_dim}),
                             B(Range{n_loops * min_dim, n}, Range{l * min_dim, (l + 1) * min_dim}), U{1.0}, &C_view);
                    } else {
                        gemm(transA, transB, alpha, A(Range{i * min_dim, (i + 1) * min_dim}, Range{l * min_dim, (l + 1) * min_dim}),
                             B(Range{l * min_dim, (l + 1) * min_dim}, Range{n_loops * min_dim, n}), U{1.0}, &C_view);
                    }
                }

                if (k - k_loops * min_dim != 0) {
                    if (tA && tB) {
                        gemm(transA, transB, alpha, A(Range{k_loops * min_dim, k}, Range{i * min_dim, (i + 1) * min_dim}),
                             B(Range{n_loops * min_dim, n}, Range{k_loops * min_dim, k}), U{1.0}, &C_view);
                    } else if (tA) {
                        gemm(transA, transB, alpha, A(Range{k_loops * min_dim, k}, Range{i * min_dim, (i + 1) * min_dim}),
                             B(Range{k_loops * min_dim, k}, Range{n_loops * min_dim, n}), U{1.0}, &C_view);
                    } else if (tB) {
                        gemm(transA, transB, alpha, A(Range{i * min_dim, (i + 1) * min_dim}, Range{k_loops * min_dim, k}),
                             B(Range{n_loops * min_dim, n}, Range{k_loops * min_dim, k}), U{1.0}, &C_view);
                    } else {
                        gemm(transA, transB, alpha, A(Range{i * min_dim, (i + 1) * min_dim}, Range{k_loops * min_dim, k}),
                             B(Range{k_loops * min_dim, k}, Range{n_loops * min_dim, n}), U{1.0}, &C_view);
                    }
                }
            }
        }
        if (m - m_loops * min_dim != 0) {
            for (int j = 0; j < n_loops; j++) {
                auto C_view = (*C)(Range{m - m_loops * min_dim, m}, Range{j * min_dim, (j + 1) * min_dim});
                for (int l = 0; l < k_loops; l++) {
                    if (tA && tB) {
                        gemm(transA, transB, alpha, A(Range{l * min_dim, (l + 1) * min_dim}, Range{m - m_loops * min_dim, m}),
                             B(Range{j * min_dim, (j + 1) * min_dim}, Range{l * min_dim, (l + 1) * min_dim}), U{1.0}, &C_view);
                    } else if (tA) {
                        gemm(transA, transB, alpha, A(Range{l * min_dim, (l + 1) * min_dim}, Range{m - m_loops * min_dim, m}),
                             B(Range{l * min_dim, (l + 1) * min_dim}, Range{j * min_dim, (j + 1) * min_dim}), U{1.0}, &C_view);
                    } else if (tB) {
                        gemm(transA, transB, alpha, A(Range{m - m_loops * min_dim, m}, Range{l * min_dim, (l + 1) * min_dim}),
                             B(Range{j * min_dim, (j + 1) * min_dim}, Range{l * min_dim, (l + 1) * min_dim}), U{1.0}, &C_view);
                    } else {
                        gemm(transA, transB, alpha, A(Range{m - m_loops * min_dim, m}, Range{l * min_dim, (l + 1) * min_dim}),
                             B(Range{l * min_dim, (l + 1) * min_dim}, Range{j * min_dim, (j + 1) * min_dim}), U{1.0}, &C_view);
                    }
                }

                if (k - k_loops * min_dim != 0) {
                    if (tA && tB) {
                        gemm(transA, transB, alpha, A(Range{k_loops * min_dim, k}, Range{m - m_loops * min_dim, m}),
                             B(Range{j * min_dim, (j + 1) * min_dim}, Range{k_loops * min_dim, k}), U{1.0}, &C_view);
                    } else if (tA) {
                        gemm(transA, transB, alpha, A(Range{k_loops * min_dim, k}, Range{m - m_loops * min_dim, m}),
                             B(Range{k_loops * min_dim, k}, Range{j * min_dim, (j + 1) * min_dim}), U{1.0}, &C_view);
                    } else if (tB) {
                        gemm(transA, transB, alpha, A(Range{m - m_loops * min_dim, m}, Range{k_loops * min_dim, k}),
                             B(Range{j * min_dim, (j + 1) * min_dim}, Range{k_loops * min_dim, k}), U{1.0}, &C_view);
                    } else {
                        gemm(transA, transB, alpha, A(Range{m - m_loops * min_dim, m}, Range{k_loops * min_dim, k}),
                             B(Range{k_loops * min_dim, k}, Range{j * min_dim, (j + 1) * min_dim}), U{1.0}, &C_view);
                    }
                }
            }
            if (n - n_loops * min_dim != 0) {
                auto C_view = (*C)(Range{m - m_loops * min_dim, m}, Range{n_loops * min_dim, n});
                for (int l = 0; l < k_loops; l++) {
                    if (tA && tB) {
                        gemm(transA, transB, alpha, A(Range{l * min_dim, (l + 1) * min_dim}, Range{m - m_loops * min_dim, m}),
                             B(Range{n_loops * min_dim, n}, Range{l * min_dim, (l + 1) * min_dim}), U{1.0}, &C_view);
                    } else if (tA) {
                        gemm(transA, transB, alpha, A(Range{l * min_dim, (l + 1) * min_dim}, Range{m - m_loops * min_dim, m}),
                             B(Range{l * min_dim, (l + 1) * min_dim}, Range{n_loops * min_dim, n}), U{1.0}, &C_view);
                    } else if (tB) {
                        gemm(transA, transB, alpha, A(Range{m - m_loops * min_dim, m}, Range{l * min_dim, (l + 1) * min_dim}),
                             B(Range{n_loops * min_dim, n}, Range{l * min_dim, (l + 1) * min_dim}), U{1.0}, &C_view);
                    } else {
                        gemm(transA, transB, alpha, A(Range{m - m_loops * min_dim, m}, Range{l * min_dim, (l + 1) * min_dim}),
                             B(Range{l * min_dim, (l + 1) * min_dim}, Range{n_loops * min_dim, n}), U{1.0}, &C_view);
                    }
                }

                if (k - k_loops * min_dim != 0) {
                    if (tA && tB) {
                        gemm(transA, transB, alpha, A(Range{k_loops * min_dim, k}, Range{m - m_loops * min_dim, m}),
                             B(Range{n_loops * min_dim, n}, Range{k_loops * min_dim, k}), U{1.0}, &C_view);
                    } else if (tA) {
                        gemm(transA, transB, alpha, A(Range{k_loops * min_dim, k}, Range{m - m_loops * min_dim, m}),
                             B(Range{k_loops * min_dim, k}, Range{n_loops * min_dim, n}), U{1.0}, &C_view);
                    } else if (tB) {
                        gemm(transA, transB, alpha, A(Range{m - m_loops * min_dim, m}, Range{k_loops * min_dim, k}),
                             B(Range{n_loops * min_dim, n}, Range{k_loops * min_dim, k}), U{1.0}, &C_view);
                    } else {
                        gemm(transA, transB, alpha, A(Range{m - m_loops * min_dim, m}, Range{k_loops * min_dim, k}),
                             B(Range{k_loops * min_dim, k}, Range{n_loops * min_dim, n}), U{1.0}, &C_view);
                    }
                }
            }
        }
    }
}

template <TensorConcept AType, TensorConcept BType, TensorConcept CType, typename U>
    requires requires {
        requires SameUnderlying<AType, BType, CType>;
        requires BufferableTensorConcept<AType> || BufferableTensorConcept<BType> || BufferableTensorConcept<CType>;
        requires MatrixConcept<AType>;
        requires VectorConcept<BType>;
        requires VectorConcept<CType>;
    }
void gemv(char transA, U alpha, AType const &A, BType const &B, U beta, CType *C) {
    bool tA = (std::tolower(transA) == 'n') ? false : true;

    // We are assuming that we have done some Strassen iterations before, so we need to find the least of these.
    size_t m = B.dim(0), n = C->dim(0);

    // If all parameters are less than 500, then perform the normal matrix multiplication.
    if (m < 500 && n < 500) {
        if constexpr (BufferableTensorConcept<AType> && BufferableTensorConcept<BType> && BufferableTensorConcept<CType>) {
            detail::gemv(transA, alpha, A.get(), B.get(), beta, &C->get());
            C->put();
        } else if constexpr (BufferableTensorConcept<AType> && BufferableTensorConcept<BType>) {
            detail::gemv(transA, alpha, A.get(), B.get(), beta, C);
        } else if constexpr (BufferableTensorConcept<AType> && BufferableTensorConcept<CType>) {
            detail::gemv(transA, alpha, A.get(), B, beta, &C->get());
            C->put();
        } else if constexpr (BufferableTensorConcept<BType> && BufferableTensorConcept<CType>) {
            detail::gemv(transA, alpha, A, B.get(), beta, &C->get());
            C->put();
        } else if constexpr (BufferableTensorConcept<AType>) {
            detail::gemv(transA, alpha, A.get(), B, beta, C);
        } else if constexpr (BufferableTensorConcept<BType>) {
            detail::gemv(transA, alpha, A, B.get(), beta, C);
        } else if constexpr (BufferableTensorConcept<CType>) {
            detail::gemv(transA, alpha, A, B, beta, &C->get());
            C->put();
        } else {
            detail::gemv(transA, alpha, A, B, beta, C);
        }
    } else {
        // Start by scaling C.
        if (beta != U{1.0}) {
            scale(beta, C);
        }

        // Next, we are going to loop over the indices in blocks of 500.
        int min_dim = 500;

        int m_loops = m / min_dim;
        int n_loops = n / min_dim;

        for (int i = 0; i < m_loops; i++) {
            auto B_view = B(Range{i * min_dim, (i + 1) * min_dim});
            for (int j = 0; j < n_loops; j++) {
                auto C_view = (*C)(Range{j * min_dim, (j + 1) * min_dim});
                if (tA) {
                    gemv(transA, alpha, A(Range{j * min_dim, (j + 1) * min_dim}, Range{i * min_dim, (i + 1) * min_dim}), B_view, U{1.0},
                         &C_view);
                } else {
                    gemv(transA, alpha, A(Range{i * min_dim, (i + 1) * min_dim}, Range{j * min_dim, (j + 1) * min_dim}), B_view, U{1.0},
                         &C_view);
                }
            }
            if (n - n_loops * min_dim != 0) {
                auto C_view = (*C)(Range{n_loops * min_dim, n});
                if (tA) {
                    gemv(transA, alpha, A(Range{n_loops * min_dim, n}, Range{i * min_dim, (i + 1) * min_dim}), B_view, U{1.0}, &C_view);

                } else {
                    gemv(transA, alpha, A(Range{i * min_dim, (i + 1) * min_dim}, Range{n_loops * min_dim, n}), B_view, U{1.0}, &C_view);
                }
            }
        }
        if (m - m_loops * min_dim != 0) {
            auto B_view = B(Range{m_loops * min_dim, m});
            for (int j = 0; j < n_loops; j++) {
                auto C_view = (*C)(Range{j * min_dim, (j + 1) * min_dim});
                if (tA) {
                    gemv(transA, alpha, A(Range{j * min_dim, (j + 1) * min_dim}, Range{m_loops * min_dim, m}), B_view, U{1.0}, &C_view);
                } else {
                    gemv(transA, alpha, A(Range{m_loops * min_dim, m}, Range{j * min_dim, (j + 1) * min_dim}), B_view, U{1.0}, &C_view);
                }
            }
            if (n - n_loops * min_dim != 0) {
                auto C_view = (*C)(Range{n_loops * min_dim, n});
                if (tA) {
                    gemv(transA, alpha, A(Range{n_loops * min_dim, n}, Range{m_loops * min_dim, m}), B_view, U{1.0}, &C_view);
                } else {
                    gemv(transA, alpha, A(Range{m_loops * min_dim, m}, Range{n_loops * min_dim, n}), B_view, U{1.0}, &C_view);
                }
            }
        }
    }
}

template <TensorConcept XType, TensorConcept YType, DiskTensorConcept AType, typename U>
    requires requires {
        requires SameUnderlying<AType, XType, YType>;
        requires MatrixConcept<AType>;
        requires VectorConcept<XType>;
        requires VectorConcept<YType>;
    }
void ger(U alpha, XType const &X, YType const &Y, AType *A) {
    // We are assuming that we have done some Strassen iterations before, so we need to find the least of these.
    size_t m = X.dim(0), n = Y.dim(0);

    // If all parameters are less than 500, then perform the normal matrix multiplication.
    if (m < 500 && n < 500) {
        if constexpr (BufferableTensorConcept<XType> && BufferableTensorConcept<YType>) {
            detail::ger(alpha, X.get(), Y.get(), &A->get());
            A->put();
        } else if constexpr (BufferableTensorConcept<XType>) {
            detail::ger(alpha, X.get(), Y, &A->get());
            A->put();
        } else if constexpr (BufferableTensorConcept<YType>) {
            detail::ger(alpha, X, Y.get(), &A->get());
            A->put();
        } else {
            detail::ger(alpha, X, Y, &A->get());
        }
    } else {
        // We are going to loop over the indices in blocks of 500.
        int min_dim = 500;

        int m_loops = m / min_dim;
        int n_loops = n / min_dim;

        // Compute the Y views.
        BufferVector<decltype(Y(All))> Y_views;
        Y_views.reserve(n_loops);

        for (int j = 0; j < n_loops; j++) {
            Y_views.push_back(Y(Range{j * min_dim, (j + 1) * min_dim}));
        }

        auto Y_last = Y(Range{n_loops * min_dim, n});

        // Now, loop.
        for (int i = 0; i < m_loops; i++) {
            auto X_view = X(Range{i * min_dim, (i + 1) * min_dim});
            for (int j = 0; j < n_loops; j++) {
                auto A_view = (*A)(Range{i * min_dim, (i + 1) * min_dim}, Range{j * min_dim, (j + 1) * min_dim});
                ger(alpha, X_view, Y_views[j], &A_view);
            }
            if (n - n_loops * min_dim != 0) {
                auto A_view = (*A)(Range{i * min_dim, (i + 1) * min_dim}, Range{n_loops * min_dim, n});
                ger(alpha, X_view, Y_last, &A_view);
            }
        }
        if (m - m_loops * min_dim != 0) {
            auto X_view = X(Range{m_loops * min_dim, m});
            for (int j = 0; j < n_loops; j++) {
                auto A_view = (*A)(Range{m_loops * min_dim, m}, Range{j * min_dim, (j + 1) * min_dim});
                ger(alpha, X_view, Y_views[j], &A_view);
            }
            if (n - n_loops * min_dim != 0) {
                auto A_view = (*A)(Range{m_loops * min_dim, m}, Range{n_loops * min_dim, n});
                ger(alpha, X_view, Y_last, &A_view);
            }
        }
    }
}

template <TensorConcept XType, TensorConcept YType, DiskTensorConcept AType, typename U>
    requires requires {
        requires SameUnderlying<AType, XType, YType>;
        requires MatrixConcept<AType>;
        requires VectorConcept<XType>;
        requires VectorConcept<YType>;
    }
void gerc(U alpha, XType const &X, YType const &Y, AType *A) {
    // We are assuming that we have done some Strassen iterations before, so we need to find the least of these.
    size_t m = X.dim(0), n = Y.dim(0);

    // If all parameters are less than 500, then perform the normal matrix multiplication.
    if (m < 500 && n < 500) {
        if constexpr (BufferableTensorConcept<XType> && BufferableTensorConcept<YType>) {
            detail::gerc(alpha, X.get(), Y.get(), &A->get());
            A->put();
        } else if constexpr (BufferableTensorConcept<XType>) {
            detail::gerc(alpha, X.get(), Y, &A->get());
            A->put();
        } else if constexpr (BufferableTensorConcept<YType>) {
            detail::gerc(alpha, X, Y.get(), &A->get());
            A->put();
        } else {
            detail::gerc(alpha, X, Y, &A->get());
        }
    } else {
        // We are going to loop over the indices in blocks of 500.
        int min_dim = 500;

        int m_loops = m / min_dim;
        int n_loops = n / min_dim;

        // Compute the Y views.
        BufferVector<decltype(Y(All))> Y_views;
        Y_views.reserve(n_loops);

        for (int j = 0; j < n_loops; j++) {
            Y_views.push_back(Y(Range{j * min_dim, (j + 1) * min_dim}));
        }

        auto Y_last = Y(Range{n_loops * min_dim, n});

        // Now, loop.
        for (int i = 0; i < m_loops; i++) {
            auto X_view = X(Range{i * min_dim, (i + 1) * min_dim});
            for (int j = 0; j < n_loops; j++) {
                auto A_view = (*A)(Range{i * min_dim, (i + 1) * min_dim}, Range{j * min_dim, (j + 1) * min_dim});
                gerc(alpha, X_view, Y_views[j], &A_view);
            }
            if (n - n_loops * min_dim != 0) {
                auto A_view = (*A)(Range{i * min_dim, (i + 1) * min_dim}, Range{n_loops * min_dim, n});
                gerc(alpha, X_view, Y_last, &A_view);
            }
        }
        if (m - m_loops * min_dim != 0) {
            auto X_view = X(Range{m_loops * min_dim, m});
            for (int j = 0; j < n_loops; j++) {
                auto A_view = (*A)(Range{m_loops * min_dim, m}, Range{j * min_dim, (j + 1) * min_dim});
                gerc(alpha, X_view, Y_views[j], &A_view);
            }
            if (n - n_loops * min_dim != 0) {
                auto A_view = (*A)(Range{m_loops * min_dim, m}, Range{n_loops * min_dim, n});
                gerc(alpha, X_view, Y_last, &A_view);
            }
        }
    }
}

} // namespace einsums::linear_algebra::detail