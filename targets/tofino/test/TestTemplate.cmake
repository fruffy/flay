# This file defines how a test should be written for a particular target. This is used by testutils

# Add a single test to the testsuite. Arguments: - TAG is a label for the set of test suite where
# this test belongs (for example, p4ctest) - DRIVER is the script that is used to run the test and
# compare the results - ALIAS is a possibly different name for the test such that the same p4
# program can be used in different test configurations. Must be unique across the test suite. -
# P4TEST is the name of the p4 program to test (path relative to the p4c directory) - TARGET is the
# target to test against - ARCH is the p4 architecture - TEST_ARGS is a list of arguments to pass to
# the test - CMAKE_ARGS are additional arguments to pass to the test
#
# It generates a ${p4test}.test file invoking ${driver} on the p4 program with command line
# arguments ${args} Sets the timeout on tests at 300s. For the slow CI machines.
function(p4tools_add_test_with_args)
  # Parse arguments.
  set(options)
  set(oneValueArgs TAG DRIVER ALIAS P4TEST TARGET ARCH CONTROL_PLANE_UPDATES)
  set(multiValueArgs TEST_ARGS CMAKE_ARGS)
  cmake_parse_arguments(
    TOOLS_FLAY_TNA_TESTS "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN}
  )
  # Set some lowercase variables for convenience.
  set(tag ${TOOLS_FLAY_TNA_TESTS_TAG})
  set(driver ${TOOLS_FLAY_TNA_TESTS_DRIVER})
  set(alias ${TOOLS_FLAY_TNA_TESTS_ALIAS})
  set(p4test ${TOOLS_FLAY_TNA_TESTS_P4TEST})
  set(target ${TOOLS_FLAY_TNA_TESTS_TARGET})
  set(arch ${TOOLS_FLAY_TNA_TESTS_ARCH})
  set(test_args ${TOOLS_FLAY_TNA_TESTS_TEST_ARGS})
  set(cmake_args ${TOOLS_FLAY_TNA_TESTS_CMAKE_ARGS})
  set(control_plane_updates ${TOOLS_FLAY_TNA_TESTS_CONTROL_PLANE_UPDATES})

  # This is the actual test processing.
  p4c_test_set_name(__testname ${tag} ${alias})
  string(REGEX REPLACE ".p4" "" aliasname ${alias})
  set(__testfile "${FLAY_DIR}/${tag}/${alias}.test")
  set(__testfolder "${FLAY_DIR}/${tag}/${aliasname}.out")
  get_filename_component(__testdir ${p4test} DIRECTORY)
  file(WRITE ${__testfile} "#! /usr/bin/env bash\n")
  file(APPEND ${__testfile} "# Generated file, modify with care\n\n")
  file(APPEND ${__testfile} "set -e\n")
  file(APPEND ${__testfile} "cd ${P4C_BINARY_DIR}\n")

  if (control_plane_updates)
    set(test_args "${test_args} --config-update-pattern \"${control_plane_updates}\"")
  endif()

  file(APPEND ${__testfile} "${driver} --target ${target} --arch ${arch} "
                            "${test_args} \"$@\" ${p4test}\n"
  )

  execute_process(COMMAND chmod +x ${__testfile})
  separate_arguments(__args UNIX_COMMAND ${cmake_args})
  add_test(NAME ${__testname} COMMAND ${tag}/${alias}.test ${__args} WORKING_DIRECTORY ${FLAY_DIR})
  if(NOT DEFINED ${tag}_timeout)
    set(${tag}_timeout 420)
  endif()
  set_tests_properties(${__testname} PROPERTIES LABELS ${tag} TIMEOUT ${${tag}_timeout})
endfunction(p4tools_add_test_with_args)
