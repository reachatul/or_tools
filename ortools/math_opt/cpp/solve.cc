// Copyright 2010-2021 Google LLC
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "ortools/math_opt/cpp/solve.h"

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "absl/base/thread_annotations.h"
#include "absl/container/flat_hash_set.h"
#include "absl/memory/memory.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "absl/synchronization/mutex.h"
#include "ortools/base/logging.h"
#include "ortools/base/source_location.h"
#include "ortools/base/status_macros.h"
#include "ortools/math_opt/callback.pb.h"
#include "ortools/math_opt/core/model_storage.h"
#include "ortools/math_opt/core/solver.h"
#include "ortools/math_opt/cpp/key_types.h"
#include "ortools/math_opt/cpp/model.h"

namespace operations_research {
namespace math_opt {

namespace {

Solver::InitArgs ToSolverInitArgs(const SolverInitArguments& arguments) {
  Solver::InitArgs solver_init_args;
  solver_init_args.streamable = arguments.streamable.Proto();
  if (arguments.non_streamable != nullptr) {
    solver_init_args.non_streamable = arguments.non_streamable.get();
  }

  return solver_init_args;
}

// Asserts (with CHECK) that the input pointer is either nullptr or that it
// points to the same model storage as storage_.
void CheckModelStorage(const ModelStorage* const storage,
                       const ModelStorage* const expected_storage) {
  if (storage != nullptr) {
    CHECK_EQ(storage, expected_storage)
        << internal::kObjectsFromOtherModelStorage;
  }
}

absl::StatusOr<SolveResult> CallSolve(
    Solver& solver, const ModelStorage* const expected_storage,
    const SolveArguments& arguments) {
  CheckModelStorage(/*storage=*/arguments.model_parameters.storage(),
                    /*expected_storage=*/expected_storage);
  CheckModelStorage(/*storage=*/arguments.callback_registration.storage(),
                    /*expected_storage=*/expected_storage);

  if (arguments.callback == nullptr) {
    CHECK(arguments.callback_registration.events.empty())
        << "No callback was provided to run, but callback events were "
           "registered.";
  }

  Solver::Callback cb = nullptr;
  if (arguments.callback != nullptr) {
    cb = [&](const CallbackDataProto& callback_data_proto) {
      const CallbackData data(expected_storage, callback_data_proto);
      const CallbackResult result = arguments.callback(data);
      CheckModelStorage(/*storage=*/result.storage(),
                        /*expected_storage=*/expected_storage);
      return result.Proto();
    };
  }
  ASSIGN_OR_RETURN(
      SolveResultProto solve_result,
      solver.Solve(
          {.parameters = arguments.parameters.Proto(),
           .model_parameters = arguments.model_parameters.Proto(),
           .message_callback = arguments.message_callback,
           .callback_registration = arguments.callback_registration.Proto(),
           .user_cb = std::move(cb),
           .interrupter = arguments.interrupter}));
  return SolveResult::FromProto(expected_storage, solve_result);
}

class PrinterMessageCallbackImpl {
 public:
  PrinterMessageCallbackImpl(std::ostream& output_stream,
                             const absl::string_view prefix)
      : output_stream_(output_stream), prefix_(prefix) {}

  void Call(const std::vector<std::string>& messages) {
    const absl::MutexLock lock(&mutex_);
    for (const std::string& message : messages) {
      output_stream_ << prefix_ << message << '\n';
    }
    output_stream_.flush();
  }

 private:
  absl::Mutex mutex_;
  std::ostream& output_stream_ ABSL_GUARDED_BY(mutex_);
  const std::string prefix_;
};

}  // namespace

SolverInitArguments::SolverInitArguments(
    StreamableSolverInitArguments streamable)
    : streamable(std::move(streamable)) {}

SolverInitArguments::SolverInitArguments(
    const NonStreamableSolverInitArguments& non_streamable)
    : non_streamable(non_streamable.Clone()) {}

SolverInitArguments::SolverInitArguments(
    StreamableSolverInitArguments streamable,
    const NonStreamableSolverInitArguments& non_streamable)
    : streamable(std::move(streamable)),
      non_streamable(non_streamable.Clone()) {}

SolverInitArguments::SolverInitArguments(const SolverInitArguments& other)
    : streamable(other.streamable),
      non_streamable(other.non_streamable != nullptr
                         ? other.non_streamable->Clone()
                         : nullptr) {}

SolverInitArguments& SolverInitArguments::operator=(
    const SolverInitArguments& other) {
  // Assignment to self is possible.
  if (&other == this) {
    return *this;
  }

  streamable = other.streamable;
  non_streamable =
      other.non_streamable != nullptr ? other.non_streamable->Clone() : nullptr;

  return *this;
}

absl::StatusOr<SolveResult> Solve(const Model& model,
                                  const SolverType solver_type,
                                  const SolveArguments& solve_args,
                                  const SolverInitArguments& init_args) {
  ASSIGN_OR_RETURN(const std::unique_ptr<Solver> solver,
                   Solver::New(EnumToProto(solver_type), model.ExportModel(),
                               ToSolverInitArgs(init_args)));
  return CallSolve(*solver, model.storage(), solve_args);
}

absl::StatusOr<std::unique_ptr<IncrementalSolver>> IncrementalSolver::New(
    Model& model, const SolverType solver_type, SolverInitArguments arguments) {
  std::unique_ptr<UpdateTracker> update_tracker = model.NewUpdateTracker();
  ASSIGN_OR_RETURN(
      std::unique_ptr<Solver> solver,
      Solver::New(EnumToProto(solver_type), update_tracker->ExportModel(),
                  ToSolverInitArgs(arguments)));
  return absl::WrapUnique<IncrementalSolver>(
      new IncrementalSolver(solver_type, std::move(arguments), model.storage(),
                            std::move(update_tracker), std::move(solver)));
}

IncrementalSolver::IncrementalSolver(
    SolverType solver_type, SolverInitArguments init_args,
    const ModelStorage* const expected_storage,
    std::unique_ptr<UpdateTracker> update_tracker,
    std::unique_ptr<Solver> solver)
    : solver_type_(solver_type),
      init_args_(std::move(init_args)),
      expected_storage_(expected_storage),
      update_tracker_(std::move(update_tracker)),
      solver_(std::move(solver)) {}

absl::StatusOr<SolveResult> IncrementalSolver::Solve(
    const SolveArguments& arguments) {
  RETURN_IF_ERROR(Update().status());
  return SolveWithoutUpdate(arguments);
}

absl::StatusOr<IncrementalSolver::UpdateResult> IncrementalSolver::Update() {
  std::optional<ModelUpdateProto> model_update =
      update_tracker_->ExportModelUpdate();
  if (!model_update) {
    return UpdateResult(true, std::move(model_update));
  }

  ASSIGN_OR_RETURN(const bool did_update, solver_->Update(*model_update));
  update_tracker_->Checkpoint();

  if (did_update) {
    return UpdateResult(true, std::move(model_update));
  }

  ASSIGN_OR_RETURN(solver_, Solver::New(EnumToProto(solver_type_),
                                        update_tracker_->ExportModel(),
                                        ToSolverInitArgs(init_args_)));

  return UpdateResult(false, std::move(model_update));
}

absl::StatusOr<SolveResult> IncrementalSolver::SolveWithoutUpdate(
    const SolveArguments& arguments) const {
  return CallSolve(*solver_, expected_storage_, arguments);
}

MessageCallback PrinterMessageCallback(std::ostream& output_stream,
                                       const absl::string_view prefix) {
  // Here we must use an std::shared_ptr since std::function requires that its
  // input is copyable. And PrinterMessageCallbackImpl can't be copyable since
  // it uses an absl::Mutex that is not.
  const auto impl =
      std::make_shared<PrinterMessageCallbackImpl>(output_stream, prefix);
  return
      [=](const std::vector<std::string>& messages) { impl->Call(messages); };
}

}  // namespace math_opt
}  // namespace operations_research
