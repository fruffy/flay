#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_CORE_LIB_COLLAPSE_DATAPLANE_VARIABLES_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_CORE_LIB_COLLAPSE_DATAPLANE_VARIABLES_H_

#include "ir/ir.h"
#include "ir/irutils.h"
#include "ir/node.h"
#include "ir/visitor.h"

namespace P4::P4Tools::Flay {

class DataPlaneVariablePropagator : public Transform {
    const IR::Node *postorder(IR::Operation_Unary *unary_op) override;
    const IR::Node *postorder(IR::Cast *cast) override;
    const IR::Node *postorder(IR::Operation_Binary *bin_op) override;
    const IR::Node *postorder(IR::LAnd *lAnd) override;
    const IR::Node *postorder(IR::Operation_Relation *relOp) override;
    const IR::Node *postorder(IR::Concat *concat) override;
    const IR::Node *postorder(IR::Slice *slice) override;
    const IR::Node *preorder(IR::Mux *mux) override;

 public:
    DataPlaneVariablePropagator();
};

}  // namespace P4::P4Tools::Flay

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_CORE_LIB_COLLAPSE_DATAPLANE_VARIABLES_H_ */
