//----------------------------------------------------------------------------------------------
// Copyright (c) The Einsums Developers. All rights reserved.
// Licensed under the MIT License. See LICENSE.txt in the project root for license information.
//----------------------------------------------------------------------------------------------

#include <Einsums/BufferAllocator/BufferAllocator.hpp>

#if defined(EINSUMS_HAVE_MALLOC_MIMALLOC)
#    include <mimalloc.h>
#endif

namespace einsums::detail {

#define EINSUMS_ALIGNMENT 64

void *allocate(size_t n) {
    // After several rounds of going back and forth with the compiler's optimizer and
    // using Ghidra to decompile hand-written assembly, this is the expression that I came
    // up with that is most optimal while still being readable.
    // On x64, this ultimately becomes two instructions totaling 8 bytes.
    // The goal here is to round up to the next multiple of EINSUMS_ALIGNMENT.

    // First, add one less than the alignment.
    size_t const modified_n = n + EINSUMS_ALIGNMENT - 1;

    // Then round down. If n was already a multiple of the alignment, these steps do nothing. But if n is even one
    // byte off, then it will be pushed above the next alignment frame, causing it to be rounded up to the next alignment.
    size_t const rounded_n = modified_n & ~static_cast<size_t>(EINSUMS_ALIGNMENT - 1);

    void *ptr = nullptr;

#if defined(EINSUMS_HAVE_MALLOC_MIMALLOC)
    ptr = mi_malloc_aligned(rounded_n, EINSUMS_ALIGNMENT);
#elif defined(_ISOC11_SOURCE) || (__STDC_VERSION__ >= 201112L)
    ptr = std::aligned_alloc(EINSUMS_ALIGNMENT, rounded_n);
#else
    // returns zero on success, or an error value. On Linux (and other systems), p is not modified on failure.
    if (posix_memalign(&ptr, EINSUMS_ALIGNMENT, rounded_n) != 0) {
        ptr = nullptr;
    }
#endif
    return ptr;
}

void deallocate(void *p) {
#if defined(EINSUMS_HAVE_MALLOC_MIMALLOC)
    mi_free(p);
#else
    std::free(static_cast<void *>(p));
#endif
}

} // namespace einsums::detail
