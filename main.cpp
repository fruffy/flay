#include <cstdlib>
#include <exception>
#include <iostream>
#include <vector>

#include "backends/p4tools/common/lib/logging.h"
#include "backends/p4tools/modules/flay/flay.h"
#include "backends/p4tools/modules/flay/toolname.h"
#include "lib/crash.h"
#include "lib/exceptions.h"
#include "lib/timer.h"

int main(int argc, char **argv) {
    P4::setup_signals();
    std::vector<const char *> args;
    args.reserve(argc);
    for (int i = 0; i < argc; ++i) {
        args.push_back(argv[i]);
    }

    int result = EXIT_SUCCESS;
    try {
        P4::Util::ScopedTimer timer("Flay Main");
        result = P4::P4Tools::Flay::Flay().main(P4::P4Tools::Flay::TOOL_NAME, args);
    } catch (const P4::Util::CompilerBug &e) {
        std::cerr << "Internal error: " << e.what() << '\n';
        std::cerr << "Please submit a bug report with your code." << '\n';
        result = EXIT_FAILURE;
    } catch (const P4::Util::CompilerUnimplemented &e) {
        std::cerr << e.what() << '\n';
        result = EXIT_FAILURE;
    } catch (const P4::Util::CompilationError &e) {
        std::cerr << e.what() << '\n';
        result = EXIT_FAILURE;
    } catch (const std::exception &e) {
        std::cerr << "Internal error: " << e.what() << '\n';
        std::cerr << "Please submit a bug report with your code." << '\n';
        result = EXIT_FAILURE;
    } catch (...) {
        std::cerr << "Internal error. Please submit a bug report with your code." << '\n';
        result = EXIT_FAILURE;
    }
    P4::P4Tools::printPerformanceReport();
    return result;
}
