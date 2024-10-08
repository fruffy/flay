#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_CORE_SPECIALIZATION_PASSES_SPECIALIZATION_STATISTICS_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_CORE_SPECIALIZATION_PASSES_SPECIALIZATION_STATISTICS_H_

/// The eliminated and optionally replaced node.
#include <utility>

#include "ir/node.h"

namespace P4::P4Tools::Flay {

using EliminatedReplacedPair = std::pair<const IR::Node *, const IR::Node *>;

}

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_CORE_SPECIALIZATION_PASSES_SPECIALIZATION_STATISTICS_H_ */
