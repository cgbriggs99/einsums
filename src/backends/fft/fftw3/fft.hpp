#pragma once

#include "einsums/Tensor.hpp"

namespace einsums::backend::fftw3 {

void scfft(const Tensor<float, 1> &a, Tensor<std::complex<float>, 1> *result);
void ccfft(const Tensor<std::complex<float>, 1> &a, Tensor<std::complex<float>, 1> *result);

void dzfft(const Tensor<double, 1> &a, Tensor<std::complex<double>, 1> *result);
void zzfft(const Tensor<std::complex<double>, 1> &a, Tensor<std::complex<double>, 1> *result);

void csifft(const Tensor<std::complex<float>, 1> &a, Tensor<float, 1> *result);
void zdifft(const Tensor<std::complex<double>, 1> &a, Tensor<double, 1> *result);

void ccifft(const Tensor<std::complex<float>, 1> &a, Tensor<std::complex<float>, 1> *result);
void zzifft(const Tensor<std::complex<double>, 1> &a, Tensor<std::complex<double>, 1> *result);

} // namespace einsums::backend::fftw3