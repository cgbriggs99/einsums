#----------------------------------------------------------------------------------------------
# Copyright (c) The Einsums Developers. All rights reserved.
# Licensed under the MIT License. See LICENSE.txt in the project root for license information.
#----------------------------------------------------------------------------------------------

set(AssertionTests Assert)

set(Assert_PARAMETERS)

foreach(test ${AssertionTests})
  set(sources ${test}.cpp)

  source_group("Source Files" FILES ${sources})

  einsums_add_executable(
    ${test}_test INTERNAL_FLAGS
    SOURCES ${sources} ${${test}_FLAGS}
    FOLDER ${folder_name}
    NOINSTALL
  )

  einsums_add_unit_test("Modules.Assertion" ${test} ${${test}_PARAMETERS})

endforeach()
