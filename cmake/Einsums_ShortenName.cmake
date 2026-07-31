#----------------------------------------------------------------------------------------------
# Copyright (c) The Einsums Developers. All rights reserved.
# Licensed under the MIT License. See LICENSE.txt in the project root for license information.
#----------------------------------------------------------------------------------------------

function(einsums_shorten_name name varname)
	if(EINSUMS_SHORTEN_NAMES)
	  execute_process(COMMAND python ${CMAKE_SOURCE_DIR}/cmake/scripts/hash_name.py --no-separate "${name}" OUTPUT_VARIABLE short_name OUTPUT_STRIP_TRAILING_WHITESPACE)
	
	  set(${varname} "${short_name}" PARENT_SCOPE)
	else()
	  set(${varname} "${name}" PARENT_SCOPE)
	endif()
endfunction()

function(einsums_shorten_file name varname)
	if(EINSUMS_SHORTEN_NAMES)
	  execute_process(COMMAND python3 ${CMAKE_SOURCE_DIR}/cmake/scripts/hash_name.py "${name}" OUTPUT_VARIABLE short_name OUTPUT_STRIP_TRAILING_WHITESPACE)
	
	  set(${varname} "${short_name}" PARENT_SCOPE)
	else()
	  set(${varname} "${name}" PARENT_SCOPE)
	endif()
endfunction()

function(einsums_add_subdirectory name)

	cmake_path(ABSOLUTE_PATH name OUTPUT_VARIABLE full_path)
	cmake_path(RELATIVE_PATH full_path BASE_DIRECTORY ${CMAKE_SOURCE_DIR} OUTPUT_VARIABLE rel_path)
	
	einsums_shorten_name("${rel_path}" __shortname)
	
	einsums_debug("Shortened ${rel_path} to ${__shortname}")
	
	add_subdirectory("${name}" "${CMAKE_BINARY_DIR}/${__shortname}" ${ARGN})
	
endfunction()

