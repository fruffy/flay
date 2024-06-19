# CMake file for P4Flay.
project(flay)

# Add cmake directory to our module path. Also add cmake directory from P4C
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

option(P4TOOLS_FLAY_WITH_GRPC "Build with gRPC support" OFF)

# Declare common P4Flay variables.
set(FLAY_DIR ${P4C_BINARY_DIR}/flay)
set(FLAY_DRIVER "${CMAKE_CURRENT_BINARY_DIR}/p4flay")
set(FLAY_REFERENCE_DRIVER "${flay_BINARY_DIR}/tools/flay_reference_checker")

# ############### Protobuf generation
set(FLAY_CONTROL_PLANE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/control_plane)
set(FLAY_P4RUNTIME_DIR ${FLAY_CONTROL_PLANE_DIR}/p4runtime)
set(FLAY_P4RUNTIME_TESTS_PROTO ${FLAY_P4RUNTIME_DIR}/flaytests.proto)
set(P4RUNTIME_DIR ${P4C_BINARY_DIR}/control-plane/p4/v1)
set(P4RUNTIME_RT_PROTO ${P4RUNTIME_STD_DIR}/p4/v1/p4runtime.proto)
set(FLAY_BFRUNTIME_DIR ${FLAY_CONTROL_PLANE_DIR}/bfruntime)
set(BFRUNTIME_RT_PROTO ${FLAY_BFRUNTIME_DIR}/bfruntime.proto)
set(BFRUNTIME_TESTS_PROTO ${FLAY_BFRUNTIME_DIR}/flaytests.proto)

add_custom_target(
  flay_runtime_dir ${CMAKE_COMMAND} -E make_directory
                   ${CMAKE_CURRENT_BINARY_DIR}/control_plane/p4runtime &&
                   ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/control_plane/bfruntime
)

# Unity builds do not work for Protobuf...
set(CMAKE_UNITY_BUILD_PREV ${CMAKE_UNITY_BUILD})
set(CMAKE_UNITY_BUILD OFF)
add_library(flay_p4runtime OBJECT ${FLAY_P4RUNTIME_TESTS_PROTO} ${BFRUNTIME_RT_PROTO} ${BFRUNTIME_TESTS_PROTO})
target_include_directories(flay_p4runtime PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(flay_p4runtime PRIVATE controlplane)
add_dependencies(flay_p4runtime flay_runtime_dir)
protobuf_generate(
  TARGET flay_p4runtime
  LANGUAGE cpp python
  IMPORT_DIRS ${P4RUNTIME_STD_DIR} ${Protobuf_INCLUDE_DIRS} ${P4C_SOURCE_DIR}/control-plane ${P4C_SOURCE_DIR}
  PROTOC_OUT_DIR ${P4C_BINARY_DIR}
  OUT_VAR FLAY_P4RUNTIME_INFO_GEN_SRCS
  DEPENDS ${FLAY_P4RUNTIME_TESTS_PROTO}
)
# Reset temporary variable modifications.
set(CMAKE_UNITY_BUILD ${CMAKE_UNITY_BUILD_PREV})

if(P4TOOLS_FLAY_WITH_GRPC)
  include(GRpc)
  p4c_obtain_grpc()
  add_subdirectory(service)
endif()

# Source files for flay.
set(FLAY_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/options.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/flay.cpp
    ${FLAY_CONTROL_PLANE_DIR}/bfruntime/protobuf.cpp
    ${FLAY_CONTROL_PLANE_DIR}/p4runtime/protobuf.cpp
    ${FLAY_CONTROL_PLANE_DIR}/control_plane_objects.cpp
    ${FLAY_CONTROL_PLANE_DIR}/id_to_ir_map.cpp
    ${FLAY_CONTROL_PLANE_DIR}/symbolic_state.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/analysis.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/collapse_dataplane_variables.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/compiler_result.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/execution_state.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/expression_resolver.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/externs.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/flay_service.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/node_map.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/parser_stepper.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/program_info.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/reachability_expression.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/reachability_map.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/service_wrapper.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/service_wrapper_bfruntime.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/service_wrapper_p4runtime.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/simplify_expression.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/stepper.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/substitution_expression.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/substitute_placeholders.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/substitution_map.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/symbolic_executor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/table_executor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/target.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/z3solver_reachability.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/passes/elim_dead_code.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/passes/substitute_expressions.cpp
)

# GTest source files for P4Flay.
set(FLAY_GTEST_SOURCES
  ${P4C_SOURCE_DIR}/test/gtest/helpers.cpp
  ${P4C_SOURCE_DIR}/test/gtest/gtestp4c.cpp
  ${CMAKE_CURRENT_LIST_DIR}/test/core/simplify_expression_test.cpp
)

# Flay libraries.
set(FLAY_LIBS
    PRIVATE p4tools-control-plane p4tools-common flay_p4runtime controlplane ${LIBGC_LIBRARIES}
)

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
      set(flay_targets_var "${flay_targets_var}    ${ext}_registerFlayTarget();\n")
    endif()
  endif()
endforeach(ext)

# Propagate def files set by target extensions upwards.
set(IR_DEF_FILES ${IR_DEF_FILES} ${CMAKE_CURRENT_SOURCE_DIR}/flay.def PARENT_SCOPE)

# Convert the list of files into #includes
foreach(include_file ${include_files})

endforeach()

# Fill the template
configure_file(register.h.in register.h)

add_library(flay STATIC ${FLAY_SOURCES})
if(P4TOOLS_FLAY_WITH_GRPC)
  target_link_libraries(flay PUBLIC flay_grpc flay_grpc_service)
endif()
target_link_libraries(flay ${FLAY_LIBS})

add_p4tools_executable(p4flay main.cpp)

target_link_libraries(
  p4flay PRIVATE flay ${FLAY_LIBS} ${P4C_LIBRARIES} ${P4C_LIB_DEPS}
)

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

# Utilities for testing.
add_subdirectory(tools)

if(ENABLE_GTESTS)
  add_executable(flay-gtest ${FLAY_GTEST_SOURCES})
  target_link_libraries(
    flay-gtest PRIVATE flay PRIVATE gtest ${FLAY_LIBS} ${P4C_LIBRARIES} ${P4C_LIB_DEPS}
  )

  if(ENABLE_TESTING)
    add_test(NAME flay-gtest COMMAND flay-gtest)
    set_tests_properties(flay-gtest PROPERTIES LABELS "gtest-flay")
  endif()

endif()
