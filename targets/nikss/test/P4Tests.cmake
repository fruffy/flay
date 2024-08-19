# Fetch the bpftool.
set(FETCHCONTENT_QUIET_PREV ${FETCHCONTENT_QUIET})
set(FETCHCONTENT_QUIET OFF)
FetchContent_Declare(
  bpftool
  URL https://github.com/libbpf/bpftool/releases/download/v7.4.0/bpftool-v7.4.0-amd64.tar.gz
  URL_HASH
    SHA256=68862a7038a269e1d4af5fed53d2b4d763f1cd82a96caa49b76596453bae0c3e
  SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/test/bpftool USES_TERMINAL_DOWNLOAD TRUE
  GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(bpftool)
execute_process(COMMAND "chmod 775 ${CMAKE_CURRENT_SOURCE_DIR}/test/bpftool/bpftool")

FetchContent_Declare(trex
  URL https://trex-tgn.cisco.com/trex/release/v3.05.tar.gz
  URL_HASH
    SHA256=15e9ac6da554b9bdbec575da5f1010e3d2b07c8806be889e074ae8a776636c0b
  SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/test/trex
  PATCH_COMMAND
    patch -p0 -i ${CMAKE_CURRENT_SOURCE_DIR}/test/trex.patch || echo
    "Patch does not apply because the patch was already applied."
  GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(trex)

set(FETCHCONTENT_QUIET ${FETCHCONTENT_QUIET_PREV})
