//----------------------------------------------------------------------------------------------
// Copyright (c) The Einsums Developers. All rights reserved.
// Licensed under the MIT License. See LICENSE.txt in the project root for license information.
//----------------------------------------------------------------------------------------------

#include <Einsums/Config.hpp>

#include <Einsums/BLASVendor/Vendor.hpp>
#include <Einsums/Print.hpp>
#include <Einsums/Profile/LabeledSection.hpp>

#include "Common.hpp"

namespace einsums::blas::vendor {

extern "C" {
extern void FC_GLOBAL(sgeqrf, SGEQRF)(int_t *, int_t *, float *, int_t *, float *, float *, int_t *, int_t *);
extern void FC_GLOBAL(dgeqrf, DGEQRF)(int_t *, int_t *, double *, int_t *, double *, double *, int_t *, int_t *);
extern void FC_GLOBAL(cgeqrf, CGEQRF)(int_t *, int_t *, std::complex<float> *, int_t *, std::complex<float> *, std::complex<float> *,
                                      int_t *, int_t *);
extern void FC_GLOBAL(zgeqrf, ZGEQRF)(int_t *, int_t *, std::complex<double> *, int_t *, std::complex<double> *, std::complex<double> *,
                                      int_t *, int_t *);
}

#define GEQRF(Type, lc, uc)                                                                                                                \
    auto lc##geqrf(int_t m, int_t n, Type *a, int_t lda, Type *tau)->int_t {                                                               \
        LabeledSection0();                                                                                                                 \
                                                                                                                                           \
        int_t info{0};                                                                                                                     \
        int_t lwork{-1};                                                                                                                   \
        Type  work_query;                                                                                                                  \
                                                                                                                                           \
        int_t lda_t = std::max(int_t{1}, m);                                                                                               \
                                                                                                                                           \
        /* Check leading dimensions */                                                                                                     \
        if (lda < n) {                                                                                                                     \
            println_warn("geqrf warning: lda < n, lda = {}, n = {}", lda, n);                                                              \
            return -4;                                                                                                                     \
        }                                                                                                                                  \
                                                                                                                                           \
        /* Query optimal working array size */                                                                                             \
        FC_GLOBAL(lc##geqrf, UC##GEQRF)(&m, &n, a, &lda_t, tau, &work_query, &lwork, &info);                                               \
                                                                                                                                           \
        lwork = (int_t)work_query;                                                                                                         \
        std::vector<Type> work(lwork);                                                                                                     \
                                                                                                                                           \
        /* Allocate memory for temporary array(s) */                                                                                       \
        std::vector<Type> a_t(lda_t *std::max(int_t{1}, n));                                                                               \
                                                                                                                                           \
        /* Transpose input matrices */                                                                                                     \
        transpose<OrderMajor::Row>(m, n, a, lda, a_t, lda_t);                                                                              \
                                                                                                                                           \
        /* Call LAPACK function and adjust info */                                                                                         \
        FC_GLOBAL(lc##geqrf, UC##GEQRF)(&m, &n, a_t.data(), &lda_t, tau, work.data(), &lwork, &info);                                      \
                                                                                                                                           \
        if (info < 0) {                                                                                                                    \
            return info;                                                                                                                   \
        }                                                                                                                                  \
                                                                                                                                           \
        /* Transpose output matrices */                                                                                                    \
        transpose<OrderMajor::Column>(m, n, a_t, lda_t, a, lda);                                                                           \
                                                                                                                                           \
        return 0;                                                                                                                          \
    } /**/

#define GEQRF_complex(Type, lc, uc)                                                                                                        \
    auto lc##geqrf(int_t m, int_t n, Type *a, int_t lda, Type *tau)->int_t {                                                               \
        LabeledSection0();                                                                                                                 \
                                                                                                                                           \
        int_t info{0};                                                                                                                     \
        int_t lwork{-1};                                                                                                                   \
        Type  work_query;                                                                                                                  \
                                                                                                                                           \
        int_t lda_t = std::max(int_t{1}, m);                                                                                               \
                                                                                                                                           \
        /* Check leading dimensions */                                                                                                     \
        if (lda < n) {                                                                                                                     \
            println_warn("geqrf warning: lda < n, lda = {}, n = {}", lda, n);                                                              \
            return -4;                                                                                                                     \
        }                                                                                                                                  \
                                                                                                                                           \
        /* Query optimal working array size */                                                                                             \
        FC_GLOBAL(lc##geqrf, UC##GEQRF)(&m, &n, a, &lda_t, tau, &work_query, &lwork, &info);                                               \
                                                                                                                                           \
        lwork = (int_t)(work_query.real());                                                                                                \
        std::vector<Type> work(lwork);                                                                                                     \
                                                                                                                                           \
        /* Allocate memory for temporary array(s) */                                                                                       \
        std::vector<Type> a_t(lda_t *std::max(int_t{1}, n));                                                                               \
                                                                                                                                           \
        /* Transpose input matrices */                                                                                                     \
        transpose<OrderMajor::Row>(m, n, a, lda, a_t, lda_t);                                                                              \
                                                                                                                                           \
        /* Call LAPACK function and adjust info */                                                                                         \
        FC_GLOBAL(lc##geqrf, UC##GEQRF)(&m, &n, a_t.data(), &lda_t, tau, work.data(), &lwork, &info);                                      \
                                                                                                                                           \
        if (info < 0) {                                                                                                                    \
            return info;                                                                                                                   \
        }                                                                                                                                  \
                                                                                                                                           \
        /* Transpose output matrices */                                                                                                    \
        transpose<OrderMajor::Column>(m, n, a_t, lda_t, a, lda);                                                                           \
                                                                                                                                           \
        return 0;                                                                                                                          \
    } /**/

GEQRF(double, d, D);
GEQRF(float, s, S);
GEQRF_complex(std::complex<double>, z, Z);
GEQRF_complex(std::complex<float>, c, C);

} // namespace einsums::blas::vendor