#include "backends/p4tools/modules/flay/core/node_map.h"

namespace P4Tools::Flay {

bool NodeAnnotationMap::initializeReachabilityMapping(const IR::Node *node,
                                                      const IR::Expression *cond) {
    SymbolCollector collector;
    cond->apply(collector);
    const auto &collectedSymbols = collector.getCollectedSymbols();
    for (const auto &symbol : collectedSymbols) {
        _reachabilitySymbolMap[symbol.get()].emplace(node);
    }

    auto result = _reachabilityMap.emplace(node, std::set<ReachabilityExpression *>());
    result.first->second.insert(new ReachabilityExpression(cond));
    return result.second;
}

bool NodeAnnotationMap::initializeExpressionMapping(const IR::Expression *expression,
                                                    const IR::Expression *value) {
    SymbolCollector collector;
    value->apply(collector);
    const auto &collectedSymbols = collector.getCollectedSymbols();
    for (const auto &symbol : collectedSymbols) {
        _expressionSymbolMap[symbol.get()].emplace(expression);
    }
    auto result = _expressionMap.emplace(expression, value);
    return result.second;
}

void NodeAnnotationMap::mergeAnnotationMapping(const NodeAnnotationMap &otherMap) {
    for (const auto &rechabilityTuple : otherMap._reachabilityMap) {
        _reachabilityMap[rechabilityTuple.first].insert(rechabilityTuple.second.begin(),
                                                        rechabilityTuple.second.end());
    }
    for (const auto &symbol : otherMap.reachabilitySymbolMap()) {
        _reachabilitySymbolMap[symbol.first.get()].insert(symbol.second.begin(),
                                                          symbol.second.end());
    }
    for (const auto &symbol : otherMap.expressionSymbolMap()) {
        _expressionSymbolMap[symbol.first.get()].insert(symbol.second.begin(), symbol.second.end());
    }
}

void NodeAnnotationMap::substitutePlaceholders(Transform &substitute) {
    for (auto &[node, reachabilityExpressionVector] : _reachabilityMap) {
        for (const auto &reachabilityExpression : reachabilityExpressionVector) {
            reachabilityExpression->setCondition(
                reachabilityExpression->getCondition()->apply(substitute));
        }
    }
    // TODO: Substitions for the expression map.
}

SymbolMap NodeAnnotationMap::reachabilitySymbolMap() const { return _reachabilitySymbolMap; }

SymbolMap NodeAnnotationMap::expressionSymbolMap() const { return _expressionSymbolMap; }

ReachabilityMap NodeAnnotationMap::reachabilityMap() const { return _reachabilityMap; }

ExpressionMap NodeAnnotationMap::expressionMap() const { return _expressionMap; }

}  // namespace P4Tools::Flay
