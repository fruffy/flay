#include "backends/p4tools/modules/flay/core/analysis.h"

#include "backends/p4tools/common/compiler/reachability.h"
#include "backends/p4tools/common/lib/logging.h"
#include "backends/p4tools/common/lib/util.h"

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

}  // namespace P4Tools::Flay
