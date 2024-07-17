#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_CORE_LIB_ANALYSIS_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_CORE_LIB_ANALYSIS_H_

#include <unordered_map>

#include "backends/p4tools/common/compiler/reachability.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "ir/ir.h"
#include "ir/visitor.h"

namespace P4Tools::Flay {

size_t computeCyclomaticComplexity(const IR::P4Program &program);

class ParserPathsCounter : public Inspector {
 public:
    explicit ParserPathsCounter(const IR::P4Program &program);

    size_t getCount() const { return count; }

    static size_t computeParserPaths(const IR::P4Program &program);

 private:
    /// The program to analyze.
    const IR::P4Program &program;

    P4::ReferenceMap refMap;

    size_t count;

    /// Visit all transition target states.
    void processSelectExpression(const IR::SelectExpression *selectExpr);

    bool preorder(const IR::P4Control *control) override;
    bool preorder(const IR::P4Parser *parser) override;

 private:
    /// For counting paths in a parser.
    P4Tools::NodesCallGraph *dcg;
    int level = 0;

    static const size_t VISITING_FLAG = -1;
    /// Map of number of paths to end from each vertex.
    std::unordered_map<DCGVertexType, size_t> numPaths;

    /// Entry method for counting paths
    size_t countPaths(const IR::P4Parser *parser);
    /// Recursive method for counting paths
    size_t countPathsSub(DCGVertexType node);
};

}  // namespace P4Tools::Flay

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_CORE_LIB_ANALYSIS_H_ */
