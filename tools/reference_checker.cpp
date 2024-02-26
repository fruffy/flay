#include <cstdlib>
#include <fstream>
#include <sstream>

#include "backends/p4tools/common/lib/logging.h"
#include "backends/p4tools/modules/flay/control_plane/util.h"
#include "backends/p4tools/modules/flay/flay.h"
#include "backends/p4tools/modules/flay/register.h"
#include "frontends/common/options.h"
#include "lib/error.h"
#include "lib/options.h"

namespace P4Tools::Flay {

class ReferenceCheckerOptions : protected FlayOptions {
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

    /// The arguments to pass to the Flay compiler.
    CompilerOptions compilerOptions;

    /// @returns the arguments to pass to the compiler. A little hacky because of an API mismatch.
    [[nodiscard]] std::vector<char *> getCompilerArgs(char *binaryName) const {
        std::vector<char *> args;
        args.reserve(compilerArgs.size() + 1);
        args.push_back(binaryName);
        // Compiler expects path to executable as first element in argument list.
        for (const auto &arg : compilerArgs) {
            args.push_back(
                const_cast<char *>(arg));  // NOLINT (cppcoreguidelines-pro-type-const-cast)
        }
        return args;
    }

 public:
    ReferenceCheckerOptions() : FlayOptions("Checks Flay output against references.") {
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
            "--enable-info-logging", nullptr,
            [](const char *) {
                enableInformationLogging();
                return true;
            },
            "Print verbose messages.");
    }
    ~ReferenceCheckerOptions() override = default;

    const char *getIncludePath() override {
        P4C_UNIMPLEMENTED("getIncludePath not implemented for FlayClient.");
    }

    // Process options; return list of remaining options.
    // Returns EXIT_FAILURE if an error occurred.
    int processOptions(int argc, char *const argv[]) {
        auto *unprocessedOptions = P4Tools::FlayOptions::process(argc, argv);
        if (unprocessedOptions != nullptr && !unprocessedOptions->empty()) {
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
        // If the environment variable P4TEST_REPLACE is set, overwrite the reference file.
        if (std::getenv("P4TEST_REPLACE") != nullptr) {
            overwriteReferences = true;
        }
        // If the environment variable P4FLAY_INFO is set, enable information logging.
        if (std::getenv("P4FLAY_INFO") != nullptr) {
            enableInformationLogging();
        }
        auto compilerString = getCompilerArgs(argv[0]);
        unprocessedOptions =
            compilerOptions.process(static_cast<int>(compilerString.size()), compilerString.data());
        if (unprocessedOptions != nullptr && !unprocessedOptions->empty()) {
            for (const auto &option : *unprocessedOptions) {
                ::error("Unprocessed compiler input: %s", option);
            }
            return EXIT_FAILURE;
        }
        // compilerOptions.target = options.getTarget().c_str();
        // compilerOptions.arch = options.getArch().c_str();
        compilerOptions.file = getInputFile().c_str();
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

    [[nodiscard]] const FlayOptions &toFlayOptions() const { return *this; }

    [[nodiscard]] const CompilerOptions &getCompilerOptions() const { return compilerOptions; }
};

/// Compare the output of Flay with the reference file.
/// Fails if any differences are found and reports the differences.
int compareAgainstReference(const std::stringstream &flayOptimizationOutput,
                            const std::filesystem::path &referenceFile) {
    // Construct the command to invoke the diff utility
    std::stringstream command;
    command << "echo " << std::quoted(flayOptimizationOutput.str());
    command << "| diff --color -u " << referenceFile.c_str() << " -";

    // Open a pipe to capture the output of the diff command.
    printInfo("Running diff command: \"%s\"", command.str());
    FILE *pipe = popen(command.str().c_str(), "r");
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
        ::error("Diff command failed.\n%1%", result.str());
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int run(const ReferenceCheckerOptions &options) {
    ASSIGN_OR_RETURN(auto flayServiceStatistics,
                     Flay::optimizeProgram(options.getCompilerOptions(), options.toFlayOptions()),
                     EXIT_FAILURE);
    std::stringstream flayOptimizationOutput;
    for (const auto &elimRepl : flayServiceStatistics.eliminatedNodes) {
        auto [node, replaced] = elimRepl;
        if (node->getSourceInfo().isValid()) {
            auto sourceFragment = node->getSourceInfo().toSourceFragment(false).trim();
            if (replaced == nullptr) {
                flayOptimizationOutput << "Eliminated node at line "
                                       << node->getSourceInfo().toPosition().sourceLine << ": "
                                       << sourceFragment;
            } else {
                RETURN_IF_FALSE(replaced->getSourceInfo().isValid(), EXIT_FAILURE);
                auto replacedFragment = replaced->getSourceInfo().toSourceFragment(false).trim();
                flayOptimizationOutput << "Replaced node at line "
                                       << node->getSourceInfo().toPosition().sourceLine << ": "
                                       << sourceFragment << " with " << replacedFragment;
            }
        } else {
            ::warning("Invalid source information for node %1%. This should be fixed", node);
            flayOptimizationOutput << "Eliminated node at line (unknown): " << node;
        }
        if (elimRepl != flayServiceStatistics.eliminatedNodes.back()) {
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
        return EXIT_SUCCESS;
    }
    const auto &referenceFileOpt = options.getReferenceFile();
    if (referenceFileOpt.has_value()) {
        auto referenceFile = std::filesystem::absolute(referenceFileOpt.value());
        return compareAgainstReference(flayOptimizationOutput, referenceFile);
    }
    if (options.getReferenceFolder().has_value()) {
        auto referenceFolder = std::filesystem::absolute(options.getReferenceFolder().value());
        auto referenceName = options.getInputFile().stem();
        for (const auto &entry : std::filesystem::directory_iterator(referenceFolder)) {
            const auto &referenceFile = entry.path();

            if (referenceFile.extension() == ".ref" && referenceFile.stem() == referenceName) {
                return compareAgainstReference(flayOptimizationOutput, referenceFile);
            }
        }
        ::error("Reference file not found in folder.");
        return EXIT_FAILURE;
    }
    ::error("Neither a reference file nor a reference folder have been specified.");
    return EXIT_FAILURE;
}

}  // namespace P4Tools::Flay

class FlayClientContext : public BaseCompileContext {};

int main(int argc, char *argv[]) {
    P4Tools::Flay::registerFlayTargets();
    P4Tools::Flay::registerCompilerTargets();

    // Set up the compilation context and the options.
    AutoCompileContext autoP4FlayClientContext(new FlayClientContext);
    P4Tools::Flay::ReferenceCheckerOptions options;

    // Process command-line options.
    if (options.processOptions(argc, argv) == EXIT_FAILURE || ::errorCount() != 0) {
        return EXIT_FAILURE;
    }

    // Run the reference checker.
    auto result = P4Tools::Flay::run(options);
    if (result == EXIT_FAILURE) {
        return EXIT_FAILURE;
    }
    return ::errorCount() == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
