#----------------------------------------------------------------------------------------------
# Copyright (c) The Einsums Developers. All rights reserved.
# Licensed under the MIT License. See LICENSE.txt in the project root for license information.
#----------------------------------------------------------------------------------------------

# The rand_s function needs to be cryptographically secure. Everyone has openssl since it's a dependency
# of HDF5.

message("-- Finding OpenSSL::Crypto")
find_package(OpenSSL COMPONENTS Crypto REQUIRED)

if(OpenSSL_FOUND)
  message("-- Found OpenSSL version ${OpenSSL_VERSION}.")
  message("-- OpenSSL libraries: ${OPENSSL_CRYPTO_LIBRARIES}")
else()
  message("-- Could not find OpenSSL!")
endif()