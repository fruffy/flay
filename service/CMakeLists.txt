# Source files for Flay gRPC server.
add_custom_command(
  OUTPUT ${P4RUNTIME_GRPC_GEN_SRCS} ${P4RUNTIME_GRPC_GEN_HDRS}
  COMMAND ${PROTOBUF_PROTOC_EXECUTABLE} -I${P4RUNTIME_STD_DIR} -I${P4C_SOURCE_DIR}/control-plane
          -I${Protobuf_INCLUDE_DIR} --grpc_out ${P4C_BINARY_DIR}/control-plane
          --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN_EXECUTABLE} ${P4RUNTIME_RT_PROTO}
  DEPENDS ${P4RUNTIME_RT_PROTO} controlplane grpc_cpp_plugin protoc
  COMMENT "Generating Flay gRPC server."
)
set(FLAY_GRPC_SOURCES ${P4RUNTIME_GRPC_GEN_SRCS} flay_server.cpp)
add_library(flay_grpc STATIC ${FLAY_GRPC_SOURCES})
target_include_directories(
  flay_grpc SYSTEM BEFORE PUBLIC ${Protobuf_INCLUDE_DIR} ${GRPC_INCLUDE_DIR}
)
target_link_libraries(flay_grpc grpc++)

# ##################################################################################################
# Flay Service Client #
# ##################################################################################################
set(FLAY_GRPC_CLIENT_SOURCES client_main.cpp client_options.cpp flay_client.cpp
                             ${flay_SOURCE_DIR}/lib/logging.cpp
)

set (FLAY_GRPC_CLIENT_LIBS p4ctoolkit ${LIBGC_LIBRARIES} controlplane-gen grpc++)
if (HAVE_LIBBACKTRACE)
set (FLAY_GRPC_CLIENT_LIBS "${FLAY_GRPC_CLIENT_LIBS};-lbacktrace")
endif ()

add_executable(flay_grpc_client ${FLAY_GRPC_CLIENT_SOURCES} ${P4RUNTIME_GRPC_GEN_SRCS})
target_link_libraries(flay_grpc_client ${FLAY_GRPC_CLIENT_LIBS})
target_include_directories(
  flay_grpc_client SYSTEM BEFORE PUBLIC ${Protobuf_INCLUDE_DIR} ${GRPC_INCLUDE_DIR}
)