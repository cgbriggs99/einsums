#----------------------------------------------------------------------------------------------
# Copyright (c) The Einsums Developers. All rights reserved.
# Licensed under the MIT License. See LICENSE.txt in the project root for license information.
#----------------------------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.18 FATAL_ERROR)

find_package(Git)

if(NOT GIT_FOUND)
  message(FATAL_ERROR "Could not find git. git is needed to download and update the GitHub pages.")
endif()

macro(add_docs what)
  # add all newly generated files
  execute_process(
    COMMAND "${GIT_EXECUTABLE}" add ${what}
    WORKING_DIRECTORY "${EINSUMS_BINARY_DIR}/docs/gh-pages"
    RESULT_VARIABLE git_add_result
    ERROR_VARIABLE git_add_result_message COMMAND_ECHO STDERR
  )
  if(NOT "${git_add_result}" EQUAL "0")
    message(
      FATAL_ERROR "Adding files to the GitHub pages branch failed: ${git_add_result_message}."
    )
  endif()
endmacro()

macro(commit_docs message)
  # check if there are changes to commit
  execute_process(
    COMMAND "${GIT_EXECUTABLE}" diff-index --quiet HEAD
    WORKING_DIRECTORY "${EINSUMS_BINARY_DIR}/docs/gh-pages"
    RESULT_VARIABLE git_diff_index_result COMMAND_ECHO STDERR
  )
  if(NOT "${git_diff_index_result}" EQUAL "0")
    # commit changes
    execute_process(
      COMMAND "${GIT_EXECUTABLE}" commit -am "Updating Sphinx docs - ${message}"
      WORKING_DIRECTORY "${EINSUMS_BINARY_DIR}/docs/gh-pages"
      RESULT_VARIABLE git_commit_result
      ERROR_VARIABLE git_pull_result_message COMMAND_ECHO STDERR
    )
    if(NOT "${git_commit_result}" EQUAL "0")
      message(
        FATAL_ERROR "Committing to the GitHub pages branch failed: ${git_pull_result_message}."
      )
    endif()

    # push everything up to github
    execute_process(
      COMMAND "${GIT_EXECUTABLE}" push --force
      WORKING_DIRECTORY "${EINSUMS_BINARY_DIR}/docs/gh-pages"
      RESULT_VARIABLE git_push_result
      ERROR_VARIABLE git_push_result_message COMMAND_ECHO STDERR
    )
    if(NOT "${git_push_result}" EQUAL "0")
      message(FATAL_ERROR "Pushing to the GitHub pages branch failed: ${git_push_result_message}.")
    endif()
  endif()
endmacro()

if(NOT GIT_REPOSITORY)
  set(GIT_REPOSITORY git@github.com:Einsums/einsums-docs.git --branch master)
endif()

if(EXISTS "${EINSUMS_BINARY_DIR}/docs/gh-pages")
  execute_process(
    COMMAND "${GIT_EXECUTABLE}" pull --rebase
    WORKING_DIRECTORY "${EINSUMS_BINARY_DIR}/docs/gh-pages"
    RESULT_VARIABLE git_pull_result
    ERROR_VARIABLE git_pull_result_message COMMAND_ECHO STDERR
  )
  if(NOT "${git_pull_result}" EQUAL "0")
    message(FATAL_ERROR "Updating the GitHub pages branch failed: ${git_pull_result_message}.")
  endif()
else()
  execute_process(
    COMMAND "${GIT_EXECUTABLE}" clone ${GIT_REPOSITORY} gh-pages
    RESULT_VARIABLE git_clone_result
    ERROR_VARIABLE git_pull_result_message COMMAND_ECHO STDERR
  )
  if(NOT "${git_clone_result}" EQUAL "0")
    message(
      FATAL_ERROR
        "Cloning the GitHub pages branch failed: ${git_pull_result_message}. Trying to clone ${GIT_REPOSITORY}"
    )
  endif()
endif()

# We copy the documentation files from DOCS_SOURCE
set(DOCS_SOURCE "${EINSUMS_BINARY_DIR}/share/einsums/docs")

# Turn the string of output formats back into a list
string(REGEX REPLACE " " ";" EINSUMS_WITH_DOCUMENTATION_OUTPUT_FORMATS
                     "${EINSUMS_WITH_DOCUMENTATION_OUTPUT_FORMATS}"
)

# If a branch name has been set, we copy files to a corresponding directory
if(EINSUMS_WITH_GIT_BRANCH)
  message("Updating branch directory, " "EINSUMS_WITH_GIT_BRANCH=\"${EINSUMS_WITH_GIT_BRANCH}\"")
  set(DOCS_BRANCH_DEST "${EINSUMS_BINARY_DIR}/docs/gh-pages/branches/${EINSUMS_WITH_GIT_BRANCH}")
  file(REMOVE_RECURSE "${DOCS_BRANCH_DEST}")
  if("html" IN_LIST EINSUMS_WITH_DOCUMENTATION_OUTPUT_FORMATS)
    file(
      COPY "${DOCS_SOURCE}/html"
      DESTINATION "${DOCS_BRANCH_DEST}"
      PATTERN "*.buildinfo" EXCLUDE
    )
    add_docs("branches/${EINSUMS_WITH_GIT_BRANCH}/*")
    commit_docs("branch (html)")
  endif()
  if("singlehtml" IN_LIST EINSUMS_WITH_DOCUMENTATION_OUTPUT_FORMATS)
    file(
      COPY "${DOCS_SOURCE}/singlehtml"
      DESTINATION "${DOCS_BRANCH_DEST}"
      PATTERN "*.buildinfo" EXCLUDE
    )
    add_docs("branches/${EINSUMS_WITH_GIT_BRANCH}/*")
    commit_docs("branch (singlehtml)")
  endif()
  if("latexpdf" IN_LIST EINSUMS_WITH_DOCUMENTATION_OUTPUT_FORMATS)
    if(EXISTS "${DOCS_SOURCE}/latexpdf/latex/EINSUMS.pdf")
      file(COPY "${DOCS_SOURCE}/latexpdf/latex/EINSUMS.pdf" DESTINATION "${DOCS_BRANCH_DEST}/pdf/")
      add_docs("branches/${EINSUMS_WITH_GIT_BRANCH}/*")
      commit_docs("branch (latexpdf)")
    endif()
  endif()

  # special handling of dependency report files
  if(EXISTS "${DOCS_SOURCE}/report")
    file(COPY "${DOCS_SOURCE}/report" DESTINATION "${DOCS_BRANCH_DEST}")
    add_docs("branches/${EINSUMS_WITH_GIT_BRANCH}/*")
    commit_docs("branch (depreport)")
  endif()
endif()

# If a tag name has been set, we copy files to a corresponding directory
if(EINSUMS_WITH_GIT_TAG)
  message("Updating tag directory, " "EINSUMS_WITH_GIT_TAG=\"${EINSUMS_WITH_GIT_TAG}\"")
  set(DOCS_TAG_DEST "${EINSUMS_BINARY_DIR}/docs/gh-pages/tags/${EINSUMS_WITH_GIT_TAG}")
  file(REMOVE_RECURSE "${DOCS_TAG_DEST}")
  if("html" IN_LIST EINSUMS_WITH_DOCUMENTATION_OUTPUT_FORMATS)
    file(
      COPY "${DOCS_SOURCE}/html"
      DESTINATION "${DOCS_TAG_DEST}"
      PATTERN "*.buildinfo" EXCLUDE
    )
    add_docs("tags/${EINSUMS_WITH_GIT_TAG}/*")
    commit_docs("tag (html)")
  endif()
  if("singlehtml" IN_LIST EINSUMS_WITH_DOCUMENTATION_OUTPUT_FORMATS)
    file(
      COPY "${DOCS_SOURCE}/singlehtml"
      DESTINATION "${DOCS_TAG_DEST}"
      PATTERN "*.buildinfo" EXCLUDE
    )
    add_docs("tags/${EINSUMS_WITH_GIT_TAG}/*")
    commit_docs("tag (singlehtml)")
  endif()
  if("latexpdf" IN_LIST EINSUMS_WITH_DOCUMENTATION_OUTPUT_FORMATS)
    file(
      COPY "${DOCS_SOURCE}/latexpdf/latex/EINSUMS.pdf"
      DESTINATION "${DOCS_TAG_DEST}/pdf/"
      PATTERN "*.buildinfo" EXCLUDE
    )
    add_docs("tags/${EINSUMS_WITH_GIT_TAG}/*")
    commit_docs("tag (latexpdf)")
  endif()

  # special handling of dependency report files
  if(EXISTS "${DOCS_SOURCE}/report")
    file(COPY "${DOCS_SOURCE}/report" DESTINATION "${DOCS_TAG_DEST}")
    add_docs("tags/${EINSUMS_WITH_GIT_TAG}/*")
    commit_docs("tag (depreport)")
  endif()

  # If a tag name has been set and it is a suitable version number, we also copy files to the
  # "latest" directory. The regex only matches full version numbers with three numerical components
  # (X.Y.Z). It does not match release candidates or other non-version tag names.
  if("${EINSUMS_WITH_GIT_TAG}" MATCHES "^v[0-9]+\\.[0-9]+\\.[0-9]+$")
    message("Updating latest directory")
    set(DOCS_LATEST_DEST "${EINSUMS_BINARY_DIR}/docs/gh-pages/latest")
    file(REMOVE_RECURSE "${DOCS_LATEST_DEST}")
    if("html" IN_LIST EINSUMS_WITH_DOCUMENTATION_OUTPUT_FORMATS)
      file(
        COPY "${DOCS_SOURCE}/html"
        DESTINATION "${DOCS_LATEST_DEST}"
        PATTERN "*.buildinfo" EXCLUDE
      )
      add_docs("latest/*")
      commit_docs("latest (html)")
    endif()
    if("singlehtml" IN_LIST EINSUMS_WITH_DOCUMENTATION_OUTPUT_FORMATS)
      file(
        COPY "${DOCS_SOURCE}/singlehtml"
        DESTINATION "${DOCS_LATEST_DEST}"
        PATTERN "*.buildinfo" EXCLUDE
      )
      add_docs("latest/*")
      commit_docs("latest (singlehtml)")
    endif()
    if("latexpdf" IN_LIST EINSUMS_WITH_DOCUMENTATION_OUTPUT_FORMATS)
      file(
        COPY "${DOCS_SOURCE}/latexpdf/latex/EINSUMS.pdf"
        DESTINATION "${DOCS_LATEST_DEST}/pdf/"
        PATTERN "*.buildinfo" EXCLUDE
      )
      add_docs("latest/*")
      commit_docs("latest (latexpdf)")
    endif()

    # special handling of dependency report files
    if(EXISTS "${DOCS_SOURCE}/report")
      file(COPY "${DOCS_SOURCE}/report" DESTINATION "${DOCS_LATEST_DEST}")
      add_docs("latest/*")
      commit_docs("latest (depreport)")
    endif()
  endif()
endif()
