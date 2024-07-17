#include "backends/p4tools/modules/flay/core/interpreter/node_map.h"

namespace P4Tools::Flay {

bool NodeAnnotationMap::initializeReachabilityMapping(const IR::Node *node,
                                                      const IR::Expression *cond) {
    SymbolCollector collector;
    cond->apply(collector);
    const auto &collectedSymbols = collector.collectedSymbols();
    for (const auto &symbol : collectedSymbols) {
        _reachabilitySymbolMap[symbol.get()].emplace(node);
    }

    auto it = _reachabilityMap.find(node);
    if (it != _reachabilityMap.end()) {
        it->second->addCondition(cond);
        return false;
    }
    return _reachabilityMap.emplace(node, new ReachabilityExpression(cond)).second;
}

bool NodeAnnotationMap::initializeExpressionMapping(const IR::Expression *expression,
                                                    const IR::Expression *value,
                                                    const IR::Expression *cond) {
    SymbolCollector collector;
    value->apply(collector);
    const auto &collectedSymbols = collector.collectedSymbols();
    for (const auto &symbol : collectedSymbols) {
        _expressionSymbolMap[symbol.get()].emplace(expression);
    }
    auto result = _expressionMap.emplace(expression, new SubstitutionExpression(cond, value));
    return result.second;
}

void NodeAnnotationMap::mergeAnnotationMapping(const NodeAnnotationMap &otherMap) {
    _reachabilityMap.insert(otherMap._reachabilityMap.begin(), otherMap._reachabilityMap.end());
    _expressionMap.insert(otherMap._expressionMap.begin(), otherMap._expressionMap.end());

    for (const auto &symbol : otherMap.reachabilitySymbolMap()) {
        _reachabilitySymbolMap[symbol.first.get()].insert(symbol.second.begin(),
                                                          symbol.second.end());
    }
    for (const auto &symbol : otherMap.expressionSymbolMap()) {
        _expressionSymbolMap[symbol.first.get()].insert(symbol.second.begin(), symbol.second.end());
    }
}

void NodeAnnotationMap::substitutePlaceholders(Transform &substitute) {
    for (auto &[node, reachabilityExpression] : _reachabilityMap) {
        reachabilityExpression->setCondition(
            reachabilityExpression->getCondition()->apply(substitute));
    }
    // TODO: Substitions for the expression map.
    P4C_UNIMPLEMENTED("NodeAnnotationMap::substitutePlaceholders not implemented");
}

SymbolMap NodeAnnotationMap::reachabilitySymbolMap() const { return _reachabilitySymbolMap; }

SymbolMap NodeAnnotationMap::expressionSymbolMap() const { return _expressionSymbolMap; }

ReachabilityMap NodeAnnotationMap::reachabilityMap() const { return _reachabilityMap; }

ExpressionMap NodeAnnotationMap::expressionMap() const { return _expressionMap; }

}  // namespace P4Tools::Flay
