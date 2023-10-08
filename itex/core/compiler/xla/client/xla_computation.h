/* Copyright (c) 2023 Intel Corporation

Copyright 2018 The TensorFlow Authors. All Rights Reserved.

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

#ifndef ITEX_CORE_COMPILER_XLA_CLIENT_XLA_COMPUTATION_H_
#define ITEX_CORE_COMPILER_XLA_CLIENT_XLA_COMPUTATION_H_

#include <memory>
#include <string>
#include <utility>

#include "itex/core/compiler/xla/shape.h"
#include "itex/core/compiler/xla/status_macros.h"
#include "protos/hlo.pb.h"
#include "protos/xla_data.pb.h"

namespace itex_xla {

// The computation graph that the user builds up with the XlaBuilder.
class XlaComputation {
 public:
  XlaComputation() : unique_id_(-1) {}
  explicit XlaComputation(HloModuleProto proto)
      : unique_id_(proto.id()), proto_(std::move(proto)) {}

  ~XlaComputation() {}

  XlaComputation(const XlaComputation&) = delete;
  XlaComputation& operator=(const XlaComputation&) = delete;

  XlaComputation(XlaComputation&& from) = default;

  XlaComputation& operator=(XlaComputation&& from) = default;

  // Returns the "program shape" (parameter and return shapes) for this
  // computation.
  StatusOr<ProgramShape> GetProgramShape() const;

  const std::string& name() const { return proto().name(); }

  const HloModuleProto& proto() const { return proto_; }
  HloModuleProto* mutable_proto() { return &proto_; }

  // Requests that we snapshot the computation into a serializable protocol
  // buffer form.
  StatusOr<std::unique_ptr<HloSnapshot>> Snapshot() const;

  // Returns true if this object is a null Computation.
  bool IsNull() const { return unique_id_ == -1; }

 private:
  explicit XlaComputation(const int64_t unique_id) : unique_id_(unique_id) {}
  friend class XlaBuilder;

  int64_t unique_id_;
  HloModuleProto proto_;
};

}  // namespace itex_xla

#endif  // ITEX_CORE_COMPILER_XLA_CLIENT_XLA_COMPUTATION_H_