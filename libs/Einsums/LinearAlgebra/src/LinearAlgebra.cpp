//----------------------------------------------------------------------------------------------
// Copyright (c) The Einsums Developers. All rights reserved.
// Licensed under the MIT License. See LICENSE.txt in the project root for license information.
//----------------------------------------------------------------------------------------------

#include <Einsums/Config.hpp>

#include <Einsums/LinearAlgebra.hpp>

namespace einsums::linear_algebra::detail {
template <CoreBasicTensorConcept AType>
auto sqrt(AType const &a, RemoveComplexT<typename AType::ValueType> cutoff) -> Tensor<typename AType::ValueType, 2> {

    assert(a.dim(0) == a.dim(1));

    using T = typename AType::ValueType;

    if constexpr (IsComplexV<typename AType::ValueType>) {
        // Special algorithm for a complex matrix. See Björk and Hammerling, 1983

        // First, compute the Schur canonical form of A.
        Tensor<T, 2> X{a.dims()}, Q{a.dims()}, U{a.dims()};
        Tensor<T, 1> eigvals{"eigenvalues", a.dim(0)};

        size_t const n = a.dim(0);

        // Transpose the tensor.
        for (size_t i = 0; i < n; i++) {
            for (size_t j = 0; j < n; j++) {
                X(i, j) = a(j, i);
            }
        }

        blas::int_t sdim;

        auto res = blas::gees('V', n, X.data(), X.stride(0), &sdim, eigvals.data(), Q.data(), Q.stride(0));

        if (res < 0) {
            EINSUMS_THROW_EXCEPTION(std::runtime_error, "The {} argument to [c,z]gees had an invalid value!", print::ordinal(-res));
        } else if (res > 0 && res <= n) {
            EINSUMS_THROW_EXCEPTION(std::runtime_error, "The QR algorithm failed to converge!");
        } else if (res == n + 1) {
            EINSUMS_THROW_EXCEPTION(std::runtime_error, "Could not reorder eigenvalues! This should not be thrown!");
        } else if (res == n + 2) {
            EINSUMS_LOG_WARN("Roundoff errors have changed eigenvalues after reordering in gees. Results may be unstable!");
        }

        // Sort the matrix so that all of the zero eigenvalues are at the back.
        size_t num_zeros = 0;

        for (size_t i = 0; i < n; i++) {
            if (std::abs(eigvals(i)) < cutoff) {
                size_t const end = n - 1 - num_zeros;
                // Swap the eigenvalue to the end of the list.
                std::swap(eigvals(i), eigvals(end));

                // Now, perform the swap of the Schur vectors.
                T const a = X(i, i), b = X(i, end), c = X(end, end);
                T const R   = std::sqrt(std::abs(a - c) * std::abs(a - c) + std::abs(b) * std::abs(b));
                T const cos = b / R, sin = (a - c) / R;

                T const p = Q(i, i), q = Q(end, i), r = Q(i, end), s = Q(end, end);

                Q(i, i)     = p * cos - q * sin;
                Q(end, i)   = p * sin + q * cos;
                Q(i, end)   = r * cos - s * sin;
                Q(end, end) = r * sin + s * cos;

                // X(end, i) and X(i, end) are not affected by design.
                std::swap(X(i, i), X(end, end));
                num_zeros++;
            }
        }

        // Then, check to see if the matrix does in fact have a square root.
        if (num_zeros > 1) {
            for (size_t i = n - num_zeros; i < n; i++) {
                for (size_t j = i + 1; j < n; j++) {
                    if (std::abs(X(j, i)) > cutoff) {
                        EINSUMS_THROW_EXCEPTION(std::domain_error, "Matrix does not have a square root due to having too many zero "
                                                                   "eigenvalues that are off-balanced by non-zero off-diagonal elements!");
                    }
                }
            }
        }

        // Now, iterate until convergence.
        bool converged = false;

        U.zero();

        // Initial guess is a diagonal matrix whose entries are the square roots of the eigenvalues.
        // The diagonal entries never change.
        for (size_t i = 0; i < n; i++) {
            U(i, i) = std::sqrt(X(i, i));
        }

        size_t const zero_tail = n - num_zeros;

        while (!converged) {
            converged = true;
            // Convert the equations to column-major form.
            for (size_t j = 0; j < n; j++) {
                for (size_t i = 0; i < j; i++) {
                    if (i >= zero_tail) {
                        // Special rule if there are close eigenvalues.
                        U(j, i) = T{0.0};
                    } else {
                        // Compute the new value.
                        T new_value{X(j, i)};
                        for (size_t k = i + 1; k < j; k++) {
                            new_value -= U(k, i) * U(j, k);
                        }
                        new_value /= U(i, i) + U(j, j);

                        // Check convergence.
                        if (std::abs(new_value - U(j, i)) > cutoff) {
                            converged = false;
                        }
                    }
                }
            }
        }

        // Back-transform to the solution.
        blas::gemm('N', 'N', n, n, n, T{1.0}, Q.data(), Q.stride(0), U.data(), U.stride(0), T{0.0}, X.data(), X.stride(0));
        blas::gemm('N', 'C', n, n, n, T{1.0}, X.data(), X.stride(0), Q.data(), Q.stride(0), T{0.0}, U.data(), U.stride(0));

        // Finally, transpose.
        for (size_t i = 0; i < n; i++) {
            for (size_t j = 0; j < n; j++) {
                X(i, j) = U(j, i);
            }
        }

        return X;
    } else {
        // Otherwise, for real values, use Sylvester decomposition.

        // Start with two guesses.
        Tensor<T, 2> P{a.dims()}, R{a.dims()}, Pinv{a.dims()}, Rinv{a.dims()}, diff{a.dims()};

        size_t const n = a.dim(0);

        T b{0.0}, B{0.0}, alpha, beta, alpha2, epsilon;

        P = a;

        R.zero();

        for (size_t i = 0; i < n; i++) {
            R(i, i) = T{1.0};
        }

        bool do_good_algorithm = true;

        // Next, compute the spectral radius of A and A^-1
        {
            Tensor<std::complex<T>, 1> eigvals{"eigenvalues", n};
            Pinv = a; // temporarily use Pinv since it's not used otherwise and it's already allocated.

            try {
                geev<false>(&Pinv, &eigvals, static_cast<decltype(&Pinv)>(nullptr), static_cast<decltype(&Pinv)>(nullptr));
            } catch (std::runtime_error &e) {
                do_good_algorithm = false;
            }

            if (do_good_algorithm) {

                T max_val{0.0}, max_inv{0.0};

                for (size_t i = 0; i < n; i++) {
                    T abs_val = std::abs(eigvals(i));

                    if (abs_val < cutoff) {
                        // We can't find b and B in this case directly, so we use a different norm.
                        // The best is the 2 norm, which is handled by the outer loop normally.
                        try {
                            Pinv = a;
                            invert(&Pinv);

                            B = linear_algebra::norm(linear_algebra::Norm::One, a);

                            T check = linear_algebra::norm(linear_algebra::Norm::One, Pinv);

                            // If we really can't handle the matrix, use a fallback.
                            if (check < cutoff) {
                                do_good_algorithm = false;
                                break;
                            }

                            b = T{1.0} / check;
                            break;
                        } catch (std::runtime_error &e) {
                            // If we can't invert the matrix, use a fallback.
                            do_good_algorithm = false;
                            break;
                        }
                    }

                    if (abs_val > max_val) {
                        max_val = abs_val;
                    }

                    if (T{1.0} / abs_val > max_inv) {
                        max_inv = T{1.0} / abs_val;
                    }
                }

                if (b == T{0.0} && B == T{0.0} && do_good_algorithm) {
                    b = T{1.0} / max_inv;
                    B = max_val;
                }
            }
        }

        EINSUMS_LOG_DEBUG("Found b = {} and B = {}", b, B);

        if (do_good_algorithm) {
            // From Hoskins and Walton 1978.

            T conv_check{0.0};

            size_t iter = 0;

            do {
                iter++;

                // Set acceleration parameters.
                alpha2  = T{2.0} / (b + B + T{6.0} * std::sqrt(b * B));
                alpha   = std::sqrt(alpha2);
                beta    = std::sqrt(b * B * alpha2);
                epsilon = T{1.0} - T{4.0} * alpha * beta;

                EINSUMS_LOG_DEBUG("iter: {}, alpha = {}, beta = {}, epsilon = {}, convergence = {}", iter, alpha, beta, epsilon,
                                  conv_check);

                diff = P;

                // Update matrices.
                Pinv = P;
                invert(&Pinv);
                Rinv = R;
                invert(&Rinv);

                axpby(beta, Rinv, alpha, &P);
                axpby(beta, Pinv, alpha, &R);

                // Update parameters.
                b = T{1.0} - epsilon;
                B = T{1.0} + epsilon;

                diff -= P;

                conv_check = linear_algebra::norm(linear_algebra::Norm::One, diff);

            } while (conv_check > cutoff && iter < 100);

            return P;
        } else {
            Tensor<std::complex<T>, 2> CLeft{a.dims()}, CRight{a.dims()}, out{a.dims()};
            Tensor<std::complex<T>, 1> eigvals{"eigenvalues", n};
            Pinv = a; // temporarily use Pinv since it's not used otherwise and it's already allocated.

            geev<true>(&Pinv, &eigvals, &P, &R);

            for (size_t i = 0; i < a.dim(1); i++) {
                if (std::imag(eigvals(i)) != T{0.0}) {
                    for (size_t j = 0; j < a.dim(0); j++) {
                        CLeft(j, i)      = std::complex<T>{P(j, i), P(j, i + 1)};
                        CRight(j, i)     = std::complex<T>{R(j, i), -R(j, i + 1)};
                        CLeft(j, i + 1)  = std::complex<T>{P(j, i), -P(j, i + 1)};
                        CRight(j, i + 1) = std::complex<T>{R(j, i), R(j, i + 1)};
                    }
                } else {
                    for (size_t j = 0; j < a.dim(0); j++) {
                        CLeft(j, i)  = std::complex<T>{P(j, i)};
                        CRight(j, i) = std::complex<T>{R(j, i)};
                    }
                }
            }

            for (size_t i = 0; i < a.dim(1); i++) {
                std::complex<T> scale = true_dot(CLeft(All, i), CRight(All, i));

                std::complex<T> lscale, rscale;

                lscale = std::sqrt(scale);
                rscale = lscale;

                for (size_t j = 0; j < a.dim(0); j++) {
                    CLeft(j, i) /= lscale;
                    CRight(j, i) /= rscale;
                }
            }

            for (size_t i = 0; i < a.dim(0); i++) {
                scale_row(i, std::sqrt(eigvals(i)), &CLeft);
            }

            gemm<true, false>(T{1.0}, CLeft, CRight, T{0.0}, &out);

            for (size_t i = 0; i < n; i++) {
                for (size_t j = 0; j < n; j++) {
                    if (std::abs(std::imag(out(i, j))) > cutoff) {
                        EINSUMS_THROW_EXCEPTION(std::domain_error, "Square root of matrix is not real!");
                    }

                    P(i, j) = std::real(out(i, j));
                }
            }

            return P;
        }
    }
}

template auto sqrt(Tensor<float, 2> const &a, float cutoff) -> Tensor<float, 2>;

template auto sqrt(Tensor<double, 2> const &a, double cutoff) -> Tensor<double, 2>;

template auto sqrt(Tensor<std::complex<float>, 2> const &a, float cutoff) -> Tensor<std::complex<float>, 2>;

template auto sqrt(Tensor<std::complex<double>, 2> const &a, double cutoff) -> Tensor<std::complex<double>, 2>;

} // namespace einsums::linear_algebra::detail