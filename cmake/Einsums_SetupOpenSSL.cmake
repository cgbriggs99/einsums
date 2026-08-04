#----------------------------------------------------------------------------------------------
# Copyright (c) The Einsums Developers. All rights reserved.
# Licensed under the MIT License. See LICENSE.txt in the project root for license information.
#----------------------------------------------------------------------------------------------

# The rand_s function needs to be cryptographically secure. Everyone has openssl since it's a dependency
# of HDF5.

find_package(OpenSSL COMPONENTS Crypto)