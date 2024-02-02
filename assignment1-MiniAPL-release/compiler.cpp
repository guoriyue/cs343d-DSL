#include "MiniAPLJIT.h"

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Utils/SimplifyLibCalls.h"
#include "llvm/Transforms/Scalar/GVN.h"


#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <map>
#include <memory>
#include <string>
#include <regex>
#include <vector>
#include <cassert>

using namespace llvm;
using namespace llvm::orc;
using namespace std;

class ASTNode;

// -------------------------------------------------
// Miscellaneous helper functions 
// -------------------------------------------------

std::unique_ptr<ASTNode> LogError(const char *Str) {
  fprintf(stderr, "Error: %s\n", Str);
  return nullptr;
}

static inline
string str(const int i) {
  return to_string(i);
}

bool is_int( const std::string& str ) { // check with regex (does not accept leading zeroes before first digit)
  static constexpr auto max_digits = std::numeric_limits<int>::digits10 ;
  static const std::string ub = std::to_string(max_digits-1) ;
  static const std::regex int_re( "^\\s*([+-]?[1-9]\\d{0," + ub + "}|0)\\s*$" ) ;

  return std::regex_match( str, int_re ) ;
}

// -------------------------------------------------
// Type information for MiniAPL programs
// -------------------------------------------------

enum ExprType {
  EXPR_TYPE_SCALAR,
  EXPR_TYPE_FUNCALL,
  EXPR_TYPE_VARIABLE 
};

class MiniAPLArrayType {
  public:

    vector<int> dimensions;
    int innermost_dimension;
    int concat_dim1;
    int concat_dim2;
    int dim_to_concat;

    int Cardinality() {
      int C = 1;
      for (auto D : dimensions) {
        C *= D;
      }
      return C;
    }

    int length(const int dim) {
      return dimensions.at(dim);
    }

    int dimension() {
      return dimensions.size();
    }
};

std::ostream& operator<<(std::ostream& out, MiniAPLArrayType& tp) {
  out << "[";
  int i = 0;
  for (auto T : tp.dimensions) {
    out << T;
    if (i < (int) (tp.dimensions.size() - 1)) {
      out << ", ";
    }
    i++;
  }
  out << "]";
  return out;
}

// -------------------------------------------------
// AST classes 
// -------------------------------------------------

// The base class for all expression nodes.
class ASTNode {
  public:
    virtual ~ASTNode() = default;

    virtual Value *codegen(Function* F) = 0;
    virtual ExprType GetType() = 0;
    virtual void Print(std::ostream& out) {

    }
};

std::ostream& operator<<(std::ostream& out, ASTNode& tp) {
  tp.Print(out);
  return out;
}

class StmtAST: public ASTNode {
  public:
    virtual bool IsAssign() = 0;
};

class ProgramAST : public ASTNode {
  public:
    std::vector<unique_ptr<StmtAST> > Stmts;
    Value *codegen(Function* F) override;
    virtual ExprType GetType() override { return EXPR_TYPE_FUNCALL; }
};

class ExprStmtAST : public StmtAST {
  public:
    std::unique_ptr<ASTNode> Val;

    bool IsAssign() override { return false; }
    ExprStmtAST(std::unique_ptr<ASTNode> Val_) : Val(std::move(Val_)) {}
    Value *codegen(Function* F) override;
    virtual ExprType GetType() override { return EXPR_TYPE_FUNCALL; }

    virtual void Print(std::ostream& out) override {
      Val->Print(out);
    }
};


class VariableASTNode : public ASTNode {

  public:
    std::string Name;
    VariableASTNode(const std::string &Name) : Name(Name) {}

    Value *codegen(Function* F) override;

    virtual ExprType GetType() override { return EXPR_TYPE_VARIABLE; }

    virtual void Print(std::ostream& out) override {
      out << Name;
    }
};

class AssignStmtAST : public StmtAST {
  public:
    std::unique_ptr<VariableASTNode> Name;
    std::unique_ptr<ASTNode> RHS;

    bool IsAssign() override { return true; }
    Value *codegen(Function* F) override;

    std::string GetName() const { return Name->Name; }

    AssignStmtAST(const std::string& Name_, std::unique_ptr<ASTNode> val_) : Name(new VariableASTNode(Name_)), RHS(std::move(val_)) {}
    virtual ExprType GetType() override { return EXPR_TYPE_FUNCALL; }
    virtual void Print(std::ostream& out) override {
      out << "assign ";
      Name->Print(out);
      out << " = ";
      RHS->Print(out);
    }
};

class NumberASTNode : public ASTNode {
  public:
    int Val;
    NumberASTNode(int Val) : Val(Val) {}

    Value *codegen(Function* F) override;

    virtual ExprType GetType() override { return EXPR_TYPE_SCALAR; }

    virtual void Print(std::ostream& out) override {
      out << Val;
    }
};

class CallASTNode : public ASTNode {

  public:
    std::string Callee;
    std::vector<std::unique_ptr<ASTNode>> Args;
    CallASTNode(const std::string &Callee,
        std::vector<std::unique_ptr<ASTNode>> Args)
      : Callee(Callee), Args(std::move(Args)) {}

    Value *codegen(Function* F) override;
    virtual ExprType GetType() override { return EXPR_TYPE_FUNCALL; }
    virtual void Print(std::ostream& out) override {
      out << Callee << "(";
      for (int i = 0; i < (int) Args.size(); i++) {
        Args.at(i)->Print(out);
        if (i < (int) Args.size() - 1) {
          out << ", ";
        }
      }
      out << ")";
    }
};


// ---------------------------------------------------------------------------
// Some global variables used in parsing, type-checking, and code generation.
// ---------------------------------------------------------------------------
static map<ASTNode*, MiniAPLArrayType> TypeTable;
static map<string, Value*> ValueTable;
static LLVMContext TheContext;
// NOTE: You will probably want to use the Builder in the "codegen" methods
static IRBuilder<> Builder(TheContext);
static std::unique_ptr<Module> TheModule;
static std::unique_ptr<legacy::FunctionPassManager> TheFPM;
static std::unique_ptr<MiniAPLJIT> TheJIT;

// ---------------------------------------------------------------------------
// LLVM codegen helpers
// ---------------------------------------------------------------------------
IntegerType* intTy(const int width) {
  return IntegerType::get(TheContext, 32);
}

ConstantInt* intConst(const int width, const int i) {
  ConstantInt* const_int32 = ConstantInt::get(TheContext , APInt(width, StringRef(str(i)), 10));
  return const_int32;
}

static void InitializeModuleAndPassManager() {
  // Open a new module.
  TheModule->setDataLayout(TheJIT->getTargetMachine().createDataLayout());

  // Create a new pass manager attached to it.
  TheFPM = llvm::make_unique<legacy::FunctionPassManager>(TheModule.get());

  // Do simple "peephole" optimizations and bit-twiddling optzns.
  TheFPM->add(createInstructionCombiningPass());
  // Reassociate expressions.
  TheFPM->add(createReassociatePass());
  // Eliminate Common SubExpressions.
  TheFPM->add(createGVNPass());
  // Simplify the control flow graph (deleting unreachable blocks, etc).
  TheFPM->add(createCFGSimplificationPass());

  TheFPM->doInitialization();
}

// NOTE: This utility function generates LLVM IR to print out the string "to_print"
void kprintf_str(Module *mod, BasicBlock *bb, const std::string& to_print) {
  Function *func_printf = mod->getFunction("printf");
  if (!func_printf) {
    PointerType::get(IntegerType::get(mod->getContext(), 8), 0);
    FunctionType *FuncTy9 = FunctionType::get(IntegerType::get(mod->getContext(), 32), true);

    func_printf = Function::Create(FuncTy9, GlobalValue::ExternalLinkage, "printf", mod);
    func_printf->setCallingConv(CallingConv::C);
  }

  IRBuilder <> builder(TheContext);
  builder.SetInsertPoint(bb);


  Value *str = builder.CreateGlobalStringPtr(to_print);

  std::vector <Value *> int32_call_params;
  int32_call_params.push_back(str);

  CallInst::Create(func_printf, int32_call_params, "call", bb);

}

// NOTE: This utility function generates code that prints out the 32 bit input "val" when
// executed.
void kprintf_val(Module *mod, BasicBlock *bb, Value* val) {
  Function *func_printf = mod->getFunction("printf");
  if (!func_printf) {
    PointerType::get(IntegerType::get(mod->getContext(), 8), 0);
    FunctionType *FuncTy9 = FunctionType::get(IntegerType::get(mod->getContext(), 32), true);

    func_printf = Function::Create(FuncTy9, GlobalValue::ExternalLinkage, "printf", mod);
    func_printf->setCallingConv(CallingConv::C);
  }

  IRBuilder <> builder(TheContext);
  builder.SetInsertPoint(bb);


  Value *str = builder.CreateGlobalStringPtr("%d");

  std::vector <Value *> int32_call_params;
  int32_call_params.push_back(str);
  int32_call_params.push_back(val);

  CallInst::Create(func_printf, int32_call_params, "call", bb);
}

Value *LogErrorV(const char *Str) {
  LogError(Str);
  return nullptr;
}

// ---------------------------------------------------------------------------
// Code generation functions that you should fill in for this assignment
// ---------------------------------------------------------------------------
Value *ProgramAST::codegen(Function* F) {
  // STUDENTS: FILL IN THIS FUNCTION
  for(auto& Stmt : Stmts) {
    Stmt->codegen(F);
  }
  return nullptr;
}

Value *AssignStmtAST::codegen(Function* F) {
  // STUDENTS: FILL IN THIS FUNCTION
  Value* rhsValue = RHS->codegen(F);
  if (!rhsValue)
    return nullptr;
  string Name = GetName();
  // printf("AssignStmtAST Name, %s\n", Name.c_str());
  ValueTable[Name] = rhsValue;
  return rhsValue;
}


Value *ExprStmtAST::codegen(Function* F) {
  // STUDENTS: FILL IN THIS FUNCTION
  // printf("ExprStmtAST\n");
  // Val->Print(std::cout);
  return Val->codegen(F);
}

Value *NumberASTNode::codegen(Function* F) {
  // STUDENTS: FILL IN THIS FUNCTION
  return ConstantInt::get(TheContext, APInt(32, Val));
}

Value *VariableASTNode::codegen(Function* F) {
  // STUDENTS: FILL IN THIS FUNCTION
  // printf("VariableASTNode Name, %s\n", Name.c_str());
  Value *V = ValueTable[Name];
  if (!V)
    LogErrorV("Unknown variable name");
  return V;
}

void codegen_print_array(vector<int> dims, Value* array_data, Module *m, BasicBlock *bb, unsigned dim, unsigned prefix) {
  kprintf_str(m, bb, "[");
  unsigned current_dim = dims[dim];
  int prev_dim = 1;

  // 2 3 4
  // 12
  for (unsigned i = dim + 1; i < dims.size(); ++i) {
    prev_dim *= dims[i];
  }
  // cout << "current_dim " << current_dim << " prev_dim " << prev_dim << endl;

  // for (unsigned i = 0; i < size; ++i) {
    //   cout << "i " << i << endl;
    //   // Value* element_ptr = Builder.CreateGEP(arg0, {intConst(32, 0), intConst(32, i)});
    //   // Value* element_i = Builder.CreateLoad(element_ptr);
    //   Value* element_i = Builder.CreateExtractElement(arg0, i);
    //   // Value* arg_i = Args[i] -> codegen(F);
    //   kprintf_str(m, bb, "[");
    //   kprintf_val(m, bb, element_i);
    //   kprintf_str(m, bb, "]");
    // }


  for (unsigned i = 0; i < current_dim; ++i) {
    if (dim == dims.size() - 1) {
      // cout << "prefix " << prefix << " i " << i << endl;
      // Value* element_ptr = Builder.CreateGEP(array_data, {intConst(32, 0), intConst(32, prefix + i)});
      // // cout << "after gep" << endl;
      // Value* element_i = Builder.CreateLoad(element_ptr);
      Value* element_i = Builder.CreateExtractElement(array_data, prefix + i);
      // cout << "before kprintf_val" << endl;
      kprintf_str(m, bb, "[");
      kprintf_val(m, bb, element_i);
      kprintf_str(m, bb, "]");
      // cout << "after kprintf_val" << endl;
    } else {
      // cout << "recursive" << endl;
      codegen_print_array(dims, array_data, m, bb, dim + 1, prefix + i * prev_dim);
    }
  }
  kprintf_str(m, bb, "]");
}
Value *CallASTNode::codegen(Function* F) {
  // Look up the name in the global module table.
  // printf("CallASTNode Callee, %s\n", Callee.c_str());

  // std::vector<Value *> ArgsV;
  // for (unsigned i = 0, e = Args.size(); i != e; ++i) {
  //   ArgsV.push_back(Args[i]->codegen(F));
  //   if (!ArgsV.back())
  //     return nullptr;
  // }

  BasicBlock *bb = Builder.GetInsertBlock();
  Module *m = TheModule.get();
  
  if (Callee == "add") {
    // Get the type of the result (and operands).
    MiniAPLArrayType type = TypeTable[this];
    const int size = type.Cardinality();
    // Get an LLVM type for the flattened array.
    auto *vec_type = VectorType::get(intTy(32), size);

    // Codegen arguments.
    Value *arg0 = Args[0]->codegen(F);
    Value *arg1 = Args[1]->codegen(F);

    // Load from the arguments.
    arg0 = Builder.CreateLoad(vec_type, arg0);
    arg1 = Builder.CreateLoad(vec_type, arg1);

    // Construct an add (this builds a vector addition).
    Value *add = Builder.CreateAdd(arg0, arg1);

    

    // Allocate a vector of size `size` and type int32.
    auto alloc = Builder.CreateAlloca(vec_type);
    // Get a pointer to the start of the allocation.
    // See https://releases.llvm.org/6.0.0/docs/GetElementPtr.html for more details.
    auto dst = Builder.CreateGEP(alloc, {intConst(32, 0), intConst(32, 0)});
    // Store the add in the destination.
    Builder.CreateStore(add, dst);

    

    // for (unsigned i = 0; i < size; ++i) {
    //   Value* element_ptr = Builder.CreateGEP(alloc, {intConst(32, 0), intConst(32, i)});
    //   Value* element_i = Builder.CreateLoad(element_ptr);
    //   kprintf_str(m, bb, "[");
    //   kprintf_val(m, bb, element_i);
    //   kprintf_str(m, bb, "]");
    // }

    // codegen_print_array(type.dimensions, alloc, m, bb, 0, 0);
    Value* arg_print = Builder.CreateLoad(vec_type, alloc);
    codegen_print_array(type.dimensions, arg_print, m, bb, 0, 0);
    kprintf_str(m, bb, "\n");

    return alloc;
  } else if (Callee == "sub") {
    MiniAPLArrayType type = TypeTable[this];
    const int size = type.Cardinality();
    auto *vec_type = VectorType::get(intTy(32), size);

    Value *arg0 = Args[0]->codegen(F);
    Value *arg1 = Args[1]->codegen(F);

    arg0 = Builder.CreateLoad(vec_type, arg0);
    arg1 = Builder.CreateLoad(vec_type, arg1);

    Value *sub = Builder.CreateSub(arg0, arg1);

    auto alloc = Builder.CreateAlloca(vec_type);
    auto dst = Builder.CreateGEP(alloc, {intConst(32, 0), intConst(32, 0)});
    Builder.CreateStore(sub, dst);

    // codegen_print_array(type.dimensions, sub, m, bb, 0, 0);
    Value* arg_print = Builder.CreateLoad(vec_type, alloc);
    codegen_print_array(type.dimensions, arg_print, m, bb, 0, 0);
    kprintf_str(m, bb, "\n");

    return alloc;

  } else if (Callee == "mkArray") {
    
    MiniAPLArrayType type = TypeTable[this];
    const int size = type.Cardinality(); // 6
    const int dim_length = type.dimensions.size();
    auto *vec_type = VectorType::get(intTy(32), size);
    // printf("dim_length %d\n", dim_length);
    // printf("tensor size %d\n", size);
    // printf("Args size %d\n", Args.size());
    std::vector<Value *> ArgsV;
    for (unsigned i = 1+dim_length, e = Args.size(); i != e; ++i) {
      ArgsV.push_back(Args[i]->codegen(F));
    }
    
    auto array_data = Builder.CreateAlloca(vec_type);
    for (unsigned i = 0; i < ArgsV.size(); ++i) {
      auto dst = Builder.CreateGEP(array_data, {intConst(32, 0), intConst(32, i)});
      Builder.CreateStore(ArgsV[i], dst);
    }

    // codegen_print_array(type.dimensions, array_data, m, bb, 0, 0);
    

    return array_data;
  } else if (Callee == "neg") {
    MiniAPLArrayType type = TypeTable[this];
    const int size = type.Cardinality();
    auto *vec_type = VectorType::get(intTy(32), size);

    Value *arg0 = Args[0]->codegen(F);

    arg0 = Builder.CreateLoad(vec_type, arg0);

    // // // kprintf_str(m, bb, "[");
    // // // kprintf_val(m, bb, arg0);
    // // // kprintf_str(m, bb, "]");
    // for (unsigned i = 0; i < size; ++i) {
    //   cout << "i " << i << endl;
    //   // Value* element_ptr = Builder.CreateGEP(arg0, {intConst(32, 0), intConst(32, i)});
    //   // Value* element_i = Builder.CreateLoad(element_ptr);
    //   Value* element_i = Builder.CreateExtractElement(arg0, i);
    //   // Value* arg_i = Args[i] -> codegen(F);
    //   kprintf_str(m, bb, "[");
    //   kprintf_val(m, bb, element_i);
    //   kprintf_str(m, bb, "]");
    // }

    Value* neg_ones = Builder.CreateVectorSplat(size, intConst(32, -1));
    Value* neg_arg0 = Builder.CreateMul(arg0, neg_ones);
    auto alloc = Builder.CreateAlloca(vec_type);
    auto dst = Builder.CreateGEP(alloc, {intConst(32, 0), intConst(32, 0)});
    Builder.CreateStore(neg_arg0, dst);


    // codegen_print_array(type.dimensions, alloc, m, bb, 0, 0);

    
    Value* arg_print = Builder.CreateLoad(vec_type, alloc);
    codegen_print_array(type.dimensions, arg_print, m, bb, 0, 0);
    kprintf_str(m, bb, "\n");

    return alloc;
  } else if (Callee == "exp") {
    MiniAPLArrayType type = TypeTable[this];
    const int size = type.Cardinality();
    auto *vec_type = VectorType::get(intTy(32), size);
    // auto *int_type = intTy(32);

    // cout << "size " << size << endl;

    Value *arg0 = Args[0]->codegen(F);
    // arg0 = Builder.CreateGEP(arg0, {intConst(32, 0), intConst(32, 0)});
    arg0 = Builder.CreateLoad(vec_type, arg0);


    // for (unsigned i = 0; i < size; ++i) {
    //   cout << "i " << i << endl;
    //   // Value* element_ptr = Builder.CreateGEP(arg0, {intConst(32, 0), intConst(32, i)});
    //   // Value* element_i = Builder.CreateLoad(element_ptr);
    //   Value* element_i = Builder.CreateGEP(arg0, {intConst(32, 0), intConst(32, i)});
    //   element_i = Builder.CreateLoad(element_i);
    //   // Value* element_i = Builder.CreateExtractElement(arg0, i);
    //   element_i->print(llvm::outs());
    //   std::cout << "\n";
    //   // // Value* arg_i = Args[i] -> codegen(F);
    //   // kprintf_str(m, bb, "[");
    //   // kprintf_val(m, bb, element_i);
    //   // kprintf_str(m, bb, "]");
    // }


    Value *power = Args[1]->codegen(F);


    // Value *res = Builder.CreateExp(arg0, power);

    // power = Builder.CreateLoad(int_type, power);

    Value *base = Args[0]->codegen(F);

    base = Builder.CreateLoad(vec_type, base);

    // how to iterate from 0 to power?
    Value *StartVal = intConst(32, 0); // ConstantFP::get(TheContext, APFloat(0.0));
    // Make the new basic block for the loop header, inserting after current
    // block.
    Function *TheFunction = Builder.GetInsertBlock()->getParent();
    BasicBlock *PreheaderBB = Builder.GetInsertBlock();
    BasicBlock *LoopBB =
        BasicBlock::Create(TheContext, "loop", TheFunction);
    string VarName = "i_";
    string MulName = "exp_";
    // Insert an explicit fall through from the current block to the LoopBB.
    Builder.CreateBr(LoopBB);

    // Start insertion in LoopBB.
    Builder.SetInsertPoint(LoopBB);
    

    // Start the PHI node with an entry for Start.
    PHINode *Variable = Builder.CreatePHI(Type::getInt32Ty(TheContext), 2, VarName);
    Variable->addIncoming(StartVal, PreheaderBB);

    Value* ones = Builder.CreateVectorSplat(size, intConst(32, 1));

    PHINode *Result = Builder.CreatePHI(vec_type, 2, MulName);
    Result->addIncoming(ones, PreheaderBB);
    

    // Within the loop, the variable is defined equal to the PHI node.  If it
    // shadows an existing variable, we have to restore it, so save it now.
    Value *OldVal = ValueTable[VarName];
    ValueTable[VarName] = Variable;

    // Emit the body of the loop.  This, like any other expr, can change the
    // current BB.  Note that we ignore the value computed by the body, but don't
    // allow an error.
    // if (!Body->codegen())
    //   return nullptr;
    // arg0 = Builder.CreateMul(arg0, base);
    // cout << "arg0 " << endl;
    Value* NextResult = Builder.CreateMul(Result, arg0);

    Value *StepVal = intConst(32, 1); // ConstantFP::get(TheContext, APFloat(1.0));
    // // Emit the step value.
    // Value *StepVal = nullptr;
    // if (Step) {
    //   StepVal = Step->codegen();
    //   if (!StepVal)
    //     return nullptr;
    // } else {
    //   // If not specified, use 1.0.
    //   StepVal = ConstantFP::get(*TheContext, APFloat(1.0));
    // }

    Value *NextVar = Builder.CreateAdd(Variable, StepVal, "nextvar");

    // Compute the end condition.


    // Value *EndCond = Builder.CreateICmpEQ(
    //     NextVar, power, "loopcond");

    Value *EndCond = Builder.CreateICmpEQ(
        NextVar, power, "loopcond");

    // kprintf_str(m, LoopBB, "EndCond ");
    // kprintf_val(m, LoopBB, EndCond);
    // kprintf_str(m, LoopBB, "\n ");
    // kprintf_str(m, LoopBB, "NextVar ");
    // kprintf_val(m, LoopBB, NextVar);
    // kprintf_str(m, LoopBB, "\n ");
    // kprintf_str(m, LoopBB, "power ");
    // kprintf_val(m, LoopBB, power);
    // kprintf_str(m, LoopBB, "\n ");
    // kprintf_str(m, LoopBB, "Result ");
    // kprintf_val(m, LoopBB, Result);
    // kprintf_str(m, LoopBB, "\n ");
    // kprintf_str(m, LoopBB, "NextResult ");
    // kprintf_val(m, LoopBB, NextResult);
    // kprintf_str(m, LoopBB, "\n ");

    // if (!EndCond)
    //   return nullptr;

    // // Convert condition to a bool by comparing non-equal to 0.0.
    // EndCond = Builder.CreateFCmpOEQ(
    //     EndCond, ConstantFP::get(*TheContext, APFloat(0.0)), "loopcond");


    // Create the "after loop" block and insert it.
    BasicBlock *LoopEndBB = Builder.GetInsertBlock();
    BasicBlock *AfterBB =
        BasicBlock::Create(TheContext, "afterloop", TheFunction);

    // Insert the conditional branch into the end of LoopEndBB.
    Builder.CreateCondBr(EndCond, AfterBB, LoopBB);

    // Any new code will be inserted in AfterBB.
    Builder.SetInsertPoint(AfterBB);


    
    


    // Add a new entry to the PHI node for the backedge.
    Variable->addIncoming(NextVar, LoopEndBB);
    Result->addIncoming(NextResult, LoopEndBB);

    // Restore the unshadowed variable.
    if (OldVal)
      ValueTable[VarName] = OldVal;
    else
      ValueTable.erase(VarName);


    // Value* pp = Builder.CreateMul(arg0, arg0);

    

    // // for expr always returns 0.0.
    // return Constant::getNullValue(Type::getDoubleTy(*TheContext));

    // Value* arg0_ = Builder.CreateMul(arg0, arg0);


    // arg0 = Builder.CreateMul(arg0, base);
    auto alloc = Builder.CreateAlloca(vec_type);
    auto dst = Builder.CreateGEP(alloc, {intConst(32, 0), intConst(32, 0)});
    Builder.CreateStore(NextResult, dst);

    // // // codegen_print_array(type.dimensions, alloc, m, bb, 0, 0);
    Value* arg_print = Builder.CreateLoad(vec_type, alloc);
    codegen_print_array(type.dimensions, arg_print, m, AfterBB, 0, 0);
    kprintf_str(m, AfterBB, "\n");

    // // Value* resultVector = Result;

    // BasicBlock *bb = Builder.GetInsertBlock();
    // // Module *m = TheModule.get();
    // for (unsigned i = 0; i < size; ++i) {
    //   cout << "i " << i << endl;
    //   // Value* element_ptr = Builder.CreateGEP(arg_print, {intConst(32, 0), intConst(32, i)});
    //   // Value* element_i = Builder.CreateLoad(element_ptr);
    //   // Value* element_i = Builder.CreateGEP(alloc, {intConst(32, 0), intConst(32, i)});
    //   // element_i = Builder.CreateLoad(element_i);
    //   Value* element_i = Builder.CreateExtractElement(arg_print, i);
    //   // element_i->print(llvm::outs());
    //   // std::cout << "\n";
    //   // // Value* arg_i = Args[i] -> codegen(F);
    //   kprintf_str(m, AfterBB, "[");
    //   kprintf_val(m, AfterBB, element_i);
    //   kprintf_str(m, AfterBB, "]");
    // }
    



    return alloc;




    // Value* ones = Builder.CreateVectorSplat(size, intConst(32, -1));
    // Value* neg_arg0 = Builder.CreateMul(arg0, ones);

    // auto alloc = Builder.CreateAlloca(vec_type);
    // auto dst = Builder.CreateGEP(alloc, {intConst(32, 0), intConst(32, 0)});
    // // auto ptr_type = PointerType::get(vec_type, 0);
    // // Value* dst = Builder.CreatePointerCast(alloc, ptr_type);
    // Builder.CreateStore(neg_arg0, dst);
    // // std::cout << "here 2\n";


    // // codegen_print_array(type.dimensions, alloc, m, bb, 0, 0);

    // return alloc;
  } else if (Callee == "print") {
    MiniAPLArrayType type = TypeTable[this];
    const int size = type.Cardinality();
    auto *vec_type = VectorType::get(intTy(32), size);

    Value *arg0 = Args[0]->codegen(F);

    arg0 = Builder.CreateLoad(vec_type, arg0);

    // // // kprintf_str(m, bb, "[");
    // // // kprintf_val(m, bb, arg0);
    // // // kprintf_str(m, bb, "]");
    // for (unsigned i = 0; i < size; ++i) {
    //   cout << "i " << i << endl;
    //   // Value* element_ptr = Builder.CreateGEP(arg0, {intConst(32, 0), intConst(32, i)});
    //   // Value* element_i = Builder.CreateLoad(element_ptr);
    //   Value* element_i = Builder.CreateExtractElement(arg0, i);
    //   // Value* arg_i = Args[i] -> codegen(F);
    //   kprintf_str(m, bb, "[");
    //   kprintf_val(m, bb, element_i);
    //   kprintf_str(m, bb, "]");
    // }

    // Value* neg_ones = Builder.CreateVectorSplat(size, intConst(32, -1));
    // Value* neg_arg0 = Builder.CreateMul(arg0, neg_ones);
    // auto alloc = Builder.CreateAlloca(vec_type);
    // auto dst = Builder.CreateGEP(alloc, {intConst(32, 0), intConst(32, 0)});
    // Builder.CreateStore(neg_arg0, dst);


    // // codegen_print_array(type.dimensions, alloc, m, bb, 0, 0);

    
    // Value* arg_print = Builder.CreateLoad(vec_type, alloc);
    codegen_print_array(type.dimensions, arg0, m, bb, 0, 0);
    kprintf_str(m, bb, "\n");

    return nullptr;
  } else if (Callee == "reduce") {
    // reduce(<array>)` - Turn an N dimensional array into an N-1 dimensional array by adding up all numbers in the innermost dimension

    MiniAPLArrayType type = TypeTable[this];
    const int size = type.Cardinality();
    auto *vec_type = VectorType::get(intTy(32), size);
    auto innermost = type.innermost_dimension;
    auto *original_vec_type = VectorType::get(intTy(32), size * innermost);

    Value *arg0 = Args[0]->codegen(F);
    arg0 = Builder.CreateLoad(original_vec_type, arg0);

    // for (int i=0;i<size;i++){
    //   Value* element = Builder.CreateExtractElement(arg0, i);
    //   element->print(llvm::outs());
    // }

    // auto dimension = type.dimensions.size();
    // for (int i=0;i<dimension;i++){
    //   cout << "dimension " << i << " " << type.dimensions[i] << endl;
    // }
    
    // int new_size = size;
    // auto *new_vec_type = VectorType::get(intTy(32), size);
    auto alloc = Builder.CreateAlloca(vec_type);

    // cout << "size " << size << " innermost " << innermost << endl;

    for (int i = 0; i < size; i++) {
      Value *sum = intConst(32, 0);
      for (int j = 0; j < innermost; j++) {
        Value *element = Builder.CreateExtractElement(arg0, i * innermost + j);
        // element->print(llvm::outs());
        sum = Builder.CreateAdd(sum, element);
        
      }
      auto dst = Builder.CreateGEP(alloc, {intConst(32, 0), intConst(32, i)});
      Builder.CreateStore(sum, dst);
    }

    // Value *sub = Builder.CreateSub(arg0, arg1);

    // auto alloc = Builder.CreateAlloca(vec_type);
    // auto dst = Builder.CreateGEP(alloc, {intConst(32, 0), intConst(32, 0)});
    // Builder.CreateStore(sub, dst);

    // codegen_print_array(type.dimensions, sub, m, bb, 0, 0);
    Value* arg_print = Builder.CreateLoad(vec_type, alloc);
    codegen_print_array(type.dimensions, arg_print, m, bb, 0, 0);
    kprintf_str(m, bb, "\n");

    return alloc;
  } else if (Callee == "expand") {
    MiniAPLArrayType type = TypeTable[this];
    const int size = type.Cardinality();
    auto *vec_type = VectorType::get(intTy(32), size);
    auto *original_vec_type = VectorType::get(intTy(32), size / type.innermost_dimension);

    // cout << "size " << size << " innermost " << type.innermost_dimension << endl;
    // for (int i=0;i<type.dimensions.size();i++){
    //   cout << "dimension " << i << " " << type.dimensions[i] << endl;
    // }
    // dimension 0 3
    // dimension 1 1

    Value *arg0 = Args[0]->codegen(F);

    arg0 = Builder.CreateLoad(original_vec_type, arg0);

    int nth_dim = type.innermost_dimension;
    int n_plus_1 = type.dimensions[type.dimensions.size() - 1];

    int prev_dims = 1;
    for (int i = 0; i < type.dimensions.size() - 2; i++) {
      prev_dims *= type.dimensions[i];
    }

    auto alloc = Builder.CreateAlloca(vec_type);
    // prev dim
    // n_plus_1
    for (int i = 0; i < nth_dim * n_plus_1; i++) {
      int k = i / nth_dim;
      for (int j = 0; j < prev_dims; j++) {
        Value *element = Builder.CreateExtractElement(arg0, j * n_plus_1 + k);
        for (int q = 0; q < nth_dim; q++) {
          auto dst = Builder.CreateGEP(alloc, {intConst(32, 0), intConst(32, j * n_plus_1 * nth_dim + n_plus_1*q + k)});
          Builder.CreateStore(element, dst);
        }
      }
    }
    // for (int i = 0; i < nth_dim; i++) {
    //   for (int j = 0; j < prev_dims; j++) {
    //     Value *element = Builder.CreateExtractElement(arg0, j * nth_dim + i);
    //     for (int k = 0; k < n_plus_1; k++) {
    //       auto dst = Builder.CreateGEP(alloc, {intConst(32, 0), intConst(32, j * n_plus_1 + n_plus_1*k + i)});
    //       Builder.CreateStore(element, dst);
    //     }
    //   }
    // }
    // for (int i = 0; i < n_plus_1; i++) {
    //   for (int j = 0; j < prev_dims; j++) {
    //     Value *element = Builder.CreateExtractElement(arg0, j * n_plus_1 + i);
    //     for (int k = 0; k < nth_dim; k++) {
    //       auto dst = Builder.CreateGEP(alloc, {intConst(32, 0), intConst(32, j * n_plus_1 + n_plus_1*k + i)});
    //       Builder.CreateStore(element, dst);
    //     }
    //   }
    // }

    Value* arg_print = Builder.CreateLoad(vec_type, alloc);
    codegen_print_array(type.dimensions, arg_print, m, bb, 0, 0);
    kprintf_str(m, bb, "\n");
    return alloc;

    // // // kprintf_str(m, bb, "[");
    // // // kprintf_val(m, bb, arg0);
    // // // kprintf_str(m, bb, "]");
    // for (unsigned i = 0; i < size; ++i) {
    //   cout << "i " << i << endl;
    //   // Value* element_ptr = Builder.CreateGEP(arg0, {intConst(32, 0), intConst(32, i)});
    //   // Value* element_i = Builder.CreateLoad(element_ptr);
    //   Value* element_i = Builder.CreateExtractElement(arg0, i);
    //   // Value* arg_i = Args[i] -> codegen(F);
    //   kprintf_str(m, bb, "[");
    //   kprintf_val(m, bb, element_i);
    //   kprintf_str(m, bb, "]");
    // }

    // Value* neg_ones = Builder.CreateVectorSplat(size, intConst(32, -1));
    // Value* neg_arg0 = Builder.CreateMul(arg0, neg_ones);
    // auto alloc = Builder.CreateAlloca(vec_type);
    // auto dst = Builder.CreateGEP(alloc, {intConst(32, 0), intConst(32, 0)});
    // Builder.CreateStore(neg_arg0, dst);
  } else if (Callee == "concat") {
    MiniAPLArrayType type = TypeTable[this];
    const int size = type.Cardinality();
    auto *vec_type = VectorType::get(intTy(32), size);
    auto *original_vec_type_1 = VectorType::get(intTy(32), size * type.concat_dim1/ (type.concat_dim1 + type.concat_dim2));
    auto *original_vec_type_2 = VectorType::get(intTy(32), size * type.concat_dim2/ (type.concat_dim1 + type.concat_dim2));

    Value *arg0 = Args[0]->codegen(F);
    Value *arg1 = Args[1]->codegen(F);
    int dim_to_concat = type.dim_to_concat;

    arg0 = Builder.CreateLoad(original_vec_type_1, arg0);
    arg1 = Builder.CreateLoad(original_vec_type_2, arg1);
    // arg2 = Builder.CreateLoad(vec_type, arg2);

    int prev_dims = 1;
    for (int i = 0; i < dim_to_concat; i++) {
      prev_dims *= type.dimensions[i];
    }
    int next_dims = 1;
    for (int i = dim_to_concat + 1; i < type.dimensions.size(); i++) {
      next_dims *= type.dimensions[i];
    }

    auto alloc = Builder.CreateAlloca(vec_type);

    for (int i = 0; i < type.concat_dim1; i++) {
      for (int j = 0; j < prev_dims; j++) {
        for (int k = 0; k < next_dims; k++) {
          Value *element = Builder.CreateExtractElement(arg0, j * type.concat_dim1 * next_dims + i * next_dims + k);
          auto dst = Builder.CreateGEP(alloc, {intConst(32, 0), intConst(32, j * (type.concat_dim1 + type.concat_dim2) * next_dims + i * next_dims + k)});
          Builder.CreateStore(element, dst);
        }
      }
    }

    for (int i = 0; i < type.concat_dim2; i++) {
      for (int j = 0; j < prev_dims; j++) {
        for (int k = 0; k < next_dims; k++) {
          Value *element = Builder.CreateExtractElement(arg1, j * type.concat_dim2 * next_dims + i * next_dims + k);
          auto dst = Builder.CreateGEP(alloc, {intConst(32, 0), intConst(32, j * (type.concat_dim1 + type.concat_dim2) * next_dims + type.concat_dim1 * next_dims + i * next_dims + k)});
          Builder.CreateStore(element, dst);
        }
      }
    }



    Value* arg_print = Builder.CreateLoad(vec_type, alloc);
    codegen_print_array(type.dimensions, arg_print, m, bb, 0, 0);
    kprintf_str(m, bb, "\n");
    return alloc;
  } else {
    return nullptr;
  }
  return nullptr;
}

// ---------------------------------------------------------------------------
// Parser utilities 
// ---------------------------------------------------------------------------
class ParseState {
  public:
    int Position;
    vector<string> Tokens;

    ParseState(vector<string>& Tokens_) : Position(0), Tokens(Tokens_) {}

    bool AtEnd() {
      return Position == (int) Tokens.size();
    }


    string peek() {
      if (AtEnd()) {
        return "";
      }
      return Tokens.at(Position);
    }

    string peek(const int Offset) {
      assert(Position + Offset < Tokens.size());
      return Tokens.at(Position + Offset);
    }

    string eat() {
      auto Current = peek();
      Position++;
      return Current;
    }
};

std::ostream& operator<<(std::ostream& out, ParseState& PS) {
  int i = 0;
  for (auto T : PS.Tokens) {
    if (i == PS.Position) {
      out << " | ";
    }
    out << T << " ";
    i++;
  }
  return out;
}

#define EAT(PS, t) if (PS.eat() != (t)) { return LogError("EAT ERROR"); }

unique_ptr<ASTNode> ParseExpr(ParseState& PS) {
  string Name = PS.eat();
  if (is_int(Name)) {
    return unique_ptr<ASTNode>(new NumberASTNode(stoi(Name)));
  }

  if (PS.peek() == "(") {
    // Parse a function call

    PS.eat(); // consume "("

    vector<unique_ptr<ASTNode> > Args;
    while (PS.peek() != ")") {
      Args.push_back(ParseExpr(PS));
      if (PS.peek() != ")") {
        EAT(PS, ",");
      }
    }
    EAT(PS, ")");

    return unique_ptr<ASTNode>(new CallASTNode(Name, move(Args)));
  } else {
    return unique_ptr<ASTNode>(new VariableASTNode(Name));
  }
}

// ---------------------------------------------------------------------------
// Driver function for type-checking 
// ---------------------------------------------------------------------------
void SetType(map<ASTNode*, MiniAPLArrayType>& Types, ASTNode* Expr) {
  if (Expr->GetType() == EXPR_TYPE_FUNCALL) {
    CallASTNode* Call = static_cast<CallASTNode*>(Expr);
    for (auto& A : Call->Args) {
      SetType(Types, A.get());
    }

    if (Call->Callee == "mkArray") {
      int NDims = static_cast<NumberASTNode*>(Call->Args.at(0).get())->Val;
      vector<int> Dims;
      for (int i = 0; i < NDims; i++) {
        Dims.push_back(static_cast<NumberASTNode*>(Call->Args.at(i + 1).get())->Val);
      }
      Types[Expr] = {Dims};
    } else if (Call->Callee == "reduce") {
      Types[Expr] = Types[Call->Args.back().get()];
      Types[Expr].innermost_dimension = Types[Expr].dimensions[Types[Expr].dimensions.size() - 1];
      Types[Expr].dimensions.pop_back();
    } else if (Call->Callee == "expand") {
      Types[Expr] = Types[Call->Args.at(0).get()];
      // for (int i = 0; i < Types[Expr].dimensions.size(); i++) {
      //   cout << "exppppppand dimension " << i << " " << Types[Expr].dimensions[i] << endl;
      // }
      int expand_times = static_cast<NumberASTNode*>(Call->Args.at(1).get())->Val;
      int last_dim = Types[Expr].dimensions[Types[Expr].dimensions.size() - 1];
      Types[Expr].innermost_dimension = expand_times;
      // here innermost_dimension is the expanded dimension, not actual innermost dimension
      // just use the same name for simplicity
      Types[Expr].dimensions.pop_back();
      Types[Expr].dimensions.push_back(expand_times);
      Types[Expr].dimensions.push_back(last_dim);
    } else if (Call->Callee == "concat") {
      // cout << "concat" << endl;
      auto dim1 = Types[Call->Args.at(0).get()].dimensions;
      auto dim2 = Types[Call->Args.at(1).get()].dimensions;
      int concat_dim = static_cast<NumberASTNode*>(Call->Args.at(2).get())->Val;
      // for (int i = 0; i < dim1.size(); i++) {
      //   cout << "dim1 " << i << " " << dim1[i] << endl;
      // }
      // for (int i = 0; i < dim2.size(); i++) {
      //   cout << "dim2 " << i << " " << dim2[i] << endl;
      // }
      // cout << "concat_dim " << concat_dim << endl;
      auto final_dim = dim1;
      final_dim[concat_dim] += dim2[concat_dim];
      Types[Expr] = Types[Call->Args.at(0).get()];
      Types[Expr].dimensions = final_dim;
      Types[Expr].concat_dim1 = dim1[concat_dim];
      Types[Expr].concat_dim2 = dim2[concat_dim];
      Types[Expr].dim_to_concat = concat_dim;
    } else if (Call->Callee == "add" || Call->Callee == "sub") {
      Types[Expr] = Types[Call->Args.at(0).get()];
    } else {
      Types[Expr] = Types[Call->Args.at(0).get()];
    }
  } else if (Expr->GetType() == EXPR_TYPE_SCALAR) {
    Types[Expr] = {{1}};
  } else if (Expr->GetType() == EXPR_TYPE_VARIABLE) {
    string ExprName = static_cast<VariableASTNode*>(Expr)->Name;
    for (auto T : Types) {
      auto V = T.first;
      if (V->GetType() == EXPR_TYPE_VARIABLE) {
        string Name = static_cast<VariableASTNode*>(V)->Name;
        if (Name == ExprName) {
          Types[Expr] = T.second;
        }
      }
    }
  }

}

int main(const int argc, const char** argv) {
  assert(argc == 2);

  // Read in the source code file to a string
  string target_file = argv[1];

  std::ifstream t(target_file);
  std::string str((std::istreambuf_iterator<char>(t)),
      std::istreambuf_iterator<char>());

  // Tokenize the file
  vector<string> Tokens;
  string NextToken = "";
  for (int i = 0; i < (int) str.size(); i++) {
    char NC = str[i];
    if (NC == ',' || NC == '(' || NC == ')' || NC == ';' || NC == '=') {
      if (NextToken != "") {
        Tokens.push_back(NextToken);
      }
      NextToken = string("") + NC;
      Tokens.push_back(NextToken);
      NextToken = "";
    } else if (!isspace(NC)) {
      NextToken += NC;
    } else {
      assert(isspace(NC));
      if (NextToken != "") {
        Tokens.push_back(NextToken);
      }
      NextToken = "";
    }
  }
  if (NextToken != "") {
    Tokens.push_back(NextToken);
  }

  vector<vector<string> > Stmts;
  vector<string> Toks;
  for (auto t : Tokens) {
    if (t == ";") {
      Stmts.push_back(Toks);
      Toks = {};
    } else {
      Toks.push_back(t);
    }
  }

  if (Toks.size() > 0) {
    Stmts.push_back(Toks);
  }

  // Parse each statement
  vector<unique_ptr<StmtAST> > ParsedStmts;
  for (auto S : Stmts) {
    ParseState PS(S);
    assert(S.size() > 0);
    if (PS.peek() != "assign") {
      unique_ptr<ASTNode> value = ParseExpr(PS);
      ParsedStmts.push_back(std::unique_ptr<StmtAST>(new ExprStmtAST(move(value))));
    } else {
      PS.eat(); // eat "assign"

      string Var = PS.eat();

      if (PS.eat() != "=") {
      } else {
        unique_ptr<ASTNode> value = ParseExpr(PS);
        ParsedStmts.push_back(std::unique_ptr<StmtAST>(new AssignStmtAST(Var, move(value))));
      }
    }
  }

  // Collect the statements into a program
  ProgramAST prog;
  prog.Stmts = move(ParsedStmts);

  // Infer types
  for (auto& S : prog.Stmts) {
    StmtAST* SA = S.get();
    if (SA->IsAssign()) {
      AssignStmtAST* Assign = static_cast<AssignStmtAST*>(SA);
      SetType(TypeTable, Assign->RHS.get());
      TypeTable[Assign->Name.get()] = TypeTable[Assign->RHS.get()];
    } else {
      ExprStmtAST* Expr = static_cast<ExprStmtAST*>(SA);
      SetType(TypeTable, Expr->Val.get());
    }
  }

  TheModule = llvm::make_unique<Module>("MiniAPL Module " + target_file, TheContext);
  std::vector<Type *> Args(0, Type::getDoubleTy(TheContext));
  FunctionType *FT =
    FunctionType::get(Type::getVoidTy(TheContext), Args, false);

  Function *F =
    Function::Create(FT, Function::ExternalLinkage, "__anon_expr", TheModule.get());
  BasicBlock::Create(TheContext, "entry", F);
  Builder.SetInsertPoint(&(F->getEntryBlock()));

  prog.codegen(F);

  Builder.CreateRet(nullptr);

  // NOTE: You may want to uncomment this line to see the LLVM IR you have generated
  TheModule->print(errs(), nullptr);

  // Initialize the JIT, compile the module to a function,
  // find the function and then run it.
  InitializeNativeTarget();
  InitializeNativeTargetAsmPrinter();
  InitializeNativeTargetAsmParser();
  TheJIT = llvm::make_unique<MiniAPLJIT>();
  InitializeModuleAndPassManager();
  auto H = TheJIT->addModule(std::move(TheModule));


  auto ExprSymbol = TheJIT->findSymbol("__anon_expr");
  void (*FP)() = (void (*)())(intptr_t)cantFail(ExprSymbol.getAddress());
  assert(FP != nullptr);
  FP();

  TheJIT->removeModule(H);

  return 0;
}
