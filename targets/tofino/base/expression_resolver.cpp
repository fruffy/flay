#include "backends/p4tools/modules/flay/targets/tofino/base/expression_resolver.h"

#include <functional>
#include <optional>

#include <boost/multiprecision/cpp_int.hpp>

#include "backends/p4tools/common/lib/variables.h"
#include "backends/p4tools/modules/flay/core/externs.h"
#include "backends/p4tools/modules/flay/core/target.h"
#include "backends/p4tools/modules/flay/targets/tofino/base/table_executor.h"
#include "backends/p4tools/modules/flay/targets/tofino/constants.h"
#include "ir/irutils.h"

namespace P4Tools::Flay::Tofino {

TofinoBaseExpressionResolver::TofinoBaseExpressionResolver(const ProgramInfo &programInfo,
                                                           ExecutionState &executionState)
    : ExpressionResolver(programInfo, executionState) {}

const IR::Expression *TofinoBaseExpressionResolver::processTable(const IR::P4Table *table) {
    auto copy = TofinoBaseExpressionResolver(*this);
    auto tableExecutor = TofinoBaseTableExecutor(*table, copy);
    return tableExecutor.processTable();
}

// Provides implementations of Tofino externs.
namespace TofinoBaseExterns {

auto ReturnDummyImpl = [](const ExternMethodImpls::ExternInfo &externInfo) {
    auto returnType =
        externInfo.originalCall.method->type->checkedTo<IR::Type_Method>()->returnType;
    auto label = externInfo.externObjectRef.path->toString() + "_" + externInfo.methodName + "_" +
                 std::to_string(externInfo.originalCall.clone_id);
    const auto *returnVar = ToolsVariables::getSymbolicVariable(returnType, label);
    return returnVar;
};

const ExternMethodImpls EXTERN_METHOD_IMPLS(
    {// -----------------------------------------------------------------------------
     // CHECKSUM
     // -----------------------------------------------------------------------------
     // Tofino checksum engine can verify the checksums for header-only checksums
     // and calculate the residual (checksum minus the header field
     // contribution) for checksums that include the payload.
     // Checksum engine only supports 16-bit ones' complement checksums, also known
     // as csum16 or internet checksum.
     // -----------------------------------------------------------------------------
     /// Add data to checksum.
     /// @param data : List of fields to be added to checksum calculation. The
     /// data must be byte aligned.
     {"Checksum.add",
      {"data"},
      [](const ExternMethodImpls::ExternInfo & /*externInfo*/) {
          // We do not calculate the checksum for now.
          return nullptr;
      }},
     /// Subtract data from existing checksum.
     /// @param data : List of fields to be subtracted from the checksum. The
     /// data must be byte aligned.
     {"Checksum.subtract",
      {"data"},
      [](const ExternMethodImpls::ExternInfo & /*externInfo*/) {
          // We do not calculate the checksum for now.
          return nullptr;
      }},
     /// Verify whether the complemented sum is zero, i.e. the checksum is valid.
     /// @return : Boolean flag indicating whether the checksum is valid or not.
     {"Checksum.verify",
      {},
      [](const ExternMethodImpls::ExternInfo &externInfo) {
          // We do not calculate the checksum for now and instead use a dummy.
          auto checksumLabel = externInfo.externObjectRef.path->toString() + "_" +
                               externInfo.methodName + "_" +
                               std::to_string(externInfo.originalCall.clone_id);
          return ToolsVariables::getSymbolicVariable(IR::Type_Boolean::get(), checksumLabel);
      }},
     /// Subtract all header fields after the current state and
     /// return the calculated checksum value.
     /// Marks the end position for residual checksum header.
     /// All header fields extracted after will be automatically subtracted.
     /// @param residual: The calculated checksum value for added fields.
     {"Checksum.subtract_all_and_deposit",
      {"residual"},
      [](const ExternMethodImpls::ExternInfo &externInfo) {
          const auto &resultVar =
              externInfo.externArgs->at(0)->expression->checkedTo<IR::InOutReference>()->ref;

          auto checksumLabel = externInfo.externObjectRef.path->toString() + "_" +
                               externInfo.methodName + "_" +
                               std::to_string(externInfo.originalCall.clone_id);
          const auto *hashCalc =
              ToolsVariables::getSymbolicVariable(resultVar->type, checksumLabel);
          externInfo.state.set(resultVar, hashCalc);
          return nullptr;
      }},
     /// Get the calculated checksum value.
     /// @return : The calculated checksum value for added fields.
     {"Checksum.get",
      {},
      [](const ExternMethodImpls::ExternInfo &externInfo) {
          auto checksumLabel = externInfo.externObjectRef.path->toString() + "_" +
                               externInfo.methodName + "_" +
                               std::to_string(externInfo.originalCall.clone_id);
          const auto *hashCalc =
              ToolsVariables::getSymbolicVariable(IR::getBitType(16), checksumLabel);
          return hashCalc;
      }},
     /// Calculate the checksum for a  given list of fields.
     /// @param data : List of fields contributing to the checksum value.
     /// @param zeros_as_ones : encode all-zeros value as all-ones.
     {"Checksum.update",
      {"data"},
      [](const ExternMethodImpls::ExternInfo &externInfo) {
          // We do not calculate the checksum for now and instead use a dummy.
          auto checksumLabel = externInfo.externObjectRef.path->toString() + "_" +
                               externInfo.methodName + "_" +
                               std::to_string(externInfo.originalCall.clone_id);
          const auto hashCalc =
              ToolsVariables::getSymbolicVariable(IR::getBitType(16), checksumLabel);
          return hashCalc;
      }},
     {"Checksum.update",
      {"data", "zeros_as_ones"},
      [](const ExternMethodImpls::ExternInfo &externInfo) {
          // We do not calculate the checksum for now and instead use a dummy.
          auto checksumLabel = externInfo.externObjectRef.path->toString() + "_" +
                               externInfo.methodName + "_" +
                               std::to_string(externInfo.originalCall.clone_id);
          const auto hashCalc =
              ToolsVariables::getSymbolicVariable(IR::getBitType(16), checksumLabel);
          return hashCalc;
      }},

     // -----------------------------------------------------------------------------
     // PARSER COUNTER
     // -----------------------------------------------------------------------------
     // Tofino parser counter can be used to extract header stacks or headers with
     // variable length. Tofino has a single 8-bit signed counter that can be
     // initialized with an immediate value or a header field.
     // -----------------------------------------------------------------------------
     /// Load the counter with an immediate value or a header field.
     {"ParserCounter.set",
      {"value"},
      [](const ExternMethodImpls::ExternInfo &externInfo) {
          P4C_UNIMPLEMENTED("Unimplemented extern method: %1%.%2%",
                            externInfo.externObjectRef.toString(), externInfo.methodName);
          return nullptr;
      }},
     /// Load the counter with a header field.
     /// @param max : Maximum permitted value for counter (pre rotate/mask/add).
     /// @param rotate : Right rotate (circular) the source field by this number of bits.
     /// @param mask : Mask the rotated source field by 2 ^ (mask + 1) - 1.
     /// @param add : Constant to add to the rotated and masked lookup field.
     {"ParserCounter.set",
      {"field", "max", "rotate", "mask", "add"},
      [](const ExternMethodImpls::ExternInfo &externInfo) {
          P4C_UNIMPLEMENTED("Unimplemented extern method: %1%.%2%",
                            externInfo.externObjectRef.toString(), externInfo.methodName);
          return nullptr;
      }},
     /// @return true if counter value is zero.
     {"ParserCounter.is_zero",
      {},
      [](const ExternMethodImpls::ExternInfo &externInfo) {
          P4C_UNIMPLEMENTED("Unimplemented extern method: %1%.%2%",
                            externInfo.externObjectRef.toString(), externInfo.methodName);
          return nullptr;
      }},
     /// @return true if counter value is negative.
     {"ParserCounter.is_negative",
      {},
      [](const ExternMethodImpls::ExternInfo &externInfo) {
          P4C_UNIMPLEMENTED("Unimplemented extern method: %1%.%2%",
                            externInfo.externObjectRef.toString(), externInfo.methodName);
          return nullptr;
      }},
     /// Add an immediate value to the parser counter.
     /// @param value : Constant to add to the counter.
     {"ParserCounter.increment",
      {"value"},
      [](const ExternMethodImpls::ExternInfo &externInfo) {
          P4C_UNIMPLEMENTED("Unimplemented extern method: %1%.%2%",
                            externInfo.externObjectRef.toString(), externInfo.methodName);
          return nullptr;
      }},
     /// Subtract an immediate value from the parser counter.
     /// @param value : Constant to subtract from the counter.
     {"ParserCounter.decrement",
      {"value"},
      [](const ExternMethodImpls::ExternInfo &externInfo) {
          P4C_UNIMPLEMENTED("Unimplemented extern method: %1%.%2%",
                            externInfo.externObjectRef.toString(), externInfo.methodName);
          return nullptr;
      }},

     // -----------------------------------------------------------------------------
     // PARSER PRIORITY
     // -----------------------------------------------------------------------------
     // Tofino ingress parser compare the priority with a configurable!!! threshold
     // to determine to whether drop the packet if the input buffer is congested.
     // Egress parser does not perform any dropping.
     // -----------------------------------------------------------------------------
     /// Set a new priority for the packet.
     /// param prio : parser priority for the parsed packet.
     {"ParserPriority.set",
      {"prio"},
      [](const ExternMethodImpls::ExternInfo & /*externInfo*/) { return nullptr; }},

     // -----------------------------------------------------------------------------
     // HASH ENGINE
     // -----------------------------------------------------------------------------
     // TODO: CRCPolynomial
     // extern CRCPolynomial<T> {
     //     CRCPolynomial(T coeff, bool reversed, bool msb, bool extended, T init, T xor);
     // }
     {"Hash.get", {"data"}, ReturnDummyImpl},
     // -----------------------------------------------------------------------------
     /// Random number generator.
     /// Return a random number with uniform distribution.
     /// @return : random number between 0 and 2**W - 1
     // -----------------------------------------------------------------------------
     {"Random.get", {}, ReturnDummyImpl},

     // -----------------------------------------------------------------------------
     // EXTERN FUNCTIONS
     // -----------------------------------------------------------------------------
     {"*method.max",
      {"t1", "t2"},
      [](const ExternMethodImpls::ExternInfo &externInfo) {
          auto t1 = externInfo.externArgs->at(0)->expression;
          auto t2 = externInfo.externArgs->at(1)->expression;
          auto returnVar = new IR::Mux(new IR::Grt(t1, t2), t1, t2);
          return returnVar;
      }},
     {"*method.min",
      {"t1", "t2"},
      [](const ExternMethodImpls::ExternInfo &externInfo) {
          auto t1 = externInfo.externArgs->at(0)->expression;
          auto t2 = externInfo.externArgs->at(1)->expression;
          auto returnVar = new IR::Mux(new IR::Lss(t1, t2), t1, t2);
          return returnVar;
      }},
     {"*method.funnel_shift_right",
      {"dst", "src1", "src2", "shift_amount"},
      [](const ExternMethodImpls::ExternInfo &externInfo) {
          P4C_UNIMPLEMENTED("Unimplemented extern method: %1%.%2%",
                            externInfo.externObjectRef.toString(), externInfo.methodName);
          return nullptr;
      }},
     {"*method.invalidate",
      {"field"},
      [](const ExternMethodImpls::ExternInfo &externInfo) {
          P4C_UNIMPLEMENTED("Unimplemented extern method: %1%.%2%",
                            externInfo.externObjectRef.toString(), externInfo.methodName);
          return nullptr;
      }},
     {"*method.is_validated",
      {"field"},
      [](const ExternMethodImpls::ExternInfo &externInfo) {
          P4C_UNIMPLEMENTED("Unimplemented extern method: %1%.%2%",
                            externInfo.externObjectRef.toString(), externInfo.methodName);
          return nullptr;
      }},
     /// Phase0
     {"*method.port_metadata_unpack",
      {"pkt"},
      [](const ExternMethodImpls::ExternInfo &externInfo) {
          auto &state = externInfo.state;
          // TODO: We currently just create a dummy variable, but this is not correct.
          // (Similar to Lookahead.)
          const auto *unpackType = externInfo.originalCall.typeArguments->at(0);
          const auto &externObjectRef = externInfo.externObjectRef;
          const auto &methodName = externInfo.methodName;
          auto unpackLabel = externObjectRef.path->toString() + "_" + methodName + "_" +
                             std::to_string(externInfo.originalCall.clone_id);
          return state.createSymbolicExpression(unpackType, unpackLabel);
      }},
     /// TODO: maybe create a type size visitor to calculate.
     {"*method.sizeInBits", {"h"}, ReturnDummyImpl},
     {"*method.sizeInBytes", {"h"}, ReturnDummyImpl},

     // -----------------------------------------------------------------------------
     /// Counter
     /// Indexed counter with `size’ independent counter values.
     // -----------------------------------------------------------------------------
     // TODO: Count currently has no effect in the symbolic interpreter.
     {"Counter.count",
      {"index"},
      [](const ExternMethodImpls::ExternInfo & /*externInfo*/) { return nullptr; }},
     {"Counter.count",
      {"index", "adjust_byte_count"},
      [](const ExternMethodImpls::ExternInfo & /*externInfo*/) { return nullptr; }},
     /// DirectCounter
     /// Increment the counter value.
     /// @param adjust_byte_count : optional parameter indicating value to be
     //                             subtracted from counter value.
     // TODO: Count currently has no effect in the symbolic interpreter.
     {"DirectCounter.count",
      {},
      [](const ExternMethodImpls::ExternInfo & /*externInfo*/) { return nullptr; }},
     {"DirectCounter.count",
      {"adjust_byte_count"},
      [](const ExternMethodImpls::ExternInfo & /*externInfo*/) { return nullptr; }},

     // -----------------------------------------------------------------------------
     /// Meter
     // -----------------------------------------------------------------------------
     {"Meter.execute", {"index", "color"}, ReturnDummyImpl},
     {"Meter.execute", {"index", "color", "adjust_byte_count"}, ReturnDummyImpl},
     {"Meter.execute", {"index"}, ReturnDummyImpl},
     {"Meter.execute", {"index", "adjust_byte_count"}, ReturnDummyImpl},

     // -----------------------------------------------------------------------------
     /// Direct meter.
     // -----------------------------------------------------------------------------
     {"DirectMeter.execute", {"color"}, ReturnDummyImpl},
     {"DirectMeter.execute", {"color", "adjust_byte_count"}, ReturnDummyImpl},
     {"DirectMeter.execute", {}, ReturnDummyImpl},
     {"DirectMeter.execute", {"adjust_byte_count"}, ReturnDummyImpl},

     // -----------------------------------------------------------------------------
     /// LPF
     // -----------------------------------------------------------------------------
     {"Lpf.execute",
      {"val", "index"},
      [](const ExternMethodImpls::ExternInfo &externInfo) {
          P4C_UNIMPLEMENTED("Unimplemented extern method: %1%.%2%",
                            externInfo.externObjectRef.toString(), externInfo.methodName);
          return nullptr;
      }},

     // -----------------------------------------------------------------------------
     /// Direct LPF
     // -----------------------------------------------------------------------------
     {"DirectLpf.execute",
      {"val"},
      [](const ExternMethodImpls::ExternInfo &externInfo) {
          P4C_UNIMPLEMENTED("Unimplemented extern method: %1%.%2%",
                            externInfo.externObjectRef.toString(), externInfo.methodName);
          return nullptr;
      }},

     // -----------------------------------------------------------------------------
     /// WRED
     // -----------------------------------------------------------------------------
     {"Wred.execute",
      {"val", "index"},
      [](const ExternMethodImpls::ExternInfo &externInfo) {
          P4C_UNIMPLEMENTED("Unimplemented extern method: %1%.%2%",
                            externInfo.externObjectRef.toString(), externInfo.methodName);
          return nullptr;
      }},

     // -----------------------------------------------------------------------------
     /// Direct WRED
     // -----------------------------------------------------------------------------
     {"DirectWred.execute",
      {"val"},
      [](const ExternMethodImpls::ExternInfo &externInfo) {
          P4C_UNIMPLEMENTED("Unimplemented extern method: %1%.%2%",
                            externInfo.externObjectRef.toString(), externInfo.methodName);
          return nullptr;
      }},

     // -----------------------------------------------------------------------------
     /// Register
     /// Return the value of register at specified index.
     // -----------------------------------------------------------------------------
     // TODO: Implement and actually keep track of the writes.
     {"Register.read", {"index"}, ReturnDummyImpl},
     // -----------------------------------------------------------------------------
     /// Write value to register at specified index.
     // -----------------------------------------------------------------------------
     // TODO: Implement and actually keep track of the writes.
     {"Register.write",
      {"index", "value"},
      [](const ExternMethodImpls::ExternInfo & /*externInfo*/) { return nullptr; }},

     // -----------------------------------------------------------------------------
     /// DirectRegister
     /// Return the value of the direct register.
     // -----------------------------------------------------------------------------
     // TODO: Implement and actually keep track of the writes.
     {"DirectRegister.read", {}, ReturnDummyImpl},
     // -----------------------------------------------------------------------------
     /// Write value to a direct register.
     // -----------------------------------------------------------------------------
     // TODO: Implement and actually keep track of the writes.
     {"DirectRegister.write",
      {"value"},
      [](const ExternMethodImpls::ExternInfo & /*externInfo*/) { return nullptr; }},

     // -----------------------------------------------------------------------------
     /// Return the value of the parameter.
     // -----------------------------------------------------------------------------
     {"RegisterParam.read",
      {},
      [](const ExternMethodImpls::ExternInfo &externInfo) {
          P4C_UNIMPLEMENTED("Unimplemented extern method: %1%.%2%",
                            externInfo.externObjectRef.toString(), externInfo.methodName);
          return nullptr;
      }},

     // -----------------------------------------------------------------------------
     /// MathUnit
     // -----------------------------------------------------------------------------
     {"MathUnit.execute",
      {"x"},
      [](const ExternMethodImpls::ExternInfo &externInfo) {
          P4C_UNIMPLEMENTED("Unimplemented extern method: %1%.%2%",
                            externInfo.externObjectRef.toString(), externInfo.methodName);
          return nullptr;
      }},

     // -----------------------------------------------------------------------------
     /// RegisterAction
     // This is implemented using an experimental feature in p4c and subject to
     // change. See https://github.com/p4lang/p4-spec/issues/561
     // U execute(in I index) {
     //     U rv;
     //     T value = reg.read(index);
     //     apply(value, rv);
     //     reg.write(index, value);
     //     return rv;
     // }
     // -----------------------------------------------------------------------------
     /// TODO: really model register. For now, we only return dummy.
     {"RegisterAction.execute",
      {"index"},
      [](const ExternMethodImpls::ExternInfo &externInfo) {
          auto &state = externInfo.state;
          const auto *actionDecl =
              state.findDecl(&externInfo.externObjectRef)->checkedTo<IR::Declaration_Instance>();
          const auto *actionType = actionDecl->type->checkedTo<IR::Type_Specialized>();
          BUG_CHECK(actionType->arguments->size() == 3, "Expected 3 arguments, got %1%",
                    actionType->arguments->size());
          const auto *valueType = actionType->arguments->at(0);  // T
          const auto &components = actionDecl->initializer->components;
          BUG_CHECK(components.size() == 1, "Expected 1 component, got %1%", components.size());
          const auto *applyDecl = components.at(0)->checkedTo<IR::Function>();
          const auto *applyParameters = applyDecl->type->parameters;

          const auto &valueName = applyParameters->parameters.at(0)->name.name;
          auto valueLabel =
              externInfo.externObjectRef.path->toString() + "_" + externInfo.methodName + "_" +
              std::to_string(externInfo.originalCall.clone_id) + "_apply_" + valueName;
          /// TODO: currently we create symbolic expression as the value, but we should model the
          /// values stored in register.
          const auto *valueExpr = state.createSymbolicExpression(valueType, valueLabel);

          if (applyParameters->size() == 2) {
              BUG_CHECK(applyParameters->getParameter(1)->direction == IR::Direction::Out,
                        "Direction of second parameter of apply is should be out");
          }
          std::vector<const IR::Expression *> arguments = {valueExpr, /*returnExpr=*/nullptr};
          std::vector<const IR::PathExpression *> paramRefs;
          for (size_t argIdx = 0; argIdx < applyParameters->size(); ++argIdx) {
              const auto *parameter = applyParameters->getParameter(argIdx);
              const auto *paramType = state.resolveType(parameter->type);
              const auto *argument = arguments.at(argIdx);
              const auto *paramRef =
                  new IR::PathExpression(paramType, new IR::Path(parameter->name));
              paramRefs.push_back(paramRef);
              if (paramType->is<IR::Type_StructLike>()) {
                  if (parameter->direction == IR::Direction::Out) {
                      state.initializeStructLike(FlayTarget::get(), paramRef, false);
                  } else {
                      state.assignStructLike(paramRef, argument);
                  }
              } else if (paramType->is<IR::Type_Base>()) {
                  if (parameter->direction == IR::Direction::Out) {
                      state.set(paramRef, FlayTarget::get().createTargetUninitialized(
                                              paramType->to<IR::Type_Base>(), false));
                  } else {
                      state.set(paramRef, argument);
                  }
              } else {
                  P4C_UNIMPLEMENTED("Unsupported parameter type %1%", paramType->node_type_name());
              }
          }
          auto &applyStepper = FlayTarget::getStepper(externInfo.programInfo.get(), state);
          applyDecl->body->apply(applyStepper);
          return state.get(paramRefs.at(1));
      }},
     // -----------------------------------------------------------------------------
     // Apply the implemented abstract method using an index that increments each
     // time. This method is useful for stateful logging.
     // -----------------------------------------------------------------------------
     {"RegisterAction.execute_log",
      {""},
      [](const ExternMethodImpls::ExternInfo &externInfo) {
          P4C_UNIMPLEMENTED("Unimplemented extern method: %1%.%2%",
                            externInfo.externObjectRef.toString(), externInfo.methodName);
          return nullptr;
      }},
     // -----------------------------------------------------------------------------
     // Abstract method that needs to be implemented when RegisterAction is
     // instantiated.
     // @param value : register value.
     // @param rv : return value.
     // -----------------------------------------------------------------------------
     {"RegisterAction.apply",
      {"value", "rv"},
      [](const ExternMethodImpls::ExternInfo &externInfo) {
          P4C_UNIMPLEMENTED("Unimplemented extern method: %1%.%2%",
                            externInfo.externObjectRef.toString(), externInfo.methodName);
          return nullptr;
      }},
     {"RegisterAction.predicate",
      {"cmplo", "cmphi"},  // TODO: the two arguments are both optional
      [](const ExternMethodImpls::ExternInfo &externInfo) {
          P4C_UNIMPLEMENTED("Unimplemented extern method: %1%.%2%",
                            externInfo.externObjectRef.toString(), externInfo.methodName);
          return nullptr;
      }},

     // -----------------------------------------------------------------------------
     // DirectRegisterAction
     //
     // U execute() {
     //     U rv;
     //     T value = reg.read();
     //     apply(value, rv);
     //     reg.write(value);
     //     return rv;
     // }
     // -----------------------------------------------------------------------------
     // TODO: support using real apply method to execute.
     {"DirectRegisterAction.execute",
      {},
      [](const ExternMethodImpls::ExternInfo &externInfo) {
          auto &state = externInfo.state;
          const auto *actionDecl =
              state.findDecl(&externInfo.externObjectRef)->checkedTo<IR::Declaration_Instance>();
          const auto *actionType = actionDecl->type->checkedTo<IR::Type_Specialized>();
          BUG_CHECK(actionType->arguments->size() == 2, "Expected 2 arguments, got %1%",
                    actionType->arguments->size());
          const auto *valueType = actionType->arguments->at(0);  // T
          const auto &components = actionDecl->initializer->components;
          BUG_CHECK(components.size() == 1, "Expected 1 component, got %1%", components.size());
          const auto *applyDecl = components.at(0)->checkedTo<IR::Function>();
          const auto *applyParameters = applyDecl->type->parameters;

          const auto &valueName = applyParameters->parameters.at(0)->name.name;
          auto valueLabel =
              externInfo.externObjectRef.path->toString() + "_" + externInfo.methodName + "_" +
              std::to_string(externInfo.originalCall.clone_id) + "_apply_" + valueName;
          /// TODO: currently we create symbolic expression as the value, but we should model the
          /// values stored in register.
          const auto *valueExpr = state.createSymbolicExpression(valueType, valueLabel);

          if (applyParameters->size() == 2) {
              BUG_CHECK(applyParameters->getParameter(1)->direction == IR::Direction::Out,
                        "Direction of second parameter of apply is should be out");
          }
          std::vector<const IR::Expression *> arguments = {valueExpr, /*returnExpr=*/nullptr};
          std::vector<const IR::PathExpression *> paramRefs;
          for (size_t argIdx = 0; argIdx < applyParameters->size(); ++argIdx) {
              const auto *parameter = applyParameters->getParameter(argIdx);
              const auto *paramType = state.resolveType(parameter->type);
              const auto *argument = arguments.at(argIdx);
              const auto *paramRef =
                  new IR::PathExpression(paramType, new IR::Path(parameter->name));
              paramRefs.push_back(paramRef);
              if (paramType->is<IR::Type_StructLike>()) {
                  if (parameter->direction == IR::Direction::Out) {
                      state.initializeStructLike(FlayTarget::get(), paramRef, false);
                  } else {
                      state.assignStructLike(paramRef, argument);
                  }
              } else if (paramType->is<IR::Type_Base>()) {
                  if (parameter->direction == IR::Direction::Out) {
                      state.set(paramRef, FlayTarget::get().createTargetUninitialized(
                                              paramType->to<IR::Type_Base>(), false));
                  } else {
                      state.set(paramRef, argument);
                  }
              } else {
                  P4C_UNIMPLEMENTED("Unsupported parameter type %1%", paramType->node_type_name());
              }
          }
          auto &applyStepper = FlayTarget::getStepper(externInfo.programInfo.get(), state);
          applyDecl->body->apply(applyStepper);
          return state.get(paramRefs.at(1));
      }},
     // -----------------------------------------------------------------------------
     // DirectRegisterAction.apply
     // -----------------------------------------------------------------------------
     {"DirectRegisterAction.apply",
      {"value", "rv"},
      [](const ExternMethodImpls::ExternInfo &externInfo) {
          P4C_UNIMPLEMENTED("Unimplemented extern method: %1%.%2%",
                            externInfo.externObjectRef.toString(), externInfo.methodName);
          return nullptr;
      }},
     // -----------------------------------------------------------------------------
     // DirectRegisterAction.predicate
     // -----------------------------------------------------------------------------
     {"DirectRegisterAction.predicate",
      {"cmplo", "cmphi"},  // TODO: the two arguments are both optional
      [](const ExternMethodImpls::ExternInfo &externInfo) {
          P4C_UNIMPLEMENTED("Unimplemented extern method: %1%.%2%",
                            externInfo.externObjectRef.toString(), externInfo.methodName);
          return nullptr;
      }},

     // -----------------------------------------------------------------------------
     /// SelectorAction
     // -----------------------------------------------------------------------------
     {"SelectorAction.execute",
      {"index"},
      [](const ExternMethodImpls::ExternInfo &externInfo) {
          P4C_UNIMPLEMENTED("Unimplemented extern method: %1%.%2%",
                            externInfo.externObjectRef.toString(), externInfo.methodName);
          return nullptr;
      }},
     {"SelectorAction.apply",
      {"value", "rv"},  // TODO: argument `rv` is both optional
      [](const ExternMethodImpls::ExternInfo &externInfo) {
          P4C_UNIMPLEMENTED("Unimplemented extern method: %1%.%2%",
                            externInfo.externObjectRef.toString(), externInfo.methodName);
          return nullptr;
      }},

     // -----------------------------------------------------------------------------
     // Tofino supports mirroring both at the ingress and egress. Ingress deparser
     // creates a copy of the original ingress packet and prepends the mirror header.
     // Egress deparser first constructs the output packet and then prepends the
     // mirror header.
     // -----------------------------------------------------------------------------
     /// Mirror the packet.
     // -----------------------------------------------------------------------------
     {"Mirror.emit",
      {"session_id"},
      [](const ExternMethodImpls::ExternInfo &externInfo) {
          P4C_UNIMPLEMENTED("Unimplemented extern method: %1%.%2%",
                            externInfo.externObjectRef.toString(), externInfo.methodName);
          return nullptr;
      }},
     // -----------------------------------------------------------------------------
     /// Write @hdr into the ingress/egress mirror buffer.
     /// @param hdr : T can be a header type.
     // -----------------------------------------------------------------------------
     {"Mirror.emit",
      {"session_id", "hdr"},
      [](const ExternMethodImpls::ExternInfo &externInfo) {
          P4C_UNIMPLEMENTED("Unimplemented extern method: %1%.%2%",
                            externInfo.externObjectRef.toString(), externInfo.methodName);
          return nullptr;
      }},

     // -----------------------------------------------------------------------------
     // Tofino supports packet resubmission at the end of ingress pipeline. When
     // a packet is resubmitted, the original packet reference and some limited
     // amount of metadata (64 bits) are passed back to the packet’s original
     // ingress buffer, where the packet is enqueued again.
     // -----------------------------------------------------------------------------
     /// Resubmit the packet.
     // -----------------------------------------------------------------------------
     {"Resubmit.emit",
      {},
      [](const ExternMethodImpls::ExternInfo &externInfo) {
          P4C_UNIMPLEMENTED("Unimplemented extern method: %1%.%2%",
                            externInfo.externObjectRef.toString(), externInfo.methodName);
          return nullptr;
      }},
     // -----------------------------------------------------------------------------
     /// Resubmit the packet and prepend it with @hdr.
     /// @param hdr : T can be a header type.
     // -----------------------------------------------------------------------------
     {"Resubmit.emit",
      {"hdr"},
      [](const ExternMethodImpls::ExternInfo &externInfo) {
          P4C_UNIMPLEMENTED("Unimplemented extern method: %1%.%2%",
                            externInfo.externObjectRef.toString(), externInfo.methodName);
          return nullptr;
      }},

     // -----------------------------------------------------------------------------
     // Digest
     // -----------------------------------------------------------------------------
     {"Digest.pack", {"data"}, [](const ExternMethodImpls::ExternInfo & /*externInfo*/) {
          return nullptr;
      }}});
}  // namespace TofinoBaseExterns

const IR::Expression *TofinoBaseExpressionResolver::processExtern(
    const ExternMethodImpls::ExternInfo &externInfo) {
    auto method = TofinoBaseExterns::EXTERN_METHOD_IMPLS.find(
        externInfo.externObjectRef, externInfo.methodName, externInfo.externArgs);
    if (method.has_value()) {
        return method.value()(externInfo);
    }
    return ExpressionResolver::processExtern(externInfo);
}

}  // namespace P4Tools::Flay::Tofino
