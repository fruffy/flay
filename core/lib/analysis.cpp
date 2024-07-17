#include "backends/p4tools/modules/flay/core/lib/analysis.h"

#include "backends/p4tools/common/compiler/reachability.h"
#include "backends/p4tools/common/lib/logging.h"

namespace P4Tools::Flay {

size_t computeCyclomaticComplexity(const IR::P4Program &program) {
    auto *currentDCG = new P4Tools::NodesCallGraph("NodesCallGraph");
    auto cfgCreator = P4ProgramDCGCreator(currentDCG);
    program.apply(cfgCreator);
    auto sccNodes = currentDCG->nodes;
    size_t sccCount = 0;
    while (sccNodes.size() > 0) {
        std::vector<const IR::Node *> stronglyConnectedSet;
        auto hasCycle = currentDCG->sccSort(sccNodes.front(), stronglyConnectedSet);
        for (const auto *node : stronglyConnectedSet) {
            sccNodes.erase(node);
        }
        if (hasCycle) {
            sccCount++;
        }
    }

    auto nodeCount = currentDCG->nodes.size();
    // Only count the outbound edges in the DCG to avoid double counting.
    // Add one inbound edge for the entry node.
    // TODO: Add edges for the leaf nodes?
    size_t edgeCount = 1;
    for (const auto *node : currentDCG->nodes) {
        printFeature("cyclomatic_complexity", 4, "Node: %1% %2%", node->node_type_name(),
                     node->getSourceInfo().toBriefSourceFragment());
    }
    for (const auto &edgePair : currentDCG->getOutEdges()) {
        edgeCount += edgePair.second->size();
        for (const auto *targetNode : *edgePair.second) {
            printFeature("cyclomatic_complexity", 4, "Edge: %1% %2% -> %3% %4%",
                         edgePair.first->node_type_name(),
                         edgePair.first->getSourceInfo().toBriefSourceFragment(),
                         targetNode->node_type_name(),
                         targetNode->getSourceInfo().toBriefSourceFragment());
        }
    }

    printFeature("cyclomatic_complexity", 4, "Nodes: %1%", nodeCount);
    printFeature("cyclomatic_complexity", 4, "Edges: %1%", edgeCount);
    printFeature("cyclomatic_complexity", 4,
                 "Strongly-Connected-Components: " + std::to_string(sccCount));
    printFeature("cyclomatic_complexity", 4, "Cyclomatic Complexity: %1%",
                 edgeCount - nodeCount + 2);
    return edgeCount - nodeCount + 2 * sccCount;
}

ParserPathsCounter::ParserPathsCounter(const IR::P4Program &program) : program(program), count(0) {
    dcg = new P4Tools::NodesCallGraph("NodesCallGraph");
    auto cfgCreator = P4ProgramDCGCreator(dcg);
    program.apply(cfgCreator);
}

size_t ParserPathsCounter::computeParserPaths(const IR::P4Program &program) {
    ParserPathsCounter parserPathsCounter(program);
    // Visit all P4Parser to count the number of paths.
    program.apply(parserPathsCounter);
    printFeature("num_parsers_paths", 4, "num_parsers_paths: %1%", parserPathsCounter.getCount());
    return parserPathsCounter.getCount();
}

bool ParserPathsCounter::preorder(const IR::P4Control * /*control*/) { return false; }

bool ParserPathsCounter::preorder(const IR::P4Parser *parser) {
    count += countPaths(parser);
    return false;
}

size_t ParserPathsCounter::countPaths(const IR::P4Parser *parser) {
    level = 0;
    numPaths.clear();
    const auto *startState = parser->states.getDeclaration<IR::ParserState>(cstring("start"));
    return countPathsSub(startState);
}

size_t ParserPathsCounter::countPathsSub(DCGVertexType node) {
    if (node->is<IR::ParserState>()) {
        auto *parserState = node->to<IR::ParserState>();
        if (parserState->name == IR::ParserState::accept ||
            parserState->name == IR::ParserState::reject) {
            return 1;
        }
    } else if (node->is<IR::P4Control>()) {
        // If we reach a control block, that means we are moving out from parsers, we have to stop
        // counting paths.
        return 0;
    }

    auto it = numPaths.find(node);
    if (it != numPaths.end()) {
        return it->second;
    }
    level++;

    numPaths[node] = ParserPathsCounter::VISITING_FLAG;
    auto &targets = dcg->getOutEdges().at(node);
    BUG_CHECK(!targets->empty(),
              "Nodes should have children, because we already terminate on accept/reject");
    size_t count = 0;
    for (const auto *target : *targets) {
        if (numPaths.find(target) != numPaths.end() &&
            numPaths[target] == ParserPathsCounter::VISITING_FLAG) {
            // Ignore visiting nodes, meaning that we don't count parser loops.
            return 0;
        }
        count += countPathsSub(target);
    }
    numPaths[node] = count;
    level--;
    return count;
}

}  // namespace P4Tools::Flay
