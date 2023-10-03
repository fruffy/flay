# This file defines how to execute Flay on P4 programs. General test utilities.
include(${P4TOOLS_SOURCE_DIR}/cmake/TestUtils.cmake)
# This file defines how we write the tests we generate.
include(${CMAKE_CURRENT_LIST_DIR}/TestTemplate.cmake)

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
    # Custom tests
    "${CMAKE_CURRENT_LIST_DIR}/programs/*.p4"
)

p4c_find_tests("${P4TESTS_FOR_BMV2}" P4_16_V1_TESTS INCLUDE "${V1_SEARCH_PATTERNS}" EXCLUDE "")

# Filter some programs  because they have issues that are not captured with Xfails.
list(
  REMOVE_ITEM
  P4_16_V1_TESTS
  # These tests time out and require fixing.
  "${P4C_SOURCE_DIR}/testdata/p4_16_samples/runtime-index-2-bmv2.p4"
  "${P4C_SOURCE_DIR}/testdata/p4_16_samples/control-hs-index-test4.p4"
  "${P4C_SOURCE_DIR}/testdata/p4_16_samples/header-stack-ops-bmv2.p4"
  "${P4C_SOURCE_DIR}/testdata/p4_16_samples/issue3374.p4"
  "${P4C_SOURCE_DIR}/testdata/p4_16_samples/subparser-with-header-stack-bmv2.p4"
  "${P4C_SOURCE_DIR}/testdata/p4_16_samples/omec/up4.p4"
)

p4tools_add_tests(
  TESTS
  "${P4_16_V1_TESTS}"
  TAG
  "flay-p4c-bmv2-v1model"
  DRIVER
  ${FLAY_DRIVER}
  TARGET
  "bmv2"
  ARCH
  "v1model"
  TEST_ARGS
  "${EXTRA_OPTS} "
)

# Custom BMv2 tests.
set(TESTGEN_BMV2_P416_TESTS "${CMAKE_CURRENT_LIST_DIR}/p4-programs/*.p4")
p4c_find_tests(
  "${TESTGEN_BMV2_P416_TESTS}" BMV2_P4_16_V1_TESTS INCLUDE "${V1_SEARCH_PATTERNS}" EXCLUDE ""
)

# Include the list of failing tests.
include(${CMAKE_CURRENT_LIST_DIR}/BMv2V1ModelXfail.cmake)
