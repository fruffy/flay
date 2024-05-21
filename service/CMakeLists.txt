# Anything linking with gRPC requires position independent code.
set(CMAKE_POSITION_INDEPENDENT_CODE_PREV ${CMAKE_POSITION_INDEPENDENT_CODE_PREV})
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_library(flay_grpc OBJECT ${P4RUNTIME_RT_PROTO})
target_link_libraries(flay_grpc PUBLIC grpc++ PRIVATE controlplane PRIVATE flay_p4runtime)

protobuf_generate(
  TARGET flay_grpc
  LANGUAGE grpc
  IMPORT_DIRS ${P4RUNTIME_STD_DIR} ${FLAY_P4RUNTIME_DIR} ${Protobuf_INCLUDE_DIRS} ${P4C_SOURCE_DIR}/control-plane
  PROTOC_OUT_DIR ${CMAKE_BINARY_DIR}/control-plane
  OUT_VAR FLAY_GRPC_P4RUNTIME_GEN_SRCS
  DEPENDS ${P4RUNTIME_RT_PROTO} controlplane
  PLUGIN protoc-gen-grpc=$<TARGET_FILE:grpc_cpp_plugin>
  GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc
)

add_library(flay_grpc_service STATIC flay_grpc_service.cpp)
target_link_libraries(flay_grpc_service PRIVATE grpc++ PRIVATE controlplane PRIVATE ${LIBGC_LIBRARIES} PRIVATE flay_grpc)

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
    PRIVATE
    flay_grpc
)
if(HAVE_LIBBACKTRACE)
  set(FLAY_GRPC_CLIENT_LIBS "${FLAY_GRPC_CLIENT_LIBS};-lbacktrace")
endif()

add_executable(flay_grpc_client ${FLAY_GRPC_CLIENT_SOURCES} ${P4RUNTIME_GRPC_GEN_SRCS})
target_link_libraries(flay_grpc_client ${FLAY_GRPC_CLIENT_LIBS})

set(CMAKE_POSITION_INDEPENDENT_CODE ${CMAKE_POSITION_INDEPENDENT_CODE_PREV})
