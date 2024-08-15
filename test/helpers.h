#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_TEST_HELPERS_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_TEST_HELPERS_H_

#include <gtest/gtest.h>

#include "backends/p4tools/common/compiler/context.h"
#include "backends/p4tools/modules/flay/core/interpreter/target.h"
#include "backends/p4tools/modules/flay/register.h"
#include "backends/p4tools/modules/flay/toolname.h"
#include "lib/compile_context.h"

namespace P4::P4Tools {

class P4FlayTest : public testing::Test {
    AutoCompileContext *_ctx = nullptr;

 public:
    void SetUp() override {
        _ctx =
            new AutoCompileContext(new P4Tools::CompileContext<P4::P4Tools::Flay::FlayOptions>());
    }

    [[nodiscard]] static std::optional<std::unique_ptr<AutoCompileContext>> SetUp(
        std::string_view target, std::string_view archName) {
        P4::P4Tools::Flay::registerFlayTargets();
        /// Set up the appropriate compile context for P4Flay tests.
        /// TODO: Remove this once options are not initialized statically anymore.
        auto ctxOpt = P4::P4Tools::Flay::FlayTarget::initializeTarget(P4::P4Tools::Flay::TOOL_NAME,
                                                                      target, archName);

        if (!ctxOpt.has_value()) {
            return std::nullopt;
        }
        return std::make_unique<AutoCompileContext>(ctxOpt.value());
    }
};

}  // namespace P4::P4Tools

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_TEST_HELPERS_H_ */
