#include "backends/p4tools/modules/flay/service/client_options.h"

#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

#include "backends/p4tools/common/lib/logging.h"
#include "lib/exceptions.h"

namespace P4Tools::Flay {

FlayClientOptions::FlayClientOptions(std::string_view message) : Options(message) {
    // Register some common options.
    registerOption(
        "--help", nullptr,
        [this](const char *) {
            usage();
            exit(0);
            return false;
        },
        "Shows this help message and exits");

    registerOption(
        "--version", nullptr,
        [/*this*/](const char *) {
            P4C_UNIMPLEMENTED("Version is currently not supported.\n");
            exit(0);
            return false;
        },
        "Prints version information and exits");
    registerOption(
        "--print-performance-report", nullptr,
        [](const char *) {
            enablePerformanceLogging();
            return true;
        },
        "Print timing report summary at the end of the program.");
    registerOption(
        "--server-address", "serverAddress",
        [this](const char *arg) {
            serverAddress = arg;
            return true;
        },
        "The address of the Flay service in the format ADDRESS:PORT.");
    registerOption(
        "-u", "proto_update",
        [this](const char *arg) {
            auto protoUpdate = std::filesystem::path(arg);
            if (!std::filesystem::exists(protoUpdate)) {
                ::error("%1% does not exist. Please provide a valid file path.",
                        protoUpdate.c_str());
                return false;
            }
            protoUpdates.emplace_back(protoUpdate);
            return true;
        },
        "Specify a proto update file to send to the server. Multiple entries are possible.");
}

const char *FlayClientOptions::getIncludePath() {
    P4C_UNIMPLEMENTED("getIncludePath not implemented for FlayClient.");
}

// Process options; return list of remaining options.
// Returns 'nullptr' if an error is signalled
std::vector<const char *> *FlayClientOptions::process(int argc, char *const argv[]) {
    return Util::Options::process(argc, argv);
}

std::string FlayClientOptions::getServerAddress() const { return serverAddress; }

std::vector<std::filesystem::path> FlayClientOptions::getProtoUpdates() const {
    return protoUpdates;
}

}  // namespace P4Tools::Flay
