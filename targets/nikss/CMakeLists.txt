# We currently do not support NIKSS on Apple.
if(NOT APPLE)
  if(ENABLE_TESTING)
    # Include the test subdirectory.
    message("-- Adding Flay nikss test suite.")
    include(test/P4Tests.cmake)
  endif()
endif()

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

