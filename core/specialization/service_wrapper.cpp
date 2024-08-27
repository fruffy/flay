#include "backends/p4tools/modules/flay/core/specialization/service_wrapper.h"

#include <glob.h>

#include <algorithm>
#include <cstdlib>
#include <optional>
#include <vector>

#include "backends/p4tools/common/lib/logging.h"
#include "backends/p4tools/modules/flay/options.h"

namespace P4::P4Tools::Flay {

void FlayServiceWrapper::outputOptimizedProgram(
    const std::filesystem::path &optimizedOutputFileName) {
    auto absoluteFilePath =
        FlayOptions::get().optimizedOutputDir().value() / optimizedOutputFileName;
    _flayService.outputOptimizedProgram(absoluteFilePath);
    printInfo("Wrote optimized program to %1%", absoluteFilePath);
}

namespace {

/// Sourced from https://stackoverflow.com/a/9745132.
bool compareNat(const std::string &a, const std::string &b) {
    if (a.empty()) {
        return true;
    }
    if (b.empty()) {
        return false;
    }
    if ((std::isdigit(a[0]) != 0) && (std::isdigit(b[0]) == 0)) {
        return true;
    }
    if ((std::isdigit(a[0]) == 0) && (std::isdigit(b[0]) != 0)) {
        return false;
    }
    if ((std::isdigit(a[0]) == 0) && (std::isdigit(b[0]) == 0)) {
        if (std::toupper(a[0]) == std::toupper(b[0])) {
            return compareNat(a.substr(1), b.substr(1));
        }
        return (std::toupper(a[0]) < std::toupper(b[0]));
    }

    // Both strings begin with digit --> parse both numbers
    std::istringstream issa(a);
    std::istringstream issb(b);
    int ia = 0;
    int ib = 0;
    issa >> ia;
    issb >> ib;
    if (ia != ib) {
        return ia < ib;
    }

    // Numbers are the same --> remove numbers and recurse
    std::string anew;
    std::string bnew;
    std::getline(issa, anew);
    std::getline(issb, bnew);
    return (compareNat(anew, bnew));
}
}  // namespace

std::vector<std::string> FlayServiceWrapper::findFiles(std::string_view pattern) {
    std::vector<std::string> files;
    glob_t globResult;

    // Perform globbing.
    if (glob(pattern.data(), GLOB_TILDE, nullptr, &globResult) == 0) {
        for (size_t i = 0; i < globResult.gl_pathc; ++i) {
            files.emplace_back(globResult.gl_pathv[i]);
        }
    }
    // Ensures natural order.
    std::sort(files.begin(), files.end(), compareNat);

    // Free allocated resources
    globfree(&globResult);

    return files;
}

std::vector<AnalysisStatistics *> FlayServiceWrapper::computeFlayServiceStatistics() const {
    return _flayService.computeFlayServiceStatistics();
}

FlayServiceWrapper::FlayServiceWrapper(const FlayCompilerResult &compilerResult,
                                       IncrementalAnalysisMap incrementalAnalysisMap)
    : _flayService(compilerResult, std::move(incrementalAnalysisMap)) {}
}  // namespace P4::P4Tools::Flay
