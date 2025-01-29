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

EINSUMS_DISABLE_WARNING_PUSH
EINSUMS_DISABLE_WARNING_RETURN_TYPE_C_LINKAGE
extern "C" {
extern float                FC_GLOBAL(sdot, SDOT)(int_t *, float const *, int_t *, float const *, int_t *);
extern double               FC_GLOBAL(ddot, DDOT)(int_t *, double const *, int_t *, double const *, int_t *);
extern std::complex<float>  FC_GLOBAL(cdotc, CDOTC)(int_t *, std::complex<float> const *, int_t *, std::complex<float> const *, int_t *);
extern std::complex<double> FC_GLOBAL(zdotc, ZDOTC)(int_t *, std::complex<double> const *, int_t *, std::complex<double> const *, int_t *);
}
EINSUMS_DISABLE_WARNING_POP

auto sdot(int_t n, float const *x, int_t incx, float const *y, int_t incy) -> float {
    LabeledSection0();

    return FC_GLOBAL(sdot, SDOT)(&n, x, &incx, y, &incy);
}

auto ddot(int_t n, double const *x, int_t incx, double const *y, int_t incy) -> double {
    LabeledSection0();

    return FC_GLOBAL(ddot, DDOT)(&n, x, &incx, y, &incy);
}

// We implement the cdotu as the default for cdot.
auto cdot(int_t n, std::complex<float> const *x, int_t incx, std::complex<float> const *y, int_t incy) -> std::complex<float> {
    LabeledSection0();

    // Since MKL does not conform to the netlib standard, we need to use the following code.
    std::complex<float> result{0.0F, 0.0F};
    for (int_t i = 0; i < n; ++i) {
        result += x[static_cast<ptrdiff_t>(i * incx)] * y[static_cast<ptrdiff_t>(i * incy)];
    }
    return result;
}

// We implement the zdotu as the default for cdot.
auto zdot(int_t n, std::complex<double> const *x, int_t incx, std::complex<double> const *y, int_t incy) -> std::complex<double> {
    LabeledSection0();

    // Since MKL does not conform to the netlib standard, we need to use the following code.
    std::complex<double> result{0.0, 0.0};
    for (int_t i = 0; i < n; ++i) {
        result += x[static_cast<ptrdiff_t>(i * incx)] * y[static_cast<ptrdiff_t>(i * incy)];
    }
    return result;
}

auto cdotc(int_t n, std::complex<float> const *x, int_t incx, std::complex<float> const *y, int_t incy) -> std::complex<float> {
    LabeledSection0();

    return FC_GLOBAL(cdotc, CDOTC)(&n, x, &incx, y, &incy);
}

auto zdotc(int_t n, std::complex<double> const *x, int_t incx, std::complex<double> const *y, int_t incy) -> std::complex<double> {
    LabeledSection0();

    return FC_GLOBAL(zdotc, ZDOTC)(&n, x, &incx, y, &incy);
}

} // namespace einsums::blas::vendor