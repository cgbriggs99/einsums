//----------------------------------------------------------------------------------------------
// Copyright (c) The Einsums Developers. All rights reserved.
// Licensed under the MIT License. See LICENSE.txt in the project root for license information.
//----------------------------------------------------------------------------------------------

//  Copyright (c) 2020 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(__has_cpp_attribute)
#    error "__has_cpp_attribute not supported, assume [[no_unique_address]] is not supported"
#else
#    if !__has_cpp_attribute(no_unique_address)
#        error "__has_cpp_attribute(no_unique_address) not supported"
#    endif
#endif

int main() {
    return 0;
}
