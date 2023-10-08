/* Copyright (c) 2023 Intel Corporation

Copyright 2020 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#ifndef ITEX_CORE_COMPILER_MLIR_XLA_TRANSFORMS_MHLO_TO_LHLO_WITH_XLA_H_
#define ITEX_CORE_COMPILER_MLIR_XLA_TRANSFORMS_MHLO_TO_LHLO_WITH_XLA_H_

#include <memory>
#include <utility>

#include "absl/types/optional.h"
#include "itex/core/compiler/xla/service/buffer_assignment.h"
#include "itex/core/compiler/xla/service/hlo_instructions.h"
#include "itex/core/compiler/xla/service/hlo_module.h"
#include "itex/core/compiler/xla/shape_util.h"
#include "itex/core/compiler/xla/statusor.h"
#include "lhlo/IR/lhlo_ops.h"
#include "lhlo_gpu/IR/lhlo_gpu_ops.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"   // from @llvm-project
#include "mlir/Dialect/MemRef/IR/MemRef.h"  // from @llvm-project
#include "mlir/IR/Attributes.h"             // from @llvm-project
#include "mlir/IR/Builders.h"               // from @llvm-project
#include "mlir/IR/BuiltinOps.h"             // from @llvm-project
#include "mlir/IR/BuiltinTypes.h"           // from @llvm-project

namespace mlir {

// This class will process an HloModule with the supplied BufferAssignment and
// populate the MLIR ModuleOp with the computation converted in the LHLO
// dialect.
class LhloDialectEmitter : public itex_xla::ConstDfsHloVisitorWithDefault {
 public:
  // Initializes internal data structures. It must be called before calling any
  // of the visitors.
  itex::Status Initialize();

  LhloDialectEmitter(const itex_xla::BufferAssignment& assignment,
                     const itex_xla::HloComputation& computation,
                     ModuleOp module)
      : assignment_(std::move(assignment)),
        computation_(computation),
        module_(module),
        builder_(module.getContext()),
        i8_type_(builder_.getIntegerType(8)) {}

  itex_xla::StatusOr<mlir::Operation*> EmitOp(
      const itex_xla::HloInstruction* instr);

  static itex_xla::StatusOr<mhlo::ScatterDimensionNumbersAttr>
  GetScatterDimensionNumbers(const itex_xla::HloInstruction* instr,
                             mlir::MLIRContext* context);

 private:
  itex_xla::StatusOr<lmhlo::SortOp> EmitSortOp(
      const itex_xla::HloInstruction* instr);
  itex_xla::StatusOr<lmhlo::FusionOp> EmitFusionOp(
      const itex_xla::HloInstruction* instr);
  itex_xla::StatusOr<lmhlo::ScatterOp> EmitScatterOp(
      const itex_xla::HloInstruction* instr);
  itex_xla::StatusOr<lmhlo::SelectAndScatterOp> EmitSelectAndScatterOp(
      const itex_xla::HloInstruction* instr);

  itex_xla::StatusOr<Operation*> EmitCustomCallOp(
      const itex_xla::HloInstruction* instr);
  itex_xla::StatusOr<lmhlo_gpu::CholeskyOp> EmitCholesky(
      const itex_xla::HloCustomCallInstruction* custom_call);
  itex_xla::StatusOr<Operation*> EmitGemm(
      const itex_xla::HloCustomCallInstruction* custom_call);
  itex_xla::StatusOr<Operation*> EmitDnnConvolution(
      const itex_xla::HloCustomCallInstruction* custom_call);
  itex_xla::StatusOr<Operation*> EmitDnnBatchNorm(
      const itex_xla::HloCustomCallInstruction* custom_call);

  itex_xla::StatusOr<memref::GetGlobalOp> EmitConstant(
      const itex_xla::HloInstruction* instr);

  itex_xla::StatusOr<lmhlo::InfeedOp> EmitInfeedOp(
      const itex_xla::HloInstruction* instr);
  itex_xla::StatusOr<lmhlo::OutfeedOp> EmitOutfeedOp(
      const itex_xla::HloInstruction* instr);

  itex_xla::StatusOr<lmhlo::AllToAllOp> EmitAllToAllOp(
      const itex_xla::HloInstruction* instr);
  itex_xla::StatusOr<lmhlo::AllGatherOp> EmitAllGatherOp(
      const itex_xla::HloInstruction* instr);
  itex_xla::StatusOr<lmhlo::AllReduceOp> EmitAllReduceOp(
      const itex_xla::HloInstruction* instr);
  itex_xla::StatusOr<lmhlo_gpu::AllReduceStartOp> EmitAllReduceStartOp(
      const itex_xla::HloInstruction* instr);
  itex_xla::StatusOr<lmhlo_gpu::AllReduceDoneOp> EmitAllReduceDoneOp(
      const itex_xla::HloInstruction* instr);
  itex_xla::StatusOr<lmhlo::ReduceScatterOp> EmitReduceScatterOp(
      const itex_xla::HloInstruction* instr);
  itex_xla::StatusOr<lmhlo::CollectivePermuteOp> EmitCollectivePermuteOp(
      const itex_xla::HloInstruction* instr);

  itex_xla::StatusOr<lmhlo::RngGetAndUpdateStateOp> EmitRngGetAndUpdateStateOp(
      const itex_xla::HloInstruction* instr);
  itex_xla::StatusOr<lmhlo::FftOp> EmitFftOp(
      const itex_xla::HloInstruction* instr);
  itex_xla::StatusOr<lmhlo::TriangularSolveOp> EmitTriangularSolveOp(
      const itex_xla::HloInstruction* instr);
  itex_xla::StatusOr<Operation*> EmitBitcast(
      const itex_xla::HloInstruction* instr);

  itex_xla::StatusOr<lmhlo::CaseOp> EmitCaseOp(
      const itex_xla::HloInstruction* instr);

  itex_xla::StatusOr<lmhlo::WhileOp> EmitWhileOp(
      const itex_xla::HloInstruction* instr);

  itex_xla::Status ImportAsLmhloRegion(itex_xla::HloComputation* computation,
                                       mlir::Region* region);

  // Since LMHLO dialect does not define token types, this enum controls how
  // token operand/results from XLA:HLO are lowered to MLIR.
  enum class TokenLoweringMode {
    kFailToLower,  // Fail lowering if token inputs are encountered.
    kUseNull,      // Use a null Value in the operand list for each token.
    // kSkip,        // Skip any token inputs or outputs (not yet needed)
  };

  // Create LHLO operation operands given an XLA HLO instruction. By default,
  // all XLA HLO operands and results are converted to MLIR and appended to
  // `operands`. If `num_operands` is specified, only the first `num_operand`
  // operands of the instruction are converted to MLIR. The function returns the
  // actual number of operands and results generated for MLIR in `num_arguments`
  // and `num_results`.
  itex_xla::Status CreateOperands(const itex_xla::HloInstruction* instr,
                                  absl::optional<int64_t> num_operands,
                                  TokenLoweringMode token_mode,
                                  SmallVectorImpl<Value>& operands,
                                  size_t& num_arguments, size_t& num_results);

  template <typename OpType>
  itex_xla::StatusOr<OpType> CreateOpWithoutAttrs(
      const itex_xla::HloInstruction* instr,
      absl::optional<int64_t> num_operands = absl::nullopt) {
    size_t unused;
    return CreateOpWithoutAttrs<OpType>(instr, unused, unused, num_operands);
  }

  template <typename OpType>
  itex_xla::StatusOr<OpType> CreateOpWithoutAttrs(
      const itex_xla::HloInstruction* instr, size_t& num_arguments,
      size_t& num_results,
      absl::optional<int64_t> num_operands = absl::nullopt);

  template <typename OpType>
  OpType CreateOpWithoutAttrs(const itex_xla::HloInstruction* instr,
                              ValueRange operands);

  itex_xla::StatusOr<mlir::Operation*> CreateOpInFusion(
      const itex_xla::HloInstruction* instr, ValueRange buffer_operands,
      size_t num_arguments, size_t num_results);

  itex_xla::StatusOr<mlir::Operation*> CreateOpInFusion(
      const itex_xla::HloInstruction* instr);

  template <typename T>
  DenseIntElementsAttr GetI64DenseElementsAttr(const T& container) {
    return builder_.getI64TensorAttr(
        {container.data(), static_cast<size_t>(container.size())});
  }

  DenseIntElementsAttr GetWindowElements(
      const itex_xla::Window& window,
      std::function<int64_t(const itex_xla::WindowDimension& dim)> getter) {
    llvm::SmallVector<int64_t, 4> elements;
    elements.reserve(window.dimensions_size());
    for (const itex_xla::WindowDimension& dim : window.dimensions()) {
      elements.push_back(getter(dim));
    }
    return GetI64DenseElementsAttr(elements);
  }

  static mlir::DenseIntElementsAttr GetLayoutAttribute(
      const itex_xla::Layout& layout, Builder* builder);

  itex::Status DefaultAction(const itex_xla::HloInstruction* instr) final;

  // Computation parameters don't need any specific handling when they are
  // visited, they are already processed when we enter a new computation.
  itex::Status HandleParameter(const itex_xla::HloInstruction* instr) final {
    return itex::Status::OK();
  }

  // Helper function that recursively visits the tuple structure in
  // `current_shape`, and reconstruct a matching lmhlo::TupleOp.
  // Each leaf node is converted to an std.view op with corresponding offsets.
  // If no tuple presents, it simply returns a view of the buffer.
  itex::Status GetOrCreateViewImpl(const itex_xla::HloInstruction* instr,
                                   const itex_xla::Shape& current_shape,
                                   itex_xla::ShapeIndex* current_shape_index,
                                   SmallVectorImpl<Value>* values,
                                   TokenLoweringMode token_mode);

  // Helper function to create view/tuple of views to a buffer for a given
  // instruction result. `result_subset` can be used to for instructions that
  // have a tuple result and MLIR conversion needs to convert only one of the
  // tuple elements. Note that if needed, this can be extended to take a list of
  // ShapeIndex values in case we need finer control on what elements of the
  // output tuple to be converted to MLIR.
  itex::Status GetOrCreateView(
      const itex_xla::HloInstruction* instr, SmallVectorImpl<Value>* values,
      const itex_xla::ShapeIndex& result_subset = {},
      TokenLoweringMode token_mode = TokenLoweringMode::kFailToLower);

  itex_xla::StatusOr<Value> GetOrCreateArrayView(
      const itex_xla::HloInstruction* instr,
      const itex_xla::Shape& current_shape,
      const itex_xla::ShapeIndex& current_shape_index);

  itex_xla::StatusOr<Value> RewriteFusionOperand(
      const itex_xla::HloInstruction* root, const itex_xla::Shape& shape,
      itex_xla::ShapeIndex* shape_index, OpBuilder* b, Location loc);

  // Return an MLIR location for an HLO instruction.
  Location getLocation(const itex_xla::HloInstruction* inst) {
    return NameLoc::get(builder_.getStringAttr(inst->name()));
  }

  // This map provides access to MLIR buffers for each HLO buffer allocation.
  // The MLIR buffers are all `memref<{size}xi8>` and correspond to function
  // parameters. It is populated at the beginning of the processing with all
  // the buffer allocations and is unchanged afterward. Every HLOInstruction
  // is using a "slice" of the buffer allocation and providing shape, layout,
  // and Dtype. An MLIR view is used separately to model slices into the
  // allocations (see below).
  llvm::DenseMap<const itex_xla::BufferAllocation*, Value> allocations_;

  // This map provides access to MLIR buffers for each HLO instruction, keyed
  // instruction identity. A slice is contained in a BufferAllocation, and has
  // an offset and a size.
  //
  // As for why we don't use HloInstruction*, see GetOrCreateView(), but
  // mostly we want to leverage better of the aliased buffers.
  //
  // If the HloInstruction is a tuple, all leaf nodes are stored flattened.
  // Otherwise, there will be a single buffer.
  //
  // An MLIR buffer is either an input parameter, or a ViewOp in the case
  // where the slice is only part of its allocation.
  //
  // `slices_` is populated lazily in the `GetOrCreateView()` helper as we
  // process every instruction.
  absl::flat_hash_map<
      std::pair<const itex_xla::HloInstruction*, itex_xla::ShapeIndex>, Value>
      slices_;

  // The BufferAssignment computed by XLA ahead of time.
  const itex_xla::BufferAssignment& assignment_;

  // The HLO module that will be converted.
  const itex_xla::HloComputation& computation_;

  // This is the MLIR module in which a function will be created for every HLO
  // computation.
  ModuleOp module_;

  // The builder keeps track of the current insertion point in the MLIR
  // module.
  OpBuilder builder_;
  // Convenient "cached" access to this widely used MLIR type (i8).
  Type i8_type_;

  // Map all-reduce-start ops to their LHLO op, so we can connect the
  // all-reduce-done op with the correct token.
  absl::flat_hash_map<const itex_xla::HloInstruction*,
                      lmhlo_gpu::AllReduceStartOp>
      all_reduce_start_ops_;
};

// Populate the MLIR `module` with the computation from the `hlo_module` using
// the provided buffer `assignment`. The returned `Status` indicates success
// or failure in the conversion.
itex::Status HloToLhloModule(const itex_xla::BufferAssignment& assignment,
                             const itex_xla::HloModule& hlo_module,
                             ModuleOp module);

itex::Status OptimizeAndConvertHloToLmhlo(
    std::unique_ptr<itex_xla::HloModule> hlo_module, ModuleOp module,
    StringRef platform_name, bool optimize_xla_hlo);
OwningOpRef<mlir::ModuleOp> HloTextToLhloTranslateFunction(
    llvm::StringRef input, MLIRContext* context, bool optimize_xla_hlo);

// This register the MLIR pass with the command line.
void RegisterMhloToLhloWithXlaPass();

}  // namespace mlir

#endif  // ITEX_CORE_COMPILER_MLIR_XLA_TRANSFORMS_MHLO_TO_LHLO_WITH_XLA_H_