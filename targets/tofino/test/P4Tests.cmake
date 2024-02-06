# This file defines how to execute Flay on P4 programs. General test utilities.
include(${P4TOOLS_SOURCE_DIR}/cmake/TestUtils.cmake)
# This file defines how we write the tests we generate.
include(${CMAKE_CURRENT_LIST_DIR}/TestTemplate.cmake)

# ##################################################################################################
# TEST PROGRAMS
# ##################################################################################################
set(TNA_SEARCH_PATTERNS "include.*tna.p4" "main")

set(P4TESTS_FOR_TOFINO
  # P4Studio tests
  "${CMAKE_CURRENT_LIST_DIR}/programs/opentofino/**/*.p4"
  # Custom tests
  "${CMAKE_CURRENT_LIST_DIR}/programs/common/*.p4"
)

p4c_find_tests("${P4TESTS_FOR_TOFINO}" TNA_TESTS INCLUDE "${TNA_SEARCH_PATTERNS}" EXCLUDE "")

# Filter some programs  because they have issues that are not captured with Xfails.
list(REMOVE_ITEM TNA_TESTS
  # These tests time out and require fixing.
)

set (EXTRA_OPTS "-I${CMAKE_CURRENT_LIST_DIR}/p4include -I${CMAKE_CURRENT_LIST_DIR}/programs/opentofino")

p4tools_add_tests(
  TESTS
  "${TNA_TESTS}"
  TAG
  "flay-tofino1-tna"
  DRIVER
  ${FLAY_DRIVER}
  TARGET
  "tofino1"
  ARCH
  "tna"
  TEST_ARGS
  "${EXTRA_OPTS} -D__TARGET_TOFINO__=1"
)

# Include the list of failing tests.
include(${CMAKE_CURRENT_LIST_DIR}/Tofino1Xfail.cmake)
