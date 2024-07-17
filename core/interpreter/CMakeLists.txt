set(FLAY_INTERPRETER_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/compiler_result.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/execution_state.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/expression_resolver.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/externs.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/node_map.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/parser_stepper.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/program_info.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/reachability_expression.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/stepper.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/substitute_placeholders.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/substitution_expression.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/symbolic_executor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/table_executor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/target.cpp
)

add_library(flay-interpreter STATIC ${FLAY_INTERPRETER_SOURCES})

target_link_libraries(flay-interpreter PRIVATE flay-control-plane ${P4C_LIB_DEPS})
add_dependencies(flay-interpreter flay-lib p4tools-common)
