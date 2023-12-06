# CMake file for P4Flay.
project(flay)

# Add cmake directory to our module path. Also add cmake directory from P4C
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Declare common P4Flay variables.
set(FLAY_DIR ${P4C_BINARY_DIR}/flay)
set(FLAY_DRIVER "${CMAKE_CURRENT_BINARY_DIR}/p4flay")

# ############### Protobuf generation
set(FLAY_P4RUNTIME_DIR ${CMAKE_CURRENT_SOURCE_DIR}/control_plane/protobuf)
set(FLAY_P4RUNTIME_INFO_PROTO ${FLAY_P4RUNTIME_DIR}/flaytests.proto)
set(FLAY_P4RUNTIME_INFO_GEN_SRCS ${CMAKE_CURRENT_BINARY_DIR}/control_plane/protobuf/flaytests.pb.cc)
set(FLAY_P4RUNTIME_INFO_GEN_HDRS ${CMAKE_CURRENT_BINARY_DIR}/control_plane/protobuf/flaytests.pb.h)

set(P4RUNTIME_DIR ${P4C_BINARY_DIR}/control-plane/p4/v1)
set(P4RUNTIME_RT_PROTO ${P4RUNTIME_STD_DIR}/p4/v1/p4runtime.proto)
set(P4RUNTIME_GRPC_GEN_SRCS ${P4RUNTIME_DIR}/p4runtime.grpc.pb.cc)
set(P4RUNTIME_GRPC_GEN_HDRS ${P4RUNTIME_DIR}/p4runtime.grpc.pb.h)

add_custom_target(
  flay_runtime_dir ${CMAKE_COMMAND} -E make_directory
                   ${CMAKE_CURRENT_BINARY_DIR}/control_plane/protobuf
)

# Generate source code from .proto using protoc. The output is placed in the build directory inside
# `control_plane` directory
add_custom_command(
  OUTPUT ${FLAY_P4RUNTIME_INFO_GEN_SRCS} ${FLAY_P4RUNTIME_INFO_GEN_HDRS}
  COMMAND ${Protobuf_PROTOC_EXECUTABLE} -I${FLAY_P4RUNTIME_DIR} -I ${P4RUNTIME_STD_DIR}
          -I${P4C_SOURCE_DIR}/control-plane -I${Protobuf_INCLUDE_DIR} --cpp_out
          ${CMAKE_CURRENT_BINARY_DIR}/control_plane/protobuf --python_out
          ${CMAKE_CURRENT_BINARY_DIR}/control_plane/protobuf ${FLAY_P4RUNTIME_INFO_PROTO}
  DEPENDS ${FLAY_P4RUNTIME_INFO_PROTO} flay_runtime_dir
  COMMENT "Generating Flay Protobuf files."
)

add_dependencies(flay_runtime_dir controlplane)

add_library(flay_runtime STATIC ${FLAY_P4RUNTIME_INFO_GEN_SRCS})
target_include_directories(flay_runtime SYSTEM BEFORE PUBLIC ${Protobuf_INCLUDE_DIR})

# Silence various warnings as the root issue is out of our control, example
# https://github.com/protocolbuffers/protobuf/issues/7140
set_source_files_properties(
  ${FLAY_P4RUNTIME_INFO_GEN_SRCS} {$FLAY_P4RUNTIME_INFO_GEN_HDRS}
  PROPERTIES COMPILE_FLAGS "-Wno-unused-parameter -Wno-array-bounds -Wno-error"
)

include(GRpc)
p4c_obtain_grpc()
add_subdirectory(service)

# Source files for flay.
set(FLAY_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/options.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/flay.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/control_plane/protobuf/protobuf.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/control_plane/control_plane_objects.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/control_plane/id_to_ir_map.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/control_plane/symbolic_state.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/collapse_mux.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/compiler_target.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/execution_state.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/expression_resolver.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/externs.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/parser_stepper.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/program_info.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/reachability.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/stepper.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/substitute_placeholders.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/symbolic_executor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/table_executor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/target.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/passes/elim_dead_code.cpp
)

# GTest source files for P4Flay.
set(
  FLAY_GTEST_SOURCES
  #  # XXX These should be in a library.
  ${P4C_SOURCE_DIR}/test/gtest/helpers.cpp
  ${P4C_SOURCE_DIR}/test/gtest/gtestp4c.cpp

  test/core/collapse_mux.cpp
)

# Flay libraries.
set(FLAY_LIBS p4tools-common flay_runtime p4tools-control-plane)

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
set(IR_DEF_FILES ${IR_DEF_FILES} ${CMAKE_CURRENT_SOURCE_DIR}/flay.def PARENT_SCOPE)

# Convert the list of files into #includes
foreach(include_file ${include_files})

endforeach()

# Fill the template
configure_file(register.h.in register.h)

add_p4tools_library(libflay ${FLAY_SOURCES})
target_link_libraries(libflay flay_grpc)
add_dependencies(libflay p4tools-common controlplane)

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

if(ENABLE_GTESTS)
  add_executable(flay-gtest ${FLAY_GTEST_SOURCES})
  target_link_libraries(
    flay-gtest
    PRIVATE libflay
    PRIVATE gtest
    ${FLAY_LIBS}
    ${P4C_LIBRARIES}
    ${P4C_LIB_DEPS}
  )

  if(ENABLE_TESTING)
    add_test(NAME flay-gtest COMMAND flay-gtest)
    set_tests_properties(flay-gtest PROPERTIES LABELS "gtest-flay")
  endif()

endif()
