#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include <llvm/ADT/SetVector.h>
#include <llvm/Analysis/ValueTracking.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/InstrTypes.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/PassManager.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Transforms/Scalar/LoopPassManager.h>
#include <llvm/Transforms/Utils.h>
#include <llvm/Transforms/Utils/LoopUtils.h>
#include <llvm/Transforms/Utils/Mem2Reg.h>

using namespace llvm;

namespace {

struct LICMPass : public PassInfoMixin<LICMPass> {
  PreservedAnalyses run(Loop &L, LoopAnalysisManager &LAM,
                        LoopStandardAnalysisResults &AR, LPMUpdater &U) {
    // get the loop preheader or insert one
    auto preheader = L.getLoopPreheader();
    if (!preheader) {
      preheader = InsertPreheaderForLoop(&L, &AR.DT, &AR.LI, nullptr, false);
    }

    SetVector<Value *> loop_invariants;

    bool changing = true;
    while (changing) {
      size_t initial_size = loop_invariants.size();

      for (auto *B : L.blocks()) {
        for (auto &I : *B) {
          if (all_of(I.operands(),
                     [&](Value *v) {
                       return L.isLoopInvariant(v) ||
                              loop_invariants.contains(v);
                     }) &&
              isSafeToSpeculativelyExecute(&I) && !I.mayReadFromMemory() &&
              !I.isEHPad()) {
            loop_invariants.insert(&I);
          }
        }
      }

      changing = loop_invariants.size() != initial_size;
    }

    for (auto V : loop_invariants) {
      Instruction *I = cast<Instruction>(V);
      I->moveBefore(preheader->getTerminator());
    }

    return PreservedAnalyses::none();
  }
  static bool isRequired() { return true; }
};

} // namespace

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return {.APIVersion = LLVM_PLUGIN_API_VERSION,
          .PluginName = "LICM pass",
          .PluginVersion = "v0.1",
          .RegisterPassBuilderCallbacks = [](PassBuilder &PB) {
            PB.registerPipelineStartEPCallback([](ModulePassManager &MPM,
                                                  OptimizationLevel Level) {
              FunctionPassManager FPM;
              FPM.addPass(PromotePass());
              FPM.addPass(createFunctionToLoopPassAdaptor(LICMPass()));
              MPM.addPass(createModuleToFunctionPassAdaptor(std::move(FPM)));
            });
          }};
}
