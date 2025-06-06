#----------------------------------------------------------------------------------------------
# Copyright (c) The Einsums Developers. All rights reserved.
# Licensed under the MIT License. See LICENSE.txt in the project root for license information.
#----------------------------------------------------------------------------------------------

function(einsums_write_module_header)
  set(options)
  set(one_value_args TEMPLATE NAMESPACE FILENAME)
  set(multi_value_args)
  cmake_parse_arguments(OPTION "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/include/${basename}/${modulename}.hpp")
    return()
  elseif(OPTION_TEMPLATE)
    configure_file("${OPTION_TEMPLATE}" "${OPTION_FILENAME}" @ONLY)
  else()
    configure_file(
      "${PROJECT_SOURCE_DIR}/cmake/templates/GlobalModuleHeader.hpp.in" "${OPTION_FILENAME}" @ONLY
    )
  endif()
endfunction()
