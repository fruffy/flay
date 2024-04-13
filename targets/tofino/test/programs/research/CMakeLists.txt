message(STATUS "Submodule update")
execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                RESULT_VARIABLE GIT_SUBMOD_RESULT)
if(NOT GIT_SUBMOD_RESULT EQUAL "0")
    message(FATAL_ERROR "git submodule update --init --recursive failed with ${GIT_SUBMOD_RESULT}, please checkout submodules")
endif()


# ##################################################################################################
# TEST PROGRAMS
# ##################################################################################################

set(TNA_RESEARCH_TESTS
    ${CMAKE_CURRENT_SOURCE_DIR}/switcharoo/p4src/switcharoo.p4
    ${CMAKE_CURRENT_SOURCE_DIR}/scion/tofino-scion-br/p4src/scion.p4
    # ${CMAKE_CURRENT_SOURCE_DIR}/scion/tofino-pktgen/p4src/pktgen.p4
    # ${CMAKE_CURRENT_SOURCE_DIR}/scion/tofino-epic-br/p4src/scion.p4
    ${CMAKE_CURRENT_SOURCE_DIR}/scion/tofino-crypto/aes_1pipe/p4src/aes_1pipe.p4
    ${CMAKE_CURRENT_SOURCE_DIR}/scion/tofino-crypto/cmac_1pipe/p4src/cmac_1pipe.p4
    ${CMAKE_CURRENT_SOURCE_DIR}/p4sta/stamper_targets/Wedge100B65/PLEASE_COPY/tofino_stamper_v1_2_0.p4
    ${CMAKE_CURRENT_SOURCE_DIR}/p4_projects/RTT-tofino/p4src/RTT.p4
    ${CMAKE_CURRENT_SOURCE_DIR}/p4_projects/PRECISION-tofino/p4src/PRECISION.p4
    ${CMAKE_CURRENT_SOURCE_DIR}/p4_projects/P40f-tofino/p40f_tofino.p4
    ${CMAKE_CURRENT_SOURCE_DIR}/p4_projects/SmartCookie/p4src/SmartCookie-HalfSipHash.p4
    ${CMAKE_CURRENT_SOURCE_DIR}/p4_projects/SipHash-tofino/p4src/halfsiphash24_ingressegress.p4
    ${CMAKE_CURRENT_SOURCE_DIR}/p4_projects/Meta4-tofino/P4/netassay_iot_j6.p4
    ${CMAKE_CURRENT_SOURCE_DIR}/p4_projects/Meta4-tofino/P4/netassay_tunnel_j7.p4
    ${CMAKE_CURRENT_SOURCE_DIR}/p4_projects/Meta4-tofino/P4/netassay_tunnel_j8.p4
    ${CMAKE_CURRENT_SOURCE_DIR}/p4_projects/Meta4-tofino/P4/netassay_v4_j6.p4
    ${CMAKE_CURRENT_SOURCE_DIR}/p4_projects/Fridge-tofino/p4src/UnbiasedRtt.p4
    ${CMAKE_CURRENT_SOURCE_DIR}/p4_projects/ConQuest-tofino/p4src/baseline.p4
    ${CMAKE_CURRENT_SOURCE_DIR}/p4_projects/AES-tofino/p4src/aes_oneround.p4
    ${CMAKE_CURRENT_SOURCE_DIR}/p4_projects/AES-tofino/p4src/aes_tworound.p4
    ${CMAKE_CURRENT_SOURCE_DIR}/p4_hh/P4-IPG-HH/IPG-HH.p4
    ${CMAKE_CURRENT_SOURCE_DIR}/flowrest/P4/Full_version/unibs_flowrest.p4
    ${CMAKE_CURRENT_SOURCE_DIR}/dta/Translator/p4src/dta_translator.p4
    ${CMAKE_CURRENT_SOURCE_DIR}/acc_turbo/tofino/p4src/accturbo.p4
    ${CMAKE_CURRENT_SOURCE_DIR}/acc_turbo/tofino/p4src/ddos_aid_4x4_singlepipe_p4_16_modified.p4
    ${CMAKE_CURRENT_SOURCE_DIR}/acc_turbo/tofino/p4src/heavy_hitter_5tupple.p4
    ${CMAKE_CURRENT_SOURCE_DIR}/acc_turbo/tofino/p4src/heavy_hitter_reaction.p4
    ${CMAKE_CURRENT_SOURCE_DIR}/acc_turbo/tofino/p4src/heavy_hitter_srcbased.p4
    ${CMAKE_CURRENT_SOURCE_DIR}/acc_turbo/tofino/p4src/simple_forwarder.p4
)


set (EXTRA_OPTS "-I${CMAKE_CURRENT_LIST_DIR}/../../p4include")
set (EXTRA_OPTS "${EXTRA_OPTS} --reference-folder ${CMAKE_CURRENT_LIST_DIR}/../../testdata")

p4tools_add_tests(
  TESTS
  "${TNA_RESEARCH_TESTS}"
  TAG
  "flay-tofino1-research-tna"
  DRIVER
  ${FLAY_REFERENCE_DRIVER}
  TARGET
  "tofino1"
  ARCH
  "tna"
  TEST_ARGS
  "${EXTRA_OPTS} -D__TARGET_TOFINO__=1"
)


# Wildcard failing tests for now. TODO:  Classify further.
p4tools_add_xfail_reason(
  "flay-tofino1-research-tna"
  ".*"
  switcharoo.p4
  scion.p4
  aes_1pipe.p4
  cmac_1pipe.p4
  RTT.p4
  PRECISION.p4
  p40f_tofino.p4
  SmartCookie-HalfSipHash.p4
  netassay_iot_j6.p4
  netassay_tunnel_j7.p4
  netassay_tunnel_j8.p4
  netassay_v4_j6.p4
  UnbiasedRtt.p4
  IPG-HH.p4
  unibs_flowrest.p4
  dta_translator.p4
  accturbo.p4
  ddos_aid_4x4_singlepipe_p4_16_modified.p4
  heavy_hitter_5tupple.p4
  heavy_hitter_reaction.p4
  heavy_hitter_srcbased.p4
  simple_forwarder.p4
)
