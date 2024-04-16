message(STATUS "Submodule update")
execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                RESULT_VARIABLE GIT_SUBMOD_RESULT)
if(NOT GIT_SUBMOD_RESULT EQUAL "0")
    message(FATAL_ERROR "git submodule update --init --recursive failed with ${GIT_SUBMOD_RESULT}, please checkout submodules")
endif()

# ##################################################################################################
# GITHUB REPOSITORIES
# ##################################################################################################

set (FETCHCONTENT_BASE_DIR_PREV ${FETCHCONTENT_BASE_DIR})
set (FETCHCONTENT_BASE_DIR ${P4C_BINARY_DIR}/tofino_research_examples)

# Effectively emulates fetchcontent_makeavailable but does not add the module to install.
macro(fetchcontent_download_without_build content)
  FetchContent_GetProperties(${content})
  if(NOT ${content}_POPULATED)
    FetchContent_Populate(${content})
  endif()
endmacro(fetchcontent_download_without_build)

FetchContent_Declare(
  switcharoo
  GIT_REPOSITORY https://github.com/Switcharoo-P4/Switcharoo-P4
  GIT_TAG main
)
# FetchContent_Declare(
#   scion
#   GIT_REPOSITORY https://github.com/netsys-lab/scion-p4
#   GIT_TAG main
# )
FetchContent_Declare(
  p4sta
  GIT_REPOSITORY https://github.com/ralfkundel/P4STA/
  GIT_TAG master
)
FetchContent_Declare(
  p4_projects
  GIT_REPOSITORY https://github.com/Princeton-Cabernet/p4-projects
  GIT_TAG master
)
FetchContent_Declare(
  p4_hh
  GIT_REPOSITORY https://github.com/intrig-unicamp/P4-HH
  GIT_TAG master
)
FetchContent_Declare(
  flowrest
  GIT_REPOSITORY https://github.com/nds-group/Flowrest
  GIT_TAG main
)
FetchContent_Declare(
  dta
  GIT_REPOSITORY https://github.com/jonlanglet/DTA
  GIT_TAG main
)
FetchContent_Declare(
  beaucoup
  GIT_REPOSITORY https://github.com/Princeton-Cabernet/BeauCoup
  GIT_TAG master
)
FetchContent_Declare(
  acc_turbo
  GIT_REPOSITORY https://github.com/nsg-ethz/ACC-Turbo
  GIT_TAG main
)

fetchcontent_download_without_build(switcharoo)
# fetchcontent_download_without_build(scion)
fetchcontent_download_without_build(p4sta)
fetchcontent_download_without_build(p4_projects)
fetchcontent_download_without_build(p4_hh)
fetchcontent_download_without_build(flowrest)
fetchcontent_download_without_build(dta)
fetchcontent_download_without_build(beaucoup)
fetchcontent_download_without_build(acc_turbo)

set(FETCHCONTENT_BASE_DIR ${FETCHCONTENT_BASE_DIR_PREV})

# ##################################################################################################
# TEST PROGRAMS
# ##################################################################################################

set(TNA_RESEARCH_TESTS
  ${switcharoo_SOURCE_DIR}/switcharoo/p4src/switcharoo.p4
  # ${scion_SOURCE_DIR}/tofino-scion-br/p4src/scion.p4
  # ${scion_SOURCE_DIR}/tofino-pktgen/p4src/pktgen.p4
  # ${scion_SOURCE_DIR}/tofino-epic-br/p4src/scion.p4
  # ${scion_SOURCE_DIR}/tofino-crypto/aes_1pipe/p4src/aes_1pipe.p4
  # ${scion_SOURCE_DIR}/tofino-crypto/cmac_1pipe/p4src/cmac_1pipe.p4
  ${p4sta_SOURCE_DIR}/stamper_targets/Wedge100B65/PLEASE_COPY/tofino_stamper_v1_2_0.p4
  ${p4_projects_SOURCE_DIR}/RTT-tofino/p4src/RTT.p4
  ${p4_projects_SOURCE_DIR}/PRECISION-tofino/p4src/PRECISION.p4
  ${p4_projects_SOURCE_DIR}/P40f-tofino/p40f_tofino.p4
  ${p4_projects_SOURCE_DIR}/SmartCookie/p4src/SmartCookie-HalfSipHash.p4
  ${p4_projects_SOURCE_DIR}/SipHash-tofino/p4src/halfsiphash24_ingressegress.p4
  ${p4_projects_SOURCE_DIR}/Meta4-tofino/P4/netassay_iot_j6.p4
  ${p4_projects_SOURCE_DIR}/Meta4-tofino/P4/netassay_tunnel_j7.p4
  ${p4_projects_SOURCE_DIR}/Meta4-tofino/P4/netassay_tunnel_j8.p4
  ${p4_projects_SOURCE_DIR}/Meta4-tofino/P4/netassay_v4_j6.p4
  ${p4_projects_SOURCE_DIR}/Fridge-tofino/p4src/UnbiasedRtt.p4
  ${p4_projects_SOURCE_DIR}/ConQuest-tofino/p4src/baseline.p4
  ${p4_projects_SOURCE_DIR}/AES-tofino/p4src/aes_oneround.p4
  ${p4_projects_SOURCE_DIR}/AES-tofino/p4src/aes_tworound.p4
  ${p4_hh_SOURCE_DIR}/P4-IPG-HH/IPG-HH.p4
  ${flowrest_SOURCE_DIR}/P4/Full_version/unibs_flowrest.p4
  ${dta_SOURCE_DIR}/Translator/p4src/dta_translator.p4
  ${acc_turbo_SOURCE_DIR}/tofino/p4src/accturbo.p4
  ${acc_turbo_SOURCE_DIR}/tofino/p4src/ddos_aid_4x4_singlepipe_p4_16_modified.p4
  ${acc_turbo_SOURCE_DIR}/tofino/p4src/heavy_hitter_5tupple.p4
  ${acc_turbo_SOURCE_DIR}/tofino/p4src/heavy_hitter_reaction.p4
  ${acc_turbo_SOURCE_DIR}/tofino/p4src/heavy_hitter_srcbased.p4
  ${acc_turbo_SOURCE_DIR}/tofino/p4src/simple_forwarder.p4
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
  # scion.p4
  # aes_1pipe.p4
  # cmac_1pipe.p4
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
