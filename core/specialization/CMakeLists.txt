set(FLAY_SPECIALIZATION_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/passes/elim_dead_code.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/passes/substitute_expressions.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/flay_service.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/reachability_map.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/service_wrapper.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/service_wrapper_bfruntime.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/service_wrapper_p4runtime.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/substitution_map.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/z3_substitution_map.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/z3solver_reachability.cpp
)

add_library(flay-specialization STATIC ${FLAY_SPECIALIZATION_SOURCES})
target_link_libraries(flay-specialization ${P4C_LIB_DEPS} flay-control-plane flay-lib)
add_dependencies(flay-specialization p4tools-common)


