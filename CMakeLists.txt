# CMake file for P4Flay.

include(common)

project(flay)

# Declare common P4Flay variables.
set(FLAY_DIR ${P4C_BINARY_DIR}/flay)
set(FLAY_DRIVER "${CMAKE_CURRENT_BINARY_DIR}/p4flay")

message("APPENDING ${CMAKE_MODULE_PATH}")

# ############### gRPC TODO: gRPC is horribly bloated and messes up the entire installation. Find a
# simple way to add this awful dependency.
set(FETCHCONTENT_QUIET_PREV ${FETCHCONTENT_QUIET})
set(FETCHCONTENT_QUIET OFF)
set(gRPC_BUILD_GRPC_CPP_PLUGIN ON CACHE BOOL "Build grpc_cpp_plugin")
set(gRPC_BUILD_GRPC_CSHARP_PLUGIN OFF CACHE BOOL "Build grpc_csharp_plugin")
set(gRPC_BUILD_CSHARP_EXT OFF CACHE BOOL "Build C# extensions")
set(gRPC_BUILD_GRPC_NODE_PLUGIN OFF CACHE BOOL "Build grpc_node_plugin")
set(gRPC_BUILD_GRPC_OBJECTIVE_C_PLUGIN OFF CACHE BOOL "Build grpc_objective_c_plugin")
set(gRPC_BUILD_GRPC_PHP_PLUGIN OFF CACHE BOOL "Build grpc_php_plugin")
set(gRPC_BUILD_GRPC_PYTHON_PLUGIN OFF CACHE BOOL "Build grpc_python_plugin")
set(gRPC_BUILD_GRPC_RUBY_PLUGIN OFF CACHE BOOL "Build grpc_ruby_plugin")
set(gRPC_ZLIB_PROVIDER "package" CACHE STRING "Provider of zlib library")
set(gRPC_CARES_PROVIDER "package" CACHE STRING "Provider of c-ares library")
set(gRPC_RE2_PROVIDER "package" CACHE STRING "Provider of re2 library")
set(gRPC_SSL_PROVIDER "package" CACHE STRING "Provider of ssl library")
set(gRPC_PROTOBUF_PROVIDER "package" CACHE STRING "Provider of protobuf library")
set(gRPC_PROTOBUF_PACKAGE_TYPE "MODULE" CACHE STRING "Algorithm for searching protobuf package")
set(gRPC_USE_PROTO_LITE ON CACHE BOOL "Use protobuf-lite")

# Unity builds do not work for gRPC...
set(SAVED_CMAKE_UNITY_BUILD ${CMAKE_UNITY_BUILD})
set(CMAKE_UNITY_BUILD OFF)
fetchcontent_declare(
  gRPC
  GIT_REPOSITORY https://github.com/grpc/grpc
  GIT_TAG v1.53.0
  GIT_SHALLOW TRUE
  PATCH_COMMAND git apply ${CMAKE_CURRENT_SOURCE_DIR}/grpc.patch  || git apply ${CMAKE_CURRENT_SOURCE_DIR}/grpc.patch  -R --check && echo "Patch does not apply because the patch was already applied."
  GIT_CONFIG core.shallow-submodules=true
)
fetchcontent_makeavailable(gRPC)
# Reset unity builds to the previous state...
set(CMAKE_UNITY_BUILD ${SAVED_CMAKE_UNITY_BUILD})
set(FETCHCONTENT_QUIET ${FETCHCONTENT_QUIET_PREV})


set(GRPC_CPP_PLUGIN_EXECUTABLE $<TARGET_FILE:grpc_cpp_plugin>)
set(GRPC_INCLUDE_DIR "${grpc_SOURCE_DIR}/include")

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
  COMMAND ${PROTOBUF_PROTOC_EXECUTABLE} -I${FLAY_P4RUNTIME_DIR} -I ${P4RUNTIME_STD_DIR}
          -I${P4C_SOURCE_DIR}/control-plane -I${Protobuf_INCLUDE_DIR} --cpp_out
          ${CMAKE_CURRENT_BINARY_DIR}/control_plane/protobuf --python_out
          ${CMAKE_CURRENT_BINARY_DIR}/control_plane/protobuf ${FLAY_P4RUNTIME_INFO_PROTO}
  DEPENDS ${FLAY_P4RUNTIME_INFO_PROTO} flay_runtime_dir
  COMMENT "Generating Flay Protobuf files."
)

add_custom_command(
  OUTPUT ${P4RUNTIME_GRPC_GEN_SRCS} ${P4RUNTIME_GRPC_GEN_HDRS}
  COMMAND ${PROTOBUF_PROTOC_EXECUTABLE} -I${P4RUNTIME_STD_DIR} -I${P4C_SOURCE_DIR}/control-plane
          -I${Protobuf_INCLUDE_DIR} --grpc_out ${P4C_BINARY_DIR}/control-plane
          --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN_EXECUTABLE} ${P4RUNTIME_RT_PROTO}
  DEPENDS ${P4RUNTIME_RT_PROTO} controlplane grpc_cpp_plugin protoc
  COMMENT "Generating Flay gRPC server."
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

# Source files for Flay gRPC server.
set(FLAY_GRPC_SOURCES ${P4RUNTIME_GRPC_GEN_SRCS}
                      ${CMAKE_CURRENT_SOURCE_DIR}/service/flay_server.cpp
)
add_library(flay_grpc STATIC ${FLAY_GRPC_SOURCES})
target_include_directories(
  flay_grpc SYSTEM BEFORE PUBLIC ${Protobuf_INCLUDE_DIR} ${GRPC_INCLUDE_DIR}
)
target_link_libraries(flay_grpc grpc++)

# Source files for flay.
set(FLAY_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/options.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/flay.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/control_plane/protobuf/protobuf.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/control_plane/id_to_ir_map.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/control_plane/symbolic_state.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/collapse_mux.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/execution_state.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/expression_resolver.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/externs.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/parser_stepper.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/program_info.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/stepper.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/substitute_placeholders.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/symbolic_executor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/table_executor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/target.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/logging.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/passes/elim_dead_code.cpp
)

# Flay libraries.
set(FLAY_LIBS p4tools-common flay_runtime)

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
