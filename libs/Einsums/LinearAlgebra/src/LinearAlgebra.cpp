//----------------------------------------------------------------------------------------------
// Copyright (c) The Einsums Developers. All rights reserved.
// Licensed under the MIT License. See LICENSE.txt in the project root for license information.
//----------------------------------------------------------------------------------------------

#include <Einsums/Config.hpp>

#include <Einsums/LinearAlgebra.hpp>

#include <queue>

#include "Einsums/BufferAllocator/BufferAllocator.hpp"

namespace einsums::linear_algebra::detail {
BufferVector<uint64_t> choose_all_n(uint64_t n) {
    BufferVector<uint64_t> out(n + 1);

    for (size_t i = 0; i < n; i++) {
        out[i] = 0;
    }

    out[0] = 1;

    for (size_t i = 1; i <= n; i++) {
        for (size_t k = i; k > 0; k--) {
            out[k] += out[k - 1];
        }
    }
    return out;
}
} // namespace einsums::linear_algebra::detail