
#include "backends/p4tools/modules/flay/core/simplify_expression.h"

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

using namespace P4::literals;

// Tests for the optimization of various expressions.
TEST_F(OptimizationTest, Optimization01) {
    const auto *eightBitType = IR::Type_Bits::get(8);
    const auto *xVar =
        P4Tools::ToolsVariables::getSymbolicVariable(IR::Type_Boolean::get(), "X"_cs);
    const auto *aVar = P4Tools::ToolsVariables::getSymbolicVariable(eightBitType, "A"_cs);
    const auto *bVar = P4Tools::ToolsVariables::getSymbolicVariable(eightBitType, "B"_cs);

    // |X(bool)| ? |X(bool)| ? |A(bit<8>)| : |B(bit<8>)| : |B(bit<8>)|"
    const auto *nestedMuxExpression = new IR::Mux(xVar, new IR::Mux(xVar, aVar, bVar), bVar);
    const auto *optimizedExpression = P4Tools::SimplifyExpression::simplify(nestedMuxExpression);

    std::stringstream stringResult;
    optimizedExpression->dbprint(stringResult);

    ASSERT_STREQ("|X(bool)| ? |A(bit<8>)| : |B(bit<8>)|;", stringResult.str().c_str());
}

TEST_F(OptimizationTest, Optimization02) {
    const auto *eightBitType = IR::Type_Bits::get(8);
    const auto *xVar =
        P4Tools::ToolsVariables::getSymbolicVariable(IR::Type_Boolean::get(), "X"_cs);
    const auto *aVar = P4Tools::ToolsVariables::getSymbolicVariable(eightBitType, "A"_cs);
    const auto *bVar = P4Tools::ToolsVariables::getSymbolicVariable(eightBitType, "B"_cs);

    // |X(bool)| ? !|X(bool)| ? |A(bit<8>)| : |B(bit<8>)| : |B(bit<8>)|
    const auto *nestedMuxExpression =
        new IR::Mux(xVar, new IR::Mux(new IR::LNot(xVar), aVar, bVar), bVar);
    const auto *optimizedExpression = P4Tools::SimplifyExpression::simplify(nestedMuxExpression);

    std::stringstream stringResult;
    optimizedExpression->dbprint(stringResult);

    ASSERT_STREQ("|B(bit<8>)|", stringResult.str().c_str());
}

TEST_F(OptimizationTest, Optimization03) {
    const auto *eightBitType = IR::Type_Bits::get(8);
    const auto *xVar =
        P4Tools::ToolsVariables::getSymbolicVariable(IR::Type_Boolean::get(), "X"_cs);
    const auto *yVar =
        P4Tools::ToolsVariables::getSymbolicVariable(IR::Type_Boolean::get(), "Y"_cs);
    const auto *aVar = P4Tools::ToolsVariables::getSymbolicVariable(eightBitType, "A"_cs);
    const auto *bVar = P4Tools::ToolsVariables::getSymbolicVariable(eightBitType, "B"_cs);

    // |X(bool)| ? |Y(bool)| || |X(bool)| ? |A(bit<8>)| : |B(bit<8>)| : |B(bit<8>)|
    const auto *nestedMuxExpression =
        new IR::Mux(xVar, new IR::Mux(new IR::LOr(yVar, xVar), aVar, bVar), bVar);
    const auto *optimizedExpression = P4Tools::SimplifyExpression::simplify(nestedMuxExpression);

    std::stringstream stringResult;
    optimizedExpression->dbprint(stringResult);

    ASSERT_STREQ("|X(bool)| ? |A(bit<8>)| : |B(bit<8>)|;", stringResult.str().c_str());
}

TEST_F(OptimizationTest, Optimization04) {
    const auto *eightBitType = IR::Type_Bits::get(8);
    const auto *xVar =
        P4Tools::ToolsVariables::getSymbolicVariable(IR::Type_Boolean::get(), "X"_cs);
    const auto *yVar =
        P4Tools::ToolsVariables::getSymbolicVariable(IR::Type_Boolean::get(), "Y"_cs);
    const auto *aVar = P4Tools::ToolsVariables::getSymbolicVariable(eightBitType, "A"_cs);
    const auto *bVar = P4Tools::ToolsVariables::getSymbolicVariable(eightBitType, "B"_cs);
    {
        // |X(bool)| ? |Y(bool)| ? |A(bit<8>)| : |B(bit<8>)| : |B(bit<8>)|
        const auto *nestedMuxExpression = new IR::Mux(xVar, new IR::Mux(yVar, aVar, bVar), bVar);
        const auto *optimizedExpression =
            P4Tools::SimplifyExpression::simplify(nestedMuxExpression);

        std::stringstream stringResult;
        optimizedExpression->dbprint(stringResult);

        ASSERT_STREQ("|X(bool)| && |Y(bool)| ? |A(bit<8>)| : |B(bit<8>)|;",
                     stringResult.str().c_str());
    }
    {
        // |X(bool)| ? |Y(bool)| ? |B(bit<8>)| : |A(bit<8>)| : |B(bit<8>)|
        const auto *nestedMuxExpression = new IR::Mux(xVar, new IR::Mux(yVar, bVar, aVar), bVar);
        const auto *optimizedExpression =
            P4Tools::SimplifyExpression::simplify(nestedMuxExpression);

        std::stringstream stringResult;
        optimizedExpression->dbprint(stringResult);
        ASSERT_STREQ("|X(bool)| && !|Y(bool)| ? |A(bit<8>)| : |B(bit<8>)|;",
                     stringResult.str().c_str());
    }
    {
        // |X(bool)| ? |A(bit<8>)| : |Y(bool)| ? |A(bit<8>)| : |B(bit<8>)|
        const auto *nestedMuxExpression = new IR::Mux(xVar, aVar, new IR::Mux(yVar, aVar, bVar));
        const auto *optimizedExpression =
            P4Tools::SimplifyExpression::simplify(nestedMuxExpression);

        std::stringstream stringResult;
        optimizedExpression->dbprint(stringResult);
        ASSERT_STREQ("|X(bool)| || |Y(bool)| ? |A(bit<8>)| : |B(bit<8>)|;",
                     stringResult.str().c_str());
    }
    {
        // |X(bool)| ? |A(bit<8>)| : |Y(bool)| ? |B(bit<8>)| : |A(bit<8>)|
        const auto *nestedMuxExpression = new IR::Mux(xVar, aVar, new IR::Mux(yVar, bVar, aVar));
        const auto *optimizedExpression =
            P4Tools::SimplifyExpression::simplify(nestedMuxExpression);

        std::stringstream stringResult;
        optimizedExpression->dbprint(stringResult);
        ASSERT_STREQ("|X(bool)| || !|Y(bool)| ? |A(bit<8>)| : |B(bit<8>)|;",
                     stringResult.str().c_str());
    }
}

}  // anonymous namespace

}  // namespace Test
