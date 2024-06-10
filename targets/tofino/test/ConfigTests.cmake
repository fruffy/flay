set (CONFIG_EXTRA_OPTS "-I${CMAKE_CURRENT_LIST_DIR}/p4include -I${CMAKE_CURRENT_LIST_DIR}/programs/opentofino")
set (CONFIG_EXTRA_OPTS "${CONFIG_EXTRA_OPTS} --reference-folder ${CMAKE_CURRENT_LIST_DIR}/testdata/config --control-plane BFRUNTIME"
)

p4tools_add_test_with_args(
  P4TEST "${CMAKE_CURRENT_LIST_DIR}/programs/common/tna_simple_action_profile.p4"
  TAG "flay-tofino1-tna-config" ALIAS "tna_simple_action_profile.p4" DRIVER ${FLAY_REFERENCE_DRIVER}
  TARGET "tofino1" ARCH "tna" CONTROL_PLANE_UPDATES "${CMAKE_CURRENT_LIST_DIR}/protos/tna_simple_action_profile/update*.txtpb" TEST_ARGS "-D__TARGET_TOFINO__=1 ${CONFIG_EXTRA_OPTS}"
)
p4tools_add_test_with_args(
  P4TEST "${CMAKE_CURRENT_LIST_DIR}/programs/common/tna_simple_action_selector.p4"
  TAG "flay-tofino1-tna-config" ALIAS "tna_simple_action_selector.p4" DRIVER ${FLAY_REFERENCE_DRIVER}
  TARGET "tofino1" ARCH "tna" CONTROL_PLANE_UPDATES "${CMAKE_CURRENT_LIST_DIR}/protos/tna_simple_action_selector/update*.txtpb" TEST_ARGS "-D__TARGET_TOFINO__=1 ${CONFIG_EXTRA_OPTS}"
)
