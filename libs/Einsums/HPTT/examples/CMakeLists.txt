#----------------------------------------------------------------------------------------------
# Copyright (c) The Einsums Developers. All rights reserved.
# Licensed under the MIT License. See LICENSE.txt in the project root for license information.
#----------------------------------------------------------------------------------------------

if(EINSUMS_WITH_EXAMPLES)
  einsums_add_pseudo_target(Examples.Modules.HPTT)
  einsums_add_pseudo_dependencies(Examples.Modules Examples.Modules.HPTT)
  if(EINSUMS_WITH_TESTS AND EINSUMS_WITH_TESTS_EXAMPLES)
    einsums_add_pseudo_target(Tests.Examples.Modules.HPTT)
    einsums_add_pseudo_dependencies(Tests.Examples.Modules Tests.Examples.Modules.HPTT)
  endif()
endif()
