#include <cstdlib>
#include <fstream>
#include <sstream>

#include "backends/p4tools/common/lib/logging.h"
#include "backends/p4tools/modules/flay/control_plane/util.h"
#include "backends/p4tools/modules/flay/flay.h"
#include "frontends/common/options.h"
#include "lib/error.h"
#include "lib/options.h"

namespace P4Tools::Flay {

class ReferenceCheckerOptions : protected Util::Options {
    /// The input file to process.
    std::optional<std::filesystem::path> inputFile;

    /// The reference file to compare against.
    std::optional<std::filesystem::path> referenceFile;

    /// Look for the reference file in this folder.
    /// The reference file stem should be named the same as the input file stem.
    std::optional<std::filesystem::path> referenceFolder;

    /// Overwrite the reference file and do not compare against it.
    bool overwriteReferences = false;

    /// The target to compile for.
    std::string target;

    /// The architecture to compile for.
    std::string arch;

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
        registerOption(
            "--target", "target",
            [this](const char *arg) {
                target = arg;
                return true;
            },
            "Compile for the specified target device.");
        registerOption(
            "--arch", "arch",
            [this](const char *arg) {
                arch = arg;
                return true;
            },
            "Compile for the specified architecture.");
        registerOption(
            "--reference-file", "referenceFile",
            [this](const char *arg) {
                referenceFile = arg;
                return true;
            },
            "The reference file to compare against.");
        registerOption(
            "--reference-folder", "referenceFolder",
            [this](const char *arg) {
                referenceFolder = arg;
                return true;
            },
            "Try to find the reference file in the folder. The reference file stem should be named "
            "the same as the input file stem.");
        registerOption(
            "-v", nullptr,
            [](const char *) {
                enableInformationLogging();
                return true;
            },
            "Print verbose messages.");
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
        if (!referenceFile.has_value() && !referenceFolder.has_value()) {
            ::error(
                "Neither a reference file nor a reference folder have been specified. Use either "
                "--reference-file or --reference-folder.");
            return EXIT_FAILURE;
        }
        if (referenceFile.has_value() && referenceFolder.has_value()) {
            ::error(
                "Both a reference file and a reference folder have been specified. Use only one.");
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    }

    [[nodiscard]] const std::filesystem::path &getInputFile() const { return inputFile.value(); }

    [[nodiscard]] std::optional<std::filesystem::path> getReferenceFile() const {
        return referenceFile;
    }

    [[nodiscard]] std::optional<std::filesystem::path> getReferenceFolder() const {
        return referenceFolder;
    }

    [[nodiscard]] bool doOverwriteReferences() const { return overwriteReferences; }

    [[nodiscard]] const std::string &getTarget() const { return target; }

    [[nodiscard]] const std::string &getArch() const { return arch; }
};

int compareAgainstReference(const std::stringstream &flayOptimizationOutput,
                            const std::filesystem::path &referenceFile) {
    // Construct the command to invoke the diff utility
    std::string command = "echo \"";
    command += flayOptimizationOutput.str();
    command += "\"| diff --color -u ";
    command += referenceFile.c_str();
    command += " -";

    // Open a pipe to capture the output of the diff command.
    printInfo("Running diff command: \"%s\"", command.c_str());
    FILE *pipe = popen(command.c_str(), "r");
    if (pipe == nullptr) {
        ::error("Unable to create pipe to diff command.");
        return EXIT_FAILURE;
    }
    // Read and print the output of the diff command.
    std::stringstream result;
    char buffer[128];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result << buffer;
    }
    if (pclose(pipe) != 0) {
        ::error("Error: Diff command failed.\n%1%", result.str());
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int run(const ReferenceCheckerOptions &options) {
    auto compilerOptions = CompilerOptions();
    compilerOptions.target = options.getTarget().c_str();
    compilerOptions.arch = options.getArch().c_str();
    compilerOptions.file = options.getInputFile().c_str();

    ASSIGN_OR_RETURN(auto flayServiceStatistics,
                     Flay::optimizeProgram(compilerOptions, FlayOptions::get()), EXIT_FAILURE);
    std::stringstream flayOptimizationOutput;
    for (const auto *node : flayServiceStatistics.eliminatedNodes) {
        flayOptimizationOutput << "Eliminated node: " << node;
        if (node != flayServiceStatistics.eliminatedNodes.back()) {
            flayOptimizationOutput << "\n";
        }
    }

    if (options.doOverwriteReferences()) {
        /// Use the stem of the input file name as test name.
        auto referencePath = options.getInputFile().stem();
        auto referenceFolderOpt = options.getReferenceFolder();
        if (referenceFolderOpt.has_value()) {
            auto referenceFolder = referenceFolderOpt.value();
            try {
                std::filesystem::create_directories(referenceFolder);
            } catch (const std::exception &err) {
                ::error("Unable to create directory %1%: %2%", referenceFolder.c_str(), err.what());
                return EXIT_FAILURE;
            }
            referencePath = referenceFolder / referencePath;
        }
        referencePath = referencePath.replace_extension(".ref");
        std::ofstream ofs(referencePath);
        ofs << flayOptimizationOutput << std::endl;
        ofs.close();
    } else {
        const auto &referenceFileOpt = options.getReferenceFile();
        if (referenceFileOpt.has_value()) {
            auto referenceFile = std::filesystem::absolute(referenceFileOpt.value());
            return compareAgainstReference(flayOptimizationOutput, referenceFile);
        }
    }

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
