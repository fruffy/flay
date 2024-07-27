
add_custom_target(
  flay_runtime_dir ${CMAKE_COMMAND} -E make_directory
                   ${CMAKE_CURRENT_BINARY_DIR}/control_plane/p4runtime &&
                   ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/control_plane/bfruntime
)

# Unity builds do not work for Protobuf...
set(CMAKE_UNITY_BUILD_PREV ${CMAKE_UNITY_BUILD})
set(CMAKE_UNITY_BUILD OFF)
add_library(flay-runtime OBJECT ${FLAY_P4RUNTIME_TESTS_PROTO} ${FLAY_BFRUNTIME_TESTS_PROTO})
target_include_directories(flay-runtime PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(flay-runtime PRIVATE controlplane p4tools-runtime-proto)
add_dependencies(flay-runtime flay_runtime_dir)
protobuf_generate(
  TARGET flay-runtime
  LANGUAGE cpp python
  IMPORT_DIRS ${P4RUNTIME_STD_DIR} ${Protobuf_INCLUDE_DIRS} ${P4C_SOURCE_DIR}/control-plane ${P4C_SOURCE_DIR}
  PROTOC_OUT_DIR ${P4C_BINARY_DIR}
  OUT_VAR FLAY_P4RUNTIME_INFO_GEN_SRCS
  DEPENDS ${FLAY_P4RUNTIME_TESTS_PROTO}
)
# Reset temporary variable modifications.
set(CMAKE_UNITY_BUILD ${CMAKE_UNITY_BUILD_PREV})


# Source files for flay.
set(FLAY_CONTROL_PLANE_SOURCES
    ${FLAY_CONTROL_PLANE_DIR}/bfruntime/protobuf.cpp
    ${FLAY_CONTROL_PLANE_DIR}/p4runtime/protobuf.cpp
    ${FLAY_CONTROL_PLANE_DIR}/control_plane_objects.cpp
    ${FLAY_CONTROL_PLANE_DIR}/id_to_ir_map.cpp
    ${FLAY_CONTROL_PLANE_DIR}/substitute_variable.cpp
    ${FLAY_CONTROL_PLANE_DIR}/symbolic_state.cpp
)

add_library(flay-control-plane STATIC ${FLAY_CONTROL_PLANE_SOURCES})
target_link_libraries(flay-control-plane
                      PRIVATE flay-runtime p4tools-control-plane flay-lib ${P4C_LIB_DEPS}
                      PUBLIC controlplane
)
