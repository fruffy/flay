#include <cstdlib>
#include <fstream>
#include <optional>
#include <sstream>

#include "backends/p4tools/common/lib/logging.h"
#include "backends/p4tools/modules/flay/core/lib/return_macros.h"
#include "backends/p4tools/modules/flay/flay.h"
#include "backends/p4tools/modules/flay/register.h"
#include "frontends/common/parser_options.h"
#include "lib/compile_context.h"
#include "lib/error.h"
#include "lib/options.h"

namespace P4::P4Tools::Flay {

namespace {

class ReferenceCheckerOptions : public FlayOptions {
    /// The reference file to compare against.
    std::optional<std::filesystem::path> _referenceFile;

    /// Look for the reference file in this folder.
    /// The reference file stem should be named the same as the input file stem.
    std::optional<std::filesystem::path> _referenceFolder;

    /// Overwrite the reference file and do not compare against it.
    bool _overwriteReferences = false;

    /// Write a performance report.
    bool _writePerformanceReport = false;

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
    ReferenceCheckerOptions() {
        registerOption(
            "--file", "inputFile",
            [this](const char *arg) {
                file = arg;
                if (!std::filesystem::exists(file)) {
                    error("The input P4 program '%s' does not exist.", file.c_str());
                    return false;
                }
                return true;
            },
            "The input file to process.");
        registerOption(
            "--overwrite", nullptr,
            [this](const char *) {
                _overwriteReferences = true;
                return true;
            },
            "Do not check references, instead overwrite the reference.");
        registerOption(
            "--reference-file", "referenceFile",
            [this](const char *arg) {
                _referenceFile = arg;
                return true;
            },
            "The reference file to compare against.");
        registerOption(
            "--reference-folder", "referenceFolder",
            [this](const char *arg) {
                _referenceFolder = arg;
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
        registerOption(
            "--write-performance-report", nullptr,
            [this](const char *) {
                enablePerformanceLogging();
                _writePerformanceReport = true;
                return true;
            },
            "Write a performance report for the file. The report will be written to either the "
            "location of the reference file or the location of the folder.");
    }

    ~ReferenceCheckerOptions() override = default;

    [[nodiscard]] const char *getIncludePath() const override {
        P4C_UNIMPLEMENTED("getIncludePath not implemented for FlayClient.");
    }

    // Process options; return list of remaining options.
    // Returns EXIT_FAILURE if an error occurred.
    int processOptions(int argc, char *const argv[]) {
        auto *unprocessedOptions = process(argc, argv);
        if (unprocessedOptions != nullptr && !unprocessedOptions->empty()) {
            for (const auto &option : *unprocessedOptions) {
                error("Unprocessed input: %s", option);
            }
            return EXIT_FAILURE;
        }
        if (file.empty()) {
            error("No input file specified.");
            return EXIT_FAILURE;
        }
        if (!_referenceFile.has_value() && !_referenceFolder.has_value()) {
            error(
                "Neither a reference file nor a reference folder have been specified. Use either "
                "--reference-file or --reference-folder.");
            return EXIT_FAILURE;
        }
        if (_referenceFile.has_value() && _referenceFolder.has_value()) {
            error(
                "Both a reference file and a reference folder have been specified. Use only one.");
            return EXIT_FAILURE;
        }
        // If the environment variable P4TEST_REPLACE is set, overwrite the reference file.
        if (std::getenv("P4TEST_REPLACE") != nullptr) {
            _overwriteReferences = true;
        }
        // If the environment variable P4FLAY_INFO is set, enable information logging.
        if (std::getenv("P4FLAY_INFO") != nullptr) {
            enableInformationLogging();
        }
        return EXIT_SUCCESS;
    }

    [[nodiscard]] const std::filesystem::path &getInputFile() const { return file; }

    [[nodiscard]] std::optional<std::filesystem::path> getReferenceFile() const {
        return _referenceFile;
    }

    [[nodiscard]] std::optional<std::filesystem::path> getReferenceFolder() const {
        return _referenceFolder;
    }

    [[nodiscard]] bool doOverwriteReferences() const { return _overwriteReferences; }

    [[nodiscard]] const FlayOptions &toFlayOptions() const { return *this; }

    [[nodiscard]] bool writePerformanceReport() const { return _writePerformanceReport; }
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
        error("Unable to create pipe to diff command.");
        return EXIT_FAILURE;
    }
    // Read and print the output of the diff command.
    std::stringstream result;
    char buffer[128];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result << buffer;
    }
    if (pclose(pipe) != 0) {
        error("Diff command failed.\n%1%", result.str());
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

std::optional<std::filesystem::path> getFilePath(const ReferenceCheckerOptions &options,
                                                 const std::filesystem::path &basePath,
                                                 std::string_view suffix) {
    auto referenceFolderOpt = options.getReferenceFolder();
    auto referenceFileOpt = options.getReferenceFile();
    auto referencePath = basePath;
    if (referenceFileOpt.has_value()) {
        // If a reference file is explicitly provided, just overwrite this file.
        referencePath = referenceFileOpt.value();
    } else if (referenceFolderOpt.has_value()) {
        const auto &referenceFolder = referenceFolderOpt.value();
        try {
            std::filesystem::create_directories(referenceFolder);
        } catch (const std::exception &err) {
            error("Unable to create directory %1%: %2%", referenceFolder.c_str(), err.what());
            return std::nullopt;
        }
        referencePath = referenceFolder / referencePath;
    } else {
        error("Neither a reference file nor a reference folder have been specified.");
        return std::nullopt;
    }
    return referencePath.replace_extension(suffix);
}

}  // namespace

int run(const ReferenceCheckerOptions &options, const FlayOptions &flayOptions) {
    ASSIGN_OR_RETURN(auto flayServiceStatistics, Flay::optimizeProgram(flayOptions), EXIT_FAILURE);
    printInfo("Flay optimization complete.");
    auto referenceFolderOpt = options.getReferenceFolder();
    auto referenceFileOpt = options.getReferenceFile();

    if (options.writePerformanceReport()) {
        auto referencePath = getFilePath(options, options.getInputFile().stem(), ".perf");
        if (!referencePath.has_value()) {
            return EXIT_FAILURE;
        }
        printPerformanceReport(referencePath);
    }

    std::stringstream flayOptimizationOutput;
    for (const auto &[analysisName, statistic] : flayServiceStatistics) {
        flayOptimizationOutput << *statistic;
    }

    if (options.doOverwriteReferences()) {
        auto referencePath = getFilePath(options, options.getInputFile().stem(), ".ref");
        if (!referencePath.has_value()) {
            return EXIT_FAILURE;
        }
        std::ofstream ofs(referencePath.value());
        ofs << flayOptimizationOutput << std::endl;
        printInfo("Writing reference file %s", referencePath.value().c_str());
        ofs.close();
        return EXIT_SUCCESS;
    }
    if (referenceFileOpt.has_value()) {
        printInfo("Comparing against reference file %s", referenceFileOpt.value().c_str());
        auto referenceFile = std::filesystem::absolute(referenceFileOpt.value());
        return compareAgainstReference(flayOptimizationOutput, referenceFile);
    }
    if (referenceFolderOpt.has_value()) {
        auto referenceFolder = std::filesystem::absolute(referenceFolderOpt.value());
        if (!std::filesystem::is_directory(referenceFolder)) {
            error("Reference folder %1% does not exist or is not a folder.",
                  referenceFolder.c_str());
            return EXIT_FAILURE;
        }
        auto referenceName = options.getInputFile().stem();
        for (const auto &entry : std::filesystem::directory_iterator(referenceFolder)) {
            const auto &referenceFile = entry.path();
            if (referenceFile.extension() == ".ref" && referenceFile.stem() == referenceName) {
                return compareAgainstReference(flayOptimizationOutput, referenceFile);
            }
        }
        error("Reference file not found in folder.");
        return EXIT_FAILURE;
    }
    error("Neither a reference file nor a reference folder have been specified.");
    return EXIT_FAILURE;
}

}  // namespace P4::P4Tools::Flay

int main(int argc, char *argv[]) {
    P4::P4Tools::Flay::registerFlayTargets();

    // Set up the options.
    auto *compileContext =
        new P4::P4Tools::CompileContext<P4::P4Tools::Flay::ReferenceCheckerOptions>();
    P4::AutoCompileContext autoContext(
        new P4::P4CContextWithOptions<P4::P4Tools::Flay::ReferenceCheckerOptions>());
    // Process command-line options.
    if (compileContext->options().processOptions(argc, argv) != EXIT_SUCCESS) {
        return EXIT_FAILURE;
    }
    auto *flayContext =
        new P4::P4Tools::CompileContext<P4::P4Tools::Flay::FlayOptions>(*compileContext);
    P4::AutoCompileContext autoContext2(flayContext);
    // Run the reference checker.
    auto result = P4::P4Tools::Flay::run(compileContext->options(), flayContext->options());
    if (result == EXIT_FAILURE) {
        return EXIT_FAILURE;
    }
    return P4::errorCount() == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
