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
  PATCH_COMMAND git apply ${CMAKE_CURRENT_SOURCE_DIR}/patches/switcharoo.patch || git apply ${CMAKE_CURRENT_SOURCE_DIR}/patches/switcharoo.patch  -R --check && echo "Patch does not apply because the patch was already applied."
  GIT_TAG main
)
FetchContent_Declare(
  scion
  GIT_REPOSITORY https://github.com/netsys-lab/scion-p4
  GIT_TAG main
)
FetchContent_Declare(
  p4sta
  GIT_REPOSITORY https://github.com/ralfkundel/P4STA/
  GIT_TAG master
)
FetchContent_Declare(
  p4_projects
  GIT_REPOSITORY https://github.com/Princeton-Cabernet/p4-projects
  GIT_TAG master
  PATCH_COMMAND git apply ${CMAKE_CURRENT_SOURCE_DIR}/patches/p4_projects.patch || git apply ${CMAKE_CURRENT_SOURCE_DIR}/patches/p4_projects.patch  -R --check && echo "Patch does not apply because the patch was already applied."
  # TODO: here is a temporal additional patch for P40f_tofino.p4 that reduces the program size and decreases the time processing the program.
  COMMAND git apply ${CMAKE_CURRENT_SOURCE_DIR}/patches/P40f_tofino.patch || git apply ${CMAKE_CURRENT_SOURCE_DIR}/patches/P40f_tofino.patch  -R --check && echo "Patch does not apply because the patch was already applied."
)
FetchContent_Declare(
  p4_hh
  GIT_REPOSITORY https://github.com/intrig-unicamp/P4-HH
  GIT_TAG master
  PATCH_COMMAND git apply ${CMAKE_CURRENT_SOURCE_DIR}/patches/p4_hh.patch  || git apply ${CMAKE_CURRENT_SOURCE_DIR}/patches/p4_hh.patch  -R --check && echo "Patch does not apply because the patch was already applied."
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
fetchcontent_download_without_build(scion)
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
  ${switcharoo_SOURCE_DIR}/p4src/switcharoo.p4
  # ${scion_SOURCE_DIR}/tofino-scion-br/p4src/scion.p4
  ${scion_SOURCE_DIR}/tofino-pktgen/p4src/pktgen.p4
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
  switcharoo.p4 #  Switch<cuckoo_ingress_headers_t, cuckoo_ingress_metadata_t, cuckoo_egress_headers_t, cuckoo_egress_metadata_t, bloom_ingress_headers_t, bloom_ingress_metadata_t, bloom_egress_headers_t, bloom_egress_metadata_t, _, _, _, _, _, _, _, _> main(cuckoo, bloom): The Tofno1 architecture requires 6 pipes. Received 12.
  # scion.p4 # Timeout
  pktgen.p4 # fatal error: t2na.p4: No such file or directory
  # aes_1pipe.p4
  # cmac_1pipe.p4
  RTT.p4 # Node @name("SwitchIngress.in_value") paired_32bit in_value_0/in_value of type Declaration_Variable not implemented in the core stepper.
  PRECISION.p4 # Unimplemented extern method: resubmit.emit
  netassay_iot_j6.p4 # Parser state parse_dns_answer; was already visited. We currently do not support parser loops.
  netassay_tunnel_j7.p4 # Parser state parse_dns_answer; was already visited. We currently do not support parser loops.
  netassay_tunnel_j8.p4 # Parser state parse_dns_answer; was already visited. We currently do not support parser loops.
  netassay_v4_j6.p4 # Parser state parse_dns_answer; was already visited. We currently do not support parser loops.
  UnbiasedRtt.p4 #error "Please specify the size of fridge's underlying registers by defining REG_SIZE."
  IPG-HH.p4 # Node @name("SwitchIngress.tmp_1") bit<16> tmp_3 of type Declaration_Variable not implemented in the core stepper.
  dta_translator.p4 # Unimplemented extern method: mirror.emit
  accturbo.p4 # Unimplemented extern method: do_resubmit.emit
  ddos_aid_4x4_singlepipe_p4_16_modified.p4 # Unimplemented extern method: do_resubmit.emit
)
