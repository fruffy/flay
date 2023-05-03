include(common)

project(flay)

# Source files for flay.
set(FLAY_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/options.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/flay.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/collapse_mux.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/execution_state.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/expression_resolver.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/externs.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/program_info.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/state_utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/stepper.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/symbolic_executor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/table_executor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/target.cpp
)

# Flay libraries.
set(FLAY_LIBS p4tools-common)

file(GLOB flay_targets RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/targets
     ${CMAKE_CURRENT_SOURCE_DIR}/targets/*
)
foreach(ext ${flay_targets})
  set(flay_targets_dir ${CMAKE_CURRENT_SOURCE_DIR}/targets/${ext}/)
  if(EXISTS ${flay_targets_dir}/CMakeLists.txt AND IS_DIRECTORY ${flay_targets_dir})
    # Generate an option that makes it possible to disable this extension.
    string(MAKE_C_IDENTIFIER ${ext} EXT_AS_IDENTIFIER)
    string(TOUPPER ${EXT_AS_IDENTIFIER} EXT_AS_OPTION_NAME)
    string(CONCAT ENABLE_EXT_OPTION "ENABLE_TOOLS_TARGET_" ${EXT_AS_OPTION_NAME})
    string(CONCAT EXT_HELP_TEXT "Build the " ${ext} " target")
    option(${ENABLE_EXT_OPTION} ${EXT_HELP_TEXT} ON)
    if(${ENABLE_EXT_OPTION})
      message("-- Enabling target ${ext}")
      add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/targets/${ext})
      set(include_statements_var
          "${include_statements_var}#include \"backends/p4tools/modules/flay/targets/${ext}/register.h\"\n"
      )
      set(compiler_targets_var "${compiler_targets_var}    ${ext}_registerCompilerTarget();\n")
      set(flay_targets_var "${flay_targets_var}    ${ext}_registerFlayTarget();\n")
    endif()
  endif()
endforeach(ext)

# Propagate def files set by target extensions upwards.
set(IR_DEF_FILES ${IR_DEF_FILES} PARENT_SCOPE)

# Convert the list of files into #includes
foreach(include_file ${include_files})

endforeach()

# Fill the template
configure_file(register.h.in register.h)

add_p4tools_library(libflay ${FLAY_SOURCES})

add_p4tools_executable(p4flay main.cpp)

target_link_libraries(p4flay libflay ${FLAY_LIBS})

# Add some convenience links for invoking flay.
add_custom_target(
  linkflay
  COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_BINARY_DIR}/p4flay
          ${P4C_BINARY_DIR}/p4flay
  COMMAND ${CMAKE_COMMAND} -E create_symlink ${P4C_BINARY_DIR}/p4include
          ${CMAKE_CURRENT_BINARY_DIR}/p4include
  DEPENDS update_includes
)

add_dependencies(p4flay linkflay)
