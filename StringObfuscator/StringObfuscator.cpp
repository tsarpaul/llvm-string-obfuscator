#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/ValueSymbolTable.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include <typeinfo>

using namespace std;
using namespace llvm;

namespace {
class GlobalString {
	public:
		GlobalVariable* Glob;
		unsigned int index;
		int type;

		static const int SIMPLE_STRING_TYPE = 1;
		static const int STRUCT_STRING_TYPE = 2;

		GlobalString(GlobalVariable* Glob) : Glob(Glob), index(-1), type(SIMPLE_STRING_TYPE) {}
		GlobalString(GlobalVariable* Glob, unsigned int index) : Glob(Glob), index(index), type(STRUCT_STRING_TYPE) {}
};

Function *createDecodeStubFunc(Module &M, vector<GlobalString*> &GlobalStrings, Function *DecodeFunc){
	auto &Ctx = M.getContext();
	// Add DecodeStub function
	FunctionCallee DecodeStubCallee = M.getOrInsertFunction("decode_stub",
/*ret*/		Type::getVoidTy(Ctx));
	Function *DecodeStubFunc = cast<Function>(DecodeStubCallee.getCallee());
	DecodeStubFunc->setCallingConv(CallingConv::C);

	// Create entry block
	BasicBlock *BB = BasicBlock::Create(Ctx, "entry", DecodeStubFunc);
	IRBuilder<> Builder(BB);

	// Add calls to decode every encoded global
	for(GlobalString *GlobString : GlobalStrings){
		if(GlobString->type == GlobString->SIMPLE_STRING_TYPE){
			auto *StrPtr = Builder.CreatePointerCast(GlobString->Glob, Type::getInt8PtrTy(Ctx, 8));
			Builder.CreateCall(DecodeFunc, {StrPtr});
		}
		else if(GlobString->type == GlobString->STRUCT_STRING_TYPE){
			auto *String = Builder.CreateStructGEP(GlobString->Glob, GlobString->index);
			auto *StrPtr = Builder.CreatePointerCast(String, Type::getInt8PtrTy(Ctx, 8));
			Builder.CreateCall(DecodeFunc, {StrPtr});
		}
	}
	Builder.CreateRetVoid();

	return DecodeStubFunc;
}

Function *createDecodeFunc(Module &M){
	auto &Ctx = M.getContext();

	// Add Decode function
	FunctionCallee DecodeFuncCallee = M.getOrInsertFunction("decode",
/*ret*/			Type::getVoidTy(Ctx),
/*args*/		Type::getInt8PtrTy(Ctx, 8));
	Function *DecodeFunc = cast<Function>(DecodeFuncCallee.getCallee());
	DecodeFunc->setCallingConv(CallingConv::C);

	// Name DecodeFunc arguments
	Function::arg_iterator Args = DecodeFunc->arg_begin();
	Value *StrPtr = Args++;
	StrPtr->setName("StrPtr");

	// Create blocks
	BasicBlock *BEntry = BasicBlock::Create(Ctx, "entry", DecodeFunc);
	BasicBlock *BWhileBody = BasicBlock::Create(Ctx, "while.body", DecodeFunc);
	BasicBlock *BWhileEnd = BasicBlock::Create(Ctx, "while.end", DecodeFunc);

	// Entry block
	IRBuilder<> *Builder = new IRBuilder<>(BEntry);
	auto *var0 = Builder->CreateLoad(StrPtr, "var0");
	auto *cmp5 = Builder->CreateICmpEQ(var0, Constant::getNullValue(Type::getInt8Ty(Ctx)), "cmp5");
	Builder->CreateCondBr(cmp5, BWhileEnd, BWhileBody);

	// Preheader block
	Builder = new IRBuilder<>(BWhileBody);
	PHINode *var1 = Builder->CreatePHI(Type::getInt8Ty(Ctx), 2, "var1");
	PHINode *stringaddr06 = Builder->CreatePHI(Type::getInt8PtrTy(Ctx, 8), 2, "stringaddr06");
	auto *sub11 = Builder->CreateSub(var1, ConstantInt::get(IntegerType::get(Ctx, 8), 1, true));
	Builder->CreateStore(sub11, stringaddr06);
	auto *incdecptr = Builder->CreateGEP(stringaddr06, ConstantInt::get(IntegerType::get(Ctx, 64), 1), "incdecptr");
	auto *var2 = Builder->CreateLoad(incdecptr, "var2");
	auto cmp = Builder->CreateICmpEQ(var2, ConstantInt::get(IntegerType::get(Ctx, 8), 0), "cmp");
	Builder->CreateCondBr(cmp, BWhileEnd, BWhileBody);

	// End block
	Builder = new IRBuilder<>(BWhileEnd);
	Builder->CreateRetVoid();

	// Fill in PHIs
	var1->addIncoming(var0, BEntry);
	var1->addIncoming(var2, BWhileBody);
	stringaddr06->addIncoming(incdecptr, BWhileBody);
	stringaddr06->addIncoming(StrPtr, BEntry);

	return DecodeFunc;
}

BasicBlock& createDecodeStubBlock(Function *F, Function *DecodeStubFunc){
	auto &Ctx = F->getContext();
	BasicBlock &EntryBlock = F->getEntryBlock();

	// Create new block
	BasicBlock *NewBB = BasicBlock::Create(Ctx, "DecodeStub", EntryBlock.getParent(), &EntryBlock);
	IRBuilder<> Builder(NewBB);

	// Call stub func instruction
	Builder.CreateCall(DecodeStubFunc);
	// Jump to original entry block
	Builder.CreateBr(&EntryBlock);

	return *NewBB;
}

char *EncodeString(const char* Data, unsigned int Length) {
	// Encode string
	char *NewData = (char*)malloc(Length);
	for(unsigned int i = 0; i < Length; i++){
		NewData[i] = Data[i] + 1;
	}

	return NewData;
}

vector<GlobalString*> encodeGlobalStrings(Module& M){
	vector<GlobalString*> GlobalStrings;
	auto &Ctx = M.getContext();

	// Encode all global strings
	for(GlobalVariable &Glob : M.globals()) {
		// Ignore external globals & uninitialized globals.
		if (!Glob.hasInitializer() || Glob.hasExternalLinkage())
			continue;

		auto Initializer = Glob.getInitializer();
		if (isa<ConstantDataArray>(Initializer)) {
			// Unwrap GlobalVariable
			auto CDA = cast<ConstantDataArray>(Initializer);
			if (!CDA->isCString())
				continue;

			// Extract raw string
			StringRef StrVal = CDA->getAsCString();	
			const char *Data = StrVal.begin();
			const int Size = StrVal.size();

			// Create encoded string variable
			char *NewData = EncodeString(Data, Size);
			auto *NewConst = ConstantDataArray::getString(Ctx, StringRef(NewData, Size), CDA->isCString());

			// Overwrite the global value
			Glob.setInitializer(NewConst);

			GlobalStrings.push_back(new GlobalString(&Glob));
			Glob.setConstant(false);
		}
		else if(isa<ConstantStruct>(Initializer)){
			// Handle structs
			auto CS = cast<ConstantStruct>(Initializer);
			if(Initializer->getNumOperands() > 1)
				continue; // TODO: Fix bug when removing this: "Constant not found in constant table!"
			for(unsigned int i = 0; i < Initializer->getNumOperands(); i++){
				auto CDA = dyn_cast<ConstantDataArray>(CS->getOperand(i));
				if (!CDA || !CDA->isString())
					continue;

				// Extract raw string
				StringRef StrVal = CDA->getAsString();
				const char *Data = StrVal.begin();
				const int Size = StrVal.size();

				// Create encoded string variable
				char *NewData = EncodeString(Data, Size);
				Constant *NewConst = ConstantDataArray::getString(Ctx, StringRef(NewData, Size), CDA->isCString());

				// Overwrite the struct member
				CS->setOperand(i, NewConst);

				GlobalStrings.push_back(new GlobalString(&Glob, i));
				Glob.setConstant(false);
			}
		}
	}

	return GlobalStrings;
}

struct StringObfuscatorModPass : public PassInfoMixin<StringObfuscatorModPass> {
	PreservedAnalyses run(Module &M, ModuleAnalysisManager &MAM) {
	auto GlobalStrings = encodeGlobalStrings(M);

	// Inject functions
	Function *DecodeFunc = createDecodeFunc(M);

	Function *DecodeStub = createDecodeStubFunc(M, GlobalStrings, DecodeFunc);

	// Inject a call to DecodeStub to main
	Function *MainFunc = M.getFunction("main");
	
	// Inject a block that calls DecodeStub
	createDecodeStubBlock(MainFunc, DecodeStub);

	return PreservedAnalyses::all();
	};
};

struct StringObfuscatorFuncPass : public PassInfoMixin<StringObfuscatorFuncPass> {
	PreservedAnalyses run(Function &F, FunctionAnalysisManager &FAM) {
		return PreservedAnalyses::all();
	};
};
} // end anonymous namespace

extern "C" ::llvm::PassPluginLibraryInfo LLVM_ATTRIBUTE_WEAK
llvmGetPassPluginInfo() {
	return {
		LLVM_PLUGIN_API_VERSION, "StringObfuscatorPass", "v0.1", [](PassBuilder &PB) {
			PB.registerPipelineParsingCallback([](StringRef Name, ModulePassManager &MPM,
					 ArrayRef<PassBuilder::PipelineElement>) {
					if(Name == "string-obfuscator-pass"){
						MPM.addPass(StringObfuscatorModPass());
						return true;
					}
					return false;
				}
			);
		}
	};
}
