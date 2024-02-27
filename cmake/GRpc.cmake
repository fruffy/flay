macro(p4c_obtain_grpc)
# TODO: gRPC is horribly bloated and messes up the entire installation. Find a
# simple way to add this awful dependency.
set(FETCHCONTENT_QUIET_PREV ${FETCHCONTENT_QUIET})
set(FETCHCONTENT_QUIET OFF)
set(CMAKE_FIND_LIBRARY_SUFFIXES_PREV ${CMAKE_FIND_LIBRARY_SUFFIXES_PREV})
set(CMAKE_FIND_LIBRARY_SUFFIXES .a)

set(gRPC_BUILD_GRPC_CPP_PLUGIN ON CACHE BOOL "Build grpc_cpp_plugin")
set(gRPC_BUILD_GRPC_CSHARP_PLUGIN OFF CACHE BOOL "Build grpc_csharp_plugin")
set(gRPC_BUILD_CSHARP_EXT OFF CACHE BOOL "Build C# extensions")
set(gRPC_BUILD_GRPC_NODE_PLUGIN OFF CACHE BOOL "Build grpc_node_plugin")
set(gRPC_BUILD_GRPC_OBJECTIVE_C_PLUGIN OFF CACHE BOOL "Build grpc_objective_c_plugin")
set(gRPC_BUILD_GRPC_PHP_PLUGIN OFF CACHE BOOL "Build grpc_php_plugin")
set(gRPC_BUILD_GRPC_PYTHON_PLUGIN OFF CACHE BOOL "Build grpc_python_plugin")
set(gRPC_BUILD_GRPC_RUBY_PLUGIN OFF CACHE BOOL "Build grpc_ruby_plugin")
set(gRPC_ABSL_PROVIDER "module" CACHE STRING "Provider of abseil library")
set(gRPC_ZLIB_PROVIDER "package" CACHE STRING "Provider of zlib library")
set(gRPC_CARES_PROVIDER "package" CACHE STRING "Provider of c-ares library")
set(gRPC_RE2_PROVIDER "module" CACHE STRING "Provider of re2 library")
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
  PATCH_COMMAND git apply ${CMAKE_CURRENT_SOURCE_DIR}/cmake/grpc.patch  || git apply ${CMAKE_CURRENT_SOURCE_DIR}/cmake/grpc.patch  -R --check && echo "Patch does not apply because the patch was already applied."
  GIT_CONFIG core.shallow-submodules=true
)
fetchcontent_makeavailable(gRPC)
# Reset unity builds to the previous state...
set(CMAKE_UNITY_BUILD ${SAVED_CMAKE_UNITY_BUILD})
set(FETCHCONTENT_QUIET ${FETCHCONTENT_QUIET_PREV})


set(GRPC_CPP_PLUGIN_EXECUTABLE $<TARGET_FILE:grpc_cpp_plugin>)
set(GRPC_INCLUDE_DIR "${grpc_SOURCE_DIR}/include")
set(CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES_PREV})

endmacro(p4c_obtain_grpc)

