set (CONFIG_EXTRA_OPTS "--reference-folder ${CMAKE_CURRENT_LIST_DIR}/testdata/config")

p4tools_add_test_with_args(
  P4TEST "${CMAKE_CURRENT_LIST_DIR}/programs/v1model_dead_ternary_table.p4"
  TAG "flay-bmv2-v1model-config" ALIAS "v1model_dead_ternary_table.p4" DRIVER ${FLAY_REFERENCE_DRIVER}
  TARGET "bmv2" ARCH "v1model" CONTROL_PLANE_UPDATES "${CMAKE_CURRENT_LIST_DIR}/protos/v1model_dead_ternary_table/update*.txtpb" TEST_ARGS "-I${P4C_BINARY_DIR}/p4include ${CONFIG_EXTRA_OPTS}"
)

p4tools_add_test_with_args(
  P4TEST "${P4C_SOURCE_DIR}/testdata/p4_16_samples/basic_routing-bmv2.p4"
  TAG "flay-bmv2-v1model-config" ALIAS "basic_routing-bmv2.p4" DRIVER ${FLAY_REFERENCE_DRIVER}
  TARGET "bmv2" ARCH "v1model" CONTROL_PLANE_UPDATES "${CMAKE_CURRENT_LIST_DIR}/protos/basic_routing/update*.txtpb" TEST_ARGS "-I${P4C_BINARY_DIR}/p4include ${CONFIG_EXTRA_OPTS}"
)

p4tools_add_test_with_args(
  P4TEST "${P4C_SOURCE_DIR}/testdata/p4_16_samples/pins/pins_middleblock.p4"
  TAG "flay-bmv2-v1model-config" ALIAS "pins_middleblock.p4" DRIVER ${FLAY_REFERENCE_DRIVER}
  TARGET "bmv2" ARCH "v1model" CONTROL_PLANE_UPDATES "${CMAKE_CURRENT_LIST_DIR}/protos/pins_middleblock/update*.txtpb" TEST_ARGS "-I${P4C_BINARY_DIR}/p4include ${CONFIG_EXTRA_OPTS}"
)

p4tools_add_test_with_args(
  P4TEST "${P4C_SOURCE_DIR}/testdata/p4_16_samples/dash/dash-pipeline-v1model-bmv2.p4"
  TAG "flay-bmv2-v1model-config" ALIAS "dash-pipeline-v1model-bmv2.p4" DRIVER ${FLAY_REFERENCE_DRIVER}
  TARGET "bmv2" ARCH "v1model" CONTROL_PLANE_UPDATES "${CMAKE_CURRENT_LIST_DIR}/protos/dash-pipeline-v1model-bmv2/update*.txtpb" TEST_ARGS "-I${P4C_BINARY_DIR}/p4include ${CONFIG_EXTRA_OPTS}"
)

p4tools_add_test_with_args(
  P4TEST "${CMAKE_CURRENT_LIST_DIR}/programs/v1model_entries_table.p4"
  TAG "flay-bmv2-v1model-config" ALIAS "v1model_entries_table.p4" DRIVER ${FLAY_REFERENCE_DRIVER}
  TARGET "bmv2" ARCH "v1model" CONTROL_PLANE_UPDATES "${CMAKE_CURRENT_LIST_DIR}/protos/v1model_entries_table/update*.txtpb" TEST_ARGS "-I${P4C_BINARY_DIR}/p4include ${CONFIG_EXTRA_OPTS}"
)
