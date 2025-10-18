#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"
#include <llvm/IR/Analysis.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/LLVMContext.h>

using namespace llvm;

namespace {

struct SkeletonPass : public PassInfoMixin<SkeletonPass> {
  PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM) {
    LLVMContext &ctx = M.getContext();

    Type *void_type = Type::getVoidTy(ctx);

    if (M.getName() == "rt.cpp")
      return PreservedAnalyses::all();

    if (Function *f = M.getFunction("main"); f && !f->isDeclaration()) {
      FunctionType *summary_fn_type = FunctionType::get(void_type, false);
      auto summary_fn =
          M.getOrInsertFunction("__instrumentor_summary", summary_fn_type);
      Function *p_summary_fn = cast<Function>(summary_fn.getCallee());
      appendToGlobalDtors(M, p_summary_fn, 0);
    }

    for (auto &F : M) {
      // errs() << "I saw a function called " << F.getName() << "!\n";
      if (F.isDeclaration())
        continue;

      auto &entry_block = F.getEntryBlock();
      auto insert_iter = entry_block.getFirstInsertionPt();
      IRBuilder<> builder(&entry_block);
      builder.SetInsertPoint(&entry_block, insert_iter);

      Type *char_type = Type::getInt8Ty(ctx);
      PointerType *c_str_type = PointerType::get(char_type, 0);
      FunctionType *instrument_fn_type =
          FunctionType::get(void_type, {c_str_type}, false);
      auto instrument_fn =
          M.getOrInsertFunction("__instrumentor_incr_cnt", instrument_fn_type);

      auto fn_name = builder.CreateGlobalString(F.getName());
      builder.CreateCall(instrument_fn, {fn_name});
    }
    return PreservedAnalyses::all();
  };
};

} // namespace

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return {.APIVersion = LLVM_PLUGIN_API_VERSION,
          .PluginName = "Skeleton pass",
          .PluginVersion = "v0.1",
          .RegisterPassBuilderCallbacks = [](PassBuilder &PB) {
            PB.registerPipelineStartEPCallback(
                [](ModulePassManager &MPM, OptimizationLevel Level) {
                  MPM.addPass(SkeletonPass());
                });
          }};
}
