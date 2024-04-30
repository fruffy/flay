if(ENABLE_TESTING)
  # Include the test subdirectory.
  message("-- Adding Flay fpga test suite.")
  include(test/P4Tests.cmake)
endif()

# Source files for the main flay.
set(FLAY_SOURCES
    ${FLAY_SOURCES}
    # Fpga base steppers and information.
    ${CMAKE_CURRENT_SOURCE_DIR}/base/expression_resolver.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/base/program_info.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/base/stepper.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/base/table_executor.cpp
    # Xsa steppers and information.
    ${CMAKE_CURRENT_SOURCE_DIR}/xsa/expression_resolver.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/xsa/program_info.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/xsa/stepper.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/xsa/table_executor.cpp
    # Common Fpga utilities.
    ${CMAKE_CURRENT_SOURCE_DIR}/constants.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/control_plane_objects.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/symbolic_state.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/target.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/fpga.cpp
    PARENT_SCOPE
)
