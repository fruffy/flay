
#include "backends/p4tools/modules/flay/core/collapse_mux.h"

#include <gtest/gtest.h>

#include <sstream>

#include <boost/multiprecision/cpp_int.hpp>

#include "backends/p4tools/common/lib/variables.h"
#include "ir/ir.h"
#include "ir/irutils.h"

namespace Test {

namespace {

/// Helper methods to build configurations for Optimization Tests.
class OptimizationTest : public ::testing::Test {};

// Tests for the optimization of various expressions.
TEST_F(OptimizationTest, Optimization01) {
    const auto *eightBitType = IR::getBitType(8);
    const auto *xVar = P4Tools::ToolsVariables::getSymbolicVariable(IR::Type_Boolean::get(), "X");
    const auto *aVar = P4Tools::ToolsVariables::getSymbolicVariable(eightBitType, "A");
    const auto *bVar = P4Tools::ToolsVariables::getSymbolicVariable(eightBitType, "B");

    const auto *nestedMuxExpression = new IR::Mux(xVar, new IR::Mux(xVar, aVar, bVar), bVar);
    const auto *optimizedExpression = P4Tools::CollapseMux::optimizeExpression(nestedMuxExpression);

    std::stringstream stringResult;
    optimizedExpression->dbprint(stringResult);

    ASSERT_STREQ("|X(bool)| ? |A(bit<8>)| : |B(bit<8>)|;", stringResult.str().c_str());
}

TEST_F(OptimizationTest, Optimization02) {
    const auto *eightBitType = IR::getBitType(8);
    const auto *xVar = P4Tools::ToolsVariables::getSymbolicVariable(IR::Type_Boolean::get(), "X");
    const auto *aVar = P4Tools::ToolsVariables::getSymbolicVariable(eightBitType, "A");
    const auto *bVar = P4Tools::ToolsVariables::getSymbolicVariable(eightBitType, "B");

    const auto *nestedMuxExpression =
        new IR::Mux(xVar, new IR::Mux(new IR::LNot(xVar), aVar, bVar), bVar);
    const auto *optimizedExpression = P4Tools::CollapseMux::optimizeExpression(nestedMuxExpression);

    std::stringstream stringResult;
    optimizedExpression->dbprint(stringResult);

    ASSERT_STREQ("|B(bit<8>)|", stringResult.str().c_str());
}

TEST_F(OptimizationTest, Optimization03) {
    const auto *eightBitType = IR::getBitType(8);
    const auto *xVar = P4Tools::ToolsVariables::getSymbolicVariable(IR::Type_Boolean::get(), "X");
    const auto *yVar = P4Tools::ToolsVariables::getSymbolicVariable(IR::Type_Boolean::get(), "Y");
    const auto *aVar = P4Tools::ToolsVariables::getSymbolicVariable(eightBitType, "A");
    const auto *bVar = P4Tools::ToolsVariables::getSymbolicVariable(eightBitType, "B");

    const auto *nestedMuxExpression =
        new IR::Mux(xVar, new IR::Mux(new IR::LOr(yVar, xVar), aVar, bVar), bVar);
    const auto *optimizedExpression = P4Tools::CollapseMux::optimizeExpression(nestedMuxExpression);

    std::stringstream stringResult;
    optimizedExpression->dbprint(stringResult);

    ASSERT_STREQ("|X(bool)| ? |A(bit<8>)| : |B(bit<8>)|;", stringResult.str().c_str());
}

}  // anonymous namespace

}  // namespace Test
