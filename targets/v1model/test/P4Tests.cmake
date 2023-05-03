# This file defines how we write the tests we generate.
# ##################################################################################################
# TEST PROGRAMS
# ##################################################################################################
set(V1_SEARCH_PATTERNS "include.*v1model.p4" "main|common_v1_test")
# General BMv2 tests supplied by the compiler.
set(P4TESTS_FOR_BMV2
    "${P4C_SOURCE_DIR}/testdata/p4_16_samples/*.p4"
    "${P4C_SOURCE_DIR}/testdata/p4_16_samples/dash/*.p4"
    "${P4C_SOURCE_DIR}/testdata/p4_16_samples/fabric_*/fabric.p4"
    "${P4C_SOURCE_DIR}/testdata/p4_16_samples/omec/*.p4"
    "${P4C_SOURCE_DIR}/testdata/p4_16_samples/pins/*.p4"
)
p4c_find_tests("${P4TESTS_FOR_BMV2}" P4_16_V1_TESTS INCLUDE "${V1_SEARCH_PATTERNS}" EXCLUDE "")

macro(
  flay_add_test_with_args
  tag
  driver
  alias
  p4test
  test_args
  cmake_args
)
  set(__testfile "${P4C_BINARY_DIR}/${tag}/${alias}.test")
  file(WRITE ${__testfile} "#! /usr/bin/env bash\n")
  file(APPEND ${__testfile} "# Generated file, modify with care\n\n")
  file(APPEND ${__testfile} "set -e\n")
  file(APPEND ${__testfile} "cd ${P4C_BINARY_DIR}\n")
  file(APPEND ${__testfile} "${driver} ${test_args} \"$@\" ${p4test}")
  execute_process(COMMAND chmod +x ${__testfile})
  p4c_test_set_name(__testname ${tag} ${alias})
  separate_arguments(__args UNIX_COMMAND ${cmake_args})
  add_test(NAME ${__testname} COMMAND ${tag}/${alias}.test ${__args}
           WORKING_DIRECTORY ${P4C_BINARY_DIR}
  )
  if(NOT DEFINED ${tag}_timeout)
    set(${tag}_timeout 300)
  endif()
  set_tests_properties(${__testname} PROPERTIES LABELS ${tag} TIMEOUT ${${tag}_timeout})
endmacro(flay_add_test_with_args)

function(flay_add_tests)
  # Parse arguments.
  set(options)
  set(oneValueArgs TAG DRIVER)
  set(multiValueArgs TESTS)
  cmake_parse_arguments(FLAY_ADD_TESTS "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
  foreach(f IN LISTS FLAY_ADD_TESTS_TESTS)
    get_filename_component(f_basename ${f} NAME)
    flay_add_test_with_args(${FLAY_ADD_TESTS_TAG} ${FLAY_ADD_TESTS_DRIVER} ${f_basename} ${f} "" "")
  endforeach()
endfunction()

set(FLAY_DRIVER "${P4C_BINARY_DIR}/p4flay --target bmv2 --arch v1model")

flay_add_tests(TAG "flay/v1model" DRIVER ${FLAY_DRIVER} TESTS "${P4_16_V1_TESTS}")

# Custom BMv2 tests.
set(TESTGEN_BMV2_P416_TESTS "${CMAKE_CURRENT_LIST_DIR}/p4-programs/*.p4")
p4c_find_tests(
  "${TESTGEN_BMV2_P416_TESTS}" BMV2_P4_16_V1_TESTS INCLUDE "${V1_SEARCH_PATTERNS}" EXCLUDE ""
)
