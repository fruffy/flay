# Anything linking with gRPC requires position independent code.
set(CMAKE_POSITION_INDEPENDENT_CODE_PREV ${CMAKE_POSITION_INDEPENDENT_CODE_PREV})
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(P4RUNTIME_GRPC_GEN_SRCS ${P4RUNTIME_DIR}/p4runtime.grpc.pb.cc)
set(P4RUNTIME_GRPC_GEN_HDRS ${P4RUNTIME_DIR}/p4runtime.grpc.pb.h)
# Source files for Flay gRPC server.
add_custom_command(
  OUTPUT ${P4RUNTIME_GRPC_GEN_SRCS} ${P4RUNTIME_GRPC_GEN_HDRS}
  COMMAND $<TARGET_FILE:protoc> -I${P4RUNTIME_STD_DIR} -I${P4C_SOURCE_DIR}/control-plane
          --proto_path ${Protobuf_INCLUDE_DIR} --grpc_out ${P4C_BINARY_DIR}/control-plane
          --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN_EXECUTABLE} ${P4RUNTIME_RT_PROTO}
  DEPENDS ${P4RUNTIME_RT_PROTO} controlplane grpc_cpp_plugin protoc
  COMMENT "Generating Flay gRPC server."
)
set(FLAY_GRPC_SOURCES ${P4RUNTIME_GRPC_GEN_SRCS} flay_grpc_service.cpp)
add_library(flay_grpc STATIC ${FLAY_GRPC_SOURCES})
target_link_libraries(flay_grpc PUBLIC grpc++ PRIVATE controlplane PRIVATE flay_runtime)

# ##################################################################################################
# Flay Service Client #
# ##################################################################################################
set(FLAY_GRPC_CLIENT_SOURCES client_main.cpp client_options.cpp flay_client.cpp
                             ${P4TOOLS_SOURCE_DIR}/common/lib/logging.cpp
)

set(FLAY_GRPC_CLIENT_LIBS
    PRIVATE
    p4ctoolkit
    ${LIBGC_LIBRARIES}
    PRIVATE
    controlplane
    PRIVATE
    grpc++
)
if(HAVE_LIBBACKTRACE)
  set(FLAY_GRPC_CLIENT_LIBS "${FLAY_GRPC_CLIENT_LIBS};-lbacktrace")
endif()

add_executable(flay_grpc_client ${FLAY_GRPC_CLIENT_SOURCES} ${P4RUNTIME_GRPC_GEN_SRCS})
target_link_libraries(flay_grpc_client ${FLAY_GRPC_CLIENT_LIBS})

set(CMAKE_POSITION_INDEPENDENT_CODE ${CMAKE_POSITION_INDEPENDENT_CODE_PREV})
