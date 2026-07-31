#----------------------------------------------------------------------------------------------
# Copyright (c) The Einsums Developers. All rights reserved.
# Licensed under the MIT License. See LICENSE.txt in the project root for license information.
#----------------------------------------------------------------------------------------------

function(einsums_add_pseudo_target)
  einsums_debug("einsums_add_pseudo_target" "adding pseudo target: ${ARGV}")
  if(EINSUMS_WITH_PSEUDO_DEPENDENCIES)
    set(args)
    foreach(arg ${ARGV})
      set(args ${args} ${arg})
    endforeach()
    einsums_debug("einsums_add_pseudo_target" "adding shortened pseudo target: ${shortened_args}")
    foreach(target ${args})
      if(NOT TARGET ${target})
        add_custom_target(${target})
      endif()
    endforeach()
  endif()
endfunction()
