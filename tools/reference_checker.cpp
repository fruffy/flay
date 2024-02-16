#include <cstdlib>
#include <sstream>

#include "backends/p4tools/common/lib/logging.h"
#include "backends/p4tools/modules/flay/control_plane/util.h"
#include "backends/p4tools/modules/flay/flay.h"
#include "frontends/common/options.h"
#include "lib/error.h"
#include "lib/options.h"

namespace P4Tools::Flay {

class ReferenceCheckerOptions : protected Util::Options {
    std::optional<std::filesystem::path> inputFile;

    std::optional<std::filesystem::path> referenceFile;

    std::optional<std::filesystem::path> referenceFolder;

    bool overwriteReferences = false;

 public:
    explicit ReferenceCheckerOptions(cstring message) : Options(message) {
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
            "--file", "inputFile",
            [this](const char *arg) {
                inputFile = arg;
                if (!std::filesystem::exists(inputFile.value())) {
                    ::error("The input P4 program '%s' does not exist.", inputFile.value().c_str());
                    return false;
                }
                return true;
            },
            "The input file to process.");
        registerOption(
            "--overwrite", nullptr,
            [this](const char *) {
                overwriteReferences = true;
                return true;
            },
            "Do not check references, instead overwrite the reference.");
    }
    virtual ~ReferenceCheckerOptions() = default;

    const char *getIncludePath() override {
        P4C_UNIMPLEMENTED("getIncludePath not implemented for FlayClient.");
    }

    // Process options; return list of remaining options.
    // Returns 'nullptr' if an error is signaled
    int processOptions(int argc, char *const argv[]) {
        auto *unprocessedOptions = Util::Options::process(argc, argv);
        if (unprocessedOptions != nullptr) {
            for (const auto &option : *unprocessedOptions) {
                ::error("Unprocessed input: %s", option);
            }
            return EXIT_FAILURE;
        }
        if (!inputFile.has_value()) {
            ::error("No input file specified.");
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    }

    [[nodiscard]] const std::filesystem::path &getInputFile() const { return inputFile.value(); }

    [[nodiscard]] const std::optional<std::filesystem::path> &getReferenceFile() const {
        return referenceFile;
    }

    [[nodiscard]] const std::optional<std::filesystem::path> &getReferenceFolder() const {
        return referenceFolder;
    }

    [[nodiscard]] bool doOverwriteReferences() const { return overwriteReferences; }
};

int run(const ReferenceCheckerOptions &options) {
    enableInformationLogging();

    auto compilerOptions = CompilerOptions();
    compilerOptions.target = "bmv2";
    compilerOptions.arch = "v1model";
    compilerOptions.file = options.getInputFile().c_str();
    auto &testgenOptions = FlayOptions::get();

    ASSIGN_OR_RETURN(auto flayServiceStatistics,
                     Flay::optimizeProgram(compilerOptions, testgenOptions), EXIT_FAILURE);
    std::stringstream flayResult;
    for (const auto *node : flayServiceStatistics.eliminatedNodes) {
        flayResult << "Eliminated node: ";
        node->dbprint(flayResult);
        flayResult << "\n";
    }
    printInfo(flayResult.str());

    return EXIT_SUCCESS;
}

}  // namespace P4Tools::Flay

class FlayClientContext : public BaseCompileContext {};

int main(int argc, char *argv[]) {
    // Set up the compilation context and the options.
    AutoCompileContext autoP4FlayClientContext(new FlayClientContext);
    P4Tools::Flay::ReferenceCheckerOptions options("Checks Flay output against references.");

    // Process command-line options.
    if (options.processOptions(argc, argv) == EXIT_FAILURE && ::errorCount() != 0) {
        return EXIT_FAILURE;
    }

    // Run the reference checker.
    auto result = P4Tools::Flay::run(options);
    if (result == EXIT_FAILURE) {
        return EXIT_FAILURE;
    }
    return ::errorCount() == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
