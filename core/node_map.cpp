#include "backends/p4tools/modules/flay/core/node_map.h"

namespace P4Tools::Flay {

bool NodeAnnotationMap::initializeReachabilityMapping(const IR::Node *node,
                                                      const IR::Expression *cond) {
    SymbolCollector collector;
    cond->apply(collector);
    const auto &collectedSymbols = collector.getCollectedSymbols();
    for (const auto &symbol : collectedSymbols) {
        _symbolMap[symbol.get()].emplace(node);
    }

    auto result = _reachabilityMap.emplace(node, std::set<ReachabilityExpression *>());
    result.first->second.insert(new ReachabilityExpression(cond));
    return result.second;
}

void NodeAnnotationMap::mergeAnnotationMapping(const NodeAnnotationMap &otherMap) {
    for (const auto &rechabilityTuple : otherMap._reachabilityMap) {
        _reachabilityMap[rechabilityTuple.first].insert(rechabilityTuple.second.begin(),
                                                        rechabilityTuple.second.end());
    }
    for (const auto &symbol : otherMap.symbolMap()) {
        _symbolMap[symbol.first.get()].insert(symbol.second.begin(), symbol.second.end());
    }
}

void NodeAnnotationMap::substitutePlaceholders(Transform &substitute) {
    for (auto &[node, reachabilityExpressionVector] : _reachabilityMap) {
        for (const auto &reachabilityExpression : reachabilityExpressionVector) {
            reachabilityExpression->setCondition(
                reachabilityExpression->getCondition()->apply(substitute));
        }
    }
}

SymbolMap NodeAnnotationMap::symbolMap() const { return _symbolMap; }

ReachabilityMap NodeAnnotationMap::reachabilityMap() const { return _reachabilityMap; }

}  // namespace P4Tools::Flay
