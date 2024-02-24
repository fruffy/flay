# ##################################################################################################
# Reference File Checker
# ##################################################################################################
set(FLAY_REFERENCE_CHECKER_SOURCES reference_checker.cpp
                                   ${P4TOOLS_SOURCE_DIR}/common/lib/logging.cpp
)

add_executable(flay_reference_checker ${FLAY_REFERENCE_CHECKER_SOURCES})
target_link_libraries(flay_reference_checker libflay ${FLAY_LIBS})
