# We currently do not support NIKSS on Apple.
if(NOT APPLE)
  if(ENABLE_TESTING)
    # Include the test subdirectory.
    message("-- Adding Flay nikss test suite.")
    include(test/P4Tests.cmake)
  endif()

  # Fetch the bpftool.
  set(FETCHCONTENT_QUIET_PREV ${FETCHCONTENT_QUIET})
  set(FETCHCONTENT_QUIET OFF)
  FetchContent_Declare(
    bpftool
    URL https://github.com/libbpf/bpftool/releases/download/v7.4.0/bpftool-v7.4.0-amd64.tar.gz
    URL_HASH
      SHA256=68862a7038a269e1d4af5fed53d2b4d763f1cd82a96caa49b76596453bae0c3e
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/bpftool USES_TERMINAL_DOWNLOAD TRUE
    GIT_PROGRESS TRUE
  )
  FetchContent_MakeAvailable(bpftool)
  execute_process(COMMAND "chmod 775 ${CMAKE_CURRENT_SOURCE_DIR}/bpftool/bpftool")

  FetchContent_Declare(trex
    URL https://trex-tgn.cisco.com/trex/release/v3.05.tar.gz
    URL_HASH
      SHA256=15e9ac6da554b9bdbec575da5f1010e3d2b07c8806be889e074ae8a776636c0b
    GIT_PROGRESS TRUE
    GIT_SHALLOW TRUE
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/trex
    PATCH_COMMAND
      patch -p0 ${CMAKE_CURRENT_SOURCE_DIR}/trex.patch || echo
      "Patch does not apply because the patch was already applied."
  )
  FetchContent_MakeAvailable(trex)

  set(FETCHCONTENT_QUIET ${FETCHCONTENT_QUIET_PREV})




  # Source files for the main flay.
  set(FLAY_SOURCES
      ${FLAY_SOURCES}
      # Nikss base steppers and information.
      ${CMAKE_CURRENT_SOURCE_DIR}/base/expression_resolver.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/base/program_info.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/base/stepper.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/base/table_executor.cpp
      # Psa steppers and information.
      ${CMAKE_CURRENT_SOURCE_DIR}/psa/expression_resolver.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/psa/program_info.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/psa/stepper.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/psa/table_executor.cpp
      # Common Nikss utilities.
      ${CMAKE_CURRENT_SOURCE_DIR}/constants.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/control_plane_objects.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/symbolic_state.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/target.cpp
      PARENT_SCOPE)

endif()
