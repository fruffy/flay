# Source files for the main flay.
set(FLAY_SOURCES
    ${FLAY_SOURCES}
    # Tofino base steppers and information.
    ${CMAKE_CURRENT_SOURCE_DIR}/base/expression_resolver.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/base/program_info.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/base/stepper.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/base/table_executor.cpp
    # Tofino1 steppers and information.
    ${CMAKE_CURRENT_SOURCE_DIR}/tofino1/expression_resolver.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tofino1/program_info.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tofino1/stepper.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tofino1/table_executor.cpp
    # Common Tofino utilities.
    ${CMAKE_CURRENT_SOURCE_DIR}/constants.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/control_plane_objects.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/symbolic_state.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/target.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tofino.cpp
    PARENT_SCOPE
)

set(FLAY_LIBS ${FLAY_LIBS} ${P4C_LIBRARIES} ${P4C_LIB_DEPS} ${CMAKE_THREAD_LIBS_INIT} PARENT_SCOPE)
