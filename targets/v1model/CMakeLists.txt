if(ENABLE_TESTING)
  # Include the test subdirectory.
  message("-- Adding Flay v1model test suite")
  include(test/P4Tests.cmake)
endif()

# Source files for the main flay.
set(FLAY_SOURCES
    ${FLAY_SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/constants.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/expression_resolver.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/program_info.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/stepper.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/target.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/table_executor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/v1model.cpp
    PARENT_SCOPE
)

set(FLAY_LIBS ${FLAY_LIBS} ${P4C_LIBRARIES} ${P4C_LIB_DEPS} ${CMAKE_THREAD_LIBS_INIT} PARENT_SCOPE)
