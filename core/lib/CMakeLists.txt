set(FLAY_LIB_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/analysis.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/collapse_dataplane_variables.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/expression_strength_reduction.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/simplify_expression.cpp
)

add_library(flay-lib STATIC ${FLAY_LIB_SOURCES})
target_link_libraries(flay-lib
    PRIVATE ${P4C_LIB_DEPS}
    PUBLIC p4tools-common
)
